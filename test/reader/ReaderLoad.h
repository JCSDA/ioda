/*
 * (C) Copyright 2025 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#pragma once

#include <algorithm>
#include <cmath>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

#define ECKIT_TESTING_SELF_REGISTER_CASES 0

#include <boost/noncopyable.hpp>

#include "eckit/config/LocalConfiguration.h"
#include "eckit/exception/Exceptions.h"
#include "eckit/testing/Test.h"

#include "ioda/containers/FrameCols.h"
#include "ioda/containers/FrameMetadata.h"
#include "ioda/containers/FrameRows.h"
#include "ioda/containers/IFrame.h"
#include "ioda/ioPool/IoPoolParameters.h"
#include "ioda/ObsDataIoParameters.h"
#include "ioda/reader/load/loadObs.hpp"

#include "oops/mpi/mpi.h"
#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"
#include "oops/util/Logger.h"
#include "oops/util/missingValues.h"
#include "oops/util/TimeWindow.h"

namespace ioda {
namespace test {

// -----------------------------------------------------------------------------
void checkOsdf(const eckit::LocalConfiguration & testConfig,
               const eckit::mpi::Comm & commAll,
               const std::string & expectedFrameType,
               const std::unique_ptr<osdf::IFrame> & testOsdf,
               const osdf::FrameMetadata & osdfMetadata) {
  // Get the proper config for expected data
  const int myRank = commAll.rank();
  const int mySize = commAll.size();
  const std::string sizeRankKey = "mpi size" + std::to_string(mySize) +
                                  ".rank" + std::to_string(myRank);

  // Verify the shape of the OSDF container
  const std::size_t expectedNumRows = testConfig.getUnsigned(sizeRankKey + ".nlocs");
  const std::size_t expectedNumCols = testConfig.getUnsigned(sizeRankKey + ".nvars");
  const std::size_t numRows = testOsdf->numRows();
  const std::size_t numCols = testOsdf->numCols();
  EXPECT_EQUAL(numRows, expectedNumRows);
  EXPECT_EQUAL(numCols, expectedNumCols);

  // Check for the existence of a few sample columns
  const std::vector<std::string> sampleColNames =
      testConfig.getStringVector("sample column names");
  for (const auto & colName : sampleColNames) {
    EXPECT(testOsdf->hasColumn(colName));
  }

  // Check for the non existence of a few problem columns
  const std::vector<std::string> sampleNonColNames
    = testConfig.getStringVector("sample non column names");
  for (const auto &colName : sampleNonColNames) {
    EXPECT(!testOsdf->hasColumn(colName));
  }

  // Check units of these sample columns
  const std::vector<std::string> sampleColUnits = testConfig.getStringVector("sample column units");
  EXPECT_EQUAL(sampleColNames.size(), sampleColUnits.size());
  for (size_t index = 0; index < sampleColNames.size(); ++index) {
    EXPECT_EQUAL(testOsdf->getColumnUnits(sampleColNames[index]), sampleColUnits[index]);
  }

  // Check frame type
  EXPECT_EQUAL(testOsdf->frameType(), expectedFrameType);

  // Check the osdf metadata contents:
  //   1) slice dimensions (e.g. Channel, Level, nfactors)
  //   2) vars with slice dimensions (e.g. Channel)
  //   3) number of vars

  // Generalized slice dimension checks (issue 1744 multi-dimension coverage).
  //   "non location dimensions" - verify the coordinate values of each registered dimension
  //   "multi slice vars" - verify the full set of variables with any slice
  //                        dimension.
  const eckit::LocalConfiguration nonLocDimConfig =
    testConfig.getSubConfiguration("non location dimensions");
  for (const auto & dimConfig : nonLocDimConfig.getSubConfigurations()) {
    const std::string dimName = dimConfig.getString("name");
    const std::vector<int> expectedNums = dimConfig.getIntVector("numbers");
    EXPECT_EQUAL(osdfMetadata.getDimNums(dimName), expectedNums);
  }

  std::vector<std::string> expectedMultiSliceVars;
  const eckit::LocalConfiguration multiSliceVarConfig =
    testConfig.getSubConfiguration("multi slice vars");
  for (const auto & dimConfig : multiSliceVarConfig.getSubConfigurations()) {
    const std::string varName = dimConfig.getString("name");
    const std::string nonLocDimName = dimConfig.getString("non location dimension");
    expectedMultiSliceVars.push_back(varName);
    EXPECT_EQUAL(osdfMetadata.varSliceDimName(varName), nonLocDimName);
  }
  const std::unordered_set<std::string> expectedMultiSliceVarsSet(
    expectedMultiSliceVars.begin(), expectedMultiSliceVars.end());
  EXPECT(osdfMetadata.getMultiSliceVars() == expectedMultiSliceVarsSet);

  // Value fidelity checks for the NaN/inf -> missing remapping performed by the
  // reader (replaceFillValuesWithMissing in loadObsFromNetcdf.cpp). Both blocks
  // below are optional, so existing obs types that omit them are unaffected.
  const float missingFloat = util::missingValue<float>();

  // 1) "float columns to sweep": on EVERY MPI rank, no element of the named
  //    float columns may be NaN or inf (since the reader is supposed to
  //    translate these to JEDI float missing values). This is invariant under
  //    how rows are distributed across ranks, so it runs regardless of comm size.
  const std::vector<std::string> sweepCols =
      testConfig.getStringVector("float columns to sweep", {});
  for (const auto & colName : sweepCols) {
    EXPECT(testOsdf->hasColumn(colName));
    std::vector<float> colData;
    testOsdf->getColumn(colName, colData);
    for (const float value : colData) {
      EXPECT(!std::isnan(value));
      EXPECT(!std::isinf(value));
    }
  }

  // 2) "value checks": exact column contents, order-independent (the finite
  //    values and the count of missing values). MPI distribution splits a
  //    column's rows across ranks, so these run only when there is a single
  //    rank and all rows are local.
  if (mySize == 1 && testConfig.has("value checks")) {
    const std::vector<eckit::LocalConfiguration> valueChecks =
        testConfig.getSubConfigurations("value checks");
    for (const auto & valueCheck : valueChecks) {
      const std::string colName = valueCheck.getString("column");
      EXPECT(testOsdf->hasColumn(colName));
      std::vector<float> colData;
      testOsdf->getColumn(colName, colData);

      std::vector<float> finiteValues;
      std::size_t missingCount = 0;
      for (const float value : colData) {
        if (value == missingFloat) {
          ++missingCount;
        } else {
          finiteValues.push_back(value);
        }
      }
      std::sort(finiteValues.begin(), finiteValues.end());

      const std::vector<double> expectedFiniteD =
          valueCheck.getDoubleVector("finite values");
      std::vector<float> expectedFinite(expectedFiniteD.begin(), expectedFiniteD.end());
      std::sort(expectedFinite.begin(), expectedFinite.end());

      EXPECT_EQUAL(missingCount, valueCheck.getUnsigned("missing count"));
      EXPECT(finiteValues == expectedFinite);
    }
  }
}

// -----------------------------------------------------------------------------
void testFrameRows() {
  // Configuration contains a list of subconfigs that each contain a file name,
  // start location index, and a location count
  const std::vector<eckit::LocalConfiguration> loadConfigs =
      ::test::TestEnvironment::config().getSubConfigurations("obs types");
  const eckit::LocalConfiguration timeWindowConfig =
      ::test::TestEnvironment::config().getSubConfiguration("time window");
  const util::TimeWindow timeWindow(timeWindowConfig);

  for (auto & config : loadConfigs) {
    // Create parameters for obsdatain and io pool
    oops::Log::info() << "testFrameRows: config = " << config << std::endl;
    const eckit::LocalConfiguration obsDataInConfig = config.getSubConfiguration("obsdatain");
    const eckit::LocalConfiguration testConfig = config.getSubConfiguration("test data");
    ioda::ObsDataInParameters dataInParams;
    dataInParams.deserialize(obsDataInConfig);

    // Simulated variable names, used only by the generator backends (empty for file backends)
    const std::vector<std::string> obsVarNames =
        config.getStringVector("simulated variables", {});

    const eckit::LocalConfiguration ioPoolConfig = config.getSubConfiguration("io pool");
    ioda::IoPool::IoPoolParameters ioPoolParams;
    ioPoolParams.validateAndDeserialize(ioPoolConfig);

    // Create a row-oriented OSDF
    std::unique_ptr<osdf::IFrame> testOsdf = std::make_unique<osdf::FrameRows>();

    // Collectively call the loadObs function with all io pool members
    // Need to set the frame type in the osdf metadata before calling loadObs.
    // In normal usage, the obs space would set the frame type prior to calling loadObs.
    osdf::FrameMetadata osdfMetadata;
    const eckit::mpi::Comm & commAll = oops::mpi::world();
    reader::loadObs(dataInParams, ioPoolParams, commAll, obsVarNames, timeWindow,
                    "ReaderLoad test", testOsdf, osdfMetadata);
    checkOsdf(testConfig, commAll, "FrameRows", testOsdf, osdfMetadata);
  }
}

// -----------------------------------------------------------------------------
void testFrameCols() {
  // Configuration contains a list of subconfigs that each contain a file name,
  // start location index, and a location count
  const std::vector<eckit::LocalConfiguration> loadConfigs =
      ::test::TestEnvironment::config().getSubConfigurations("obs types");
  const eckit::LocalConfiguration timeWindowConfig =
      ::test::TestEnvironment::config().getSubConfiguration("time window");
  const util::TimeWindow timeWindow(timeWindowConfig);

  for (auto & config : loadConfigs) {
    // Create parameters for obsdatain and io pool
    oops::Log::info() << "testFrameCols: config = " << config << std::endl;
    const eckit::LocalConfiguration obsDataInConfig = config.getSubConfiguration("obsdatain");
    const eckit::LocalConfiguration testConfig = config.getSubConfiguration("test data");
    ioda::ObsDataInParameters dataInParams;
    dataInParams.deserialize(obsDataInConfig);

    // Simulated variable names, used only by the generator backends (empty for file backends)
    const std::vector<std::string> obsVarNames =
        config.getStringVector("simulated variables", {});

    const eckit::LocalConfiguration ioPoolConfig = config.getSubConfiguration("io pool");
    ioda::IoPool::IoPoolParameters ioPoolParams;
    ioPoolParams.validateAndDeserialize(ioPoolConfig);

    // Create a column-oriented OSDF
    std::unique_ptr<osdf::IFrame> testOsdf = std::make_unique<osdf::FrameCols>();

    // Collectively call the loadObs function with all io pool members
    // In normal usage, the obs space would set the frame type prior to calling loadObs.
    osdf::FrameMetadata osdfMetadata;
    const eckit::mpi::Comm & commAll = oops::mpi::world();
    reader::loadObs(dataInParams, ioPoolParams, commAll, obsVarNames, timeWindow,
                    "ReaderLoad test", testOsdf, osdfMetadata);
    checkOsdf(testConfig, commAll, "FrameCols", testOsdf, osdfMetadata);
  }
}

// -----------------------------------------------------------------------------
class ReaderLoad : public oops::Test {
 public:
  ReaderLoad() {}
  virtual ~ReaderLoad() {}

 private:
  std::string testid() const override {return "test::ReaderLoad";}

  void register_tests() const override {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    ts.emplace_back(CASE("ioda/ReaderLoad/testFrameRows")
      { testFrameRows(); });
    ts.emplace_back(CASE("ioda/ReaderLoad/testFrameCols")
      { testFrameCols(); });
  }

  void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda
