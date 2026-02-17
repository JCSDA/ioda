/*
 * (C) Copyright 2025 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#pragma once

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

  // Check the osdf metadata contents:
  //   1) frame type
  //   2) channel numbers
  //   3) vars with channels
  //   4) number of vars
  //   5) date time epoch

  // Frame type
  const std::string frameType = osdfMetadata.getFrameType();
  EXPECT_EQUAL(frameType, expectedFrameType);

  // Channel numbers
  const std::vector<int> expectedChannelNumbers =
      testConfig.getIntVector("channel numbers");
  const std::vector<int> channelNumbers = osdfMetadata.getChanNums();
  EXPECT_EQUAL(channelNumbers, expectedChannelNumbers);

  // Vars with channels
  const std::vector<std::string> configVarsWithChans =
      testConfig.getStringVector("variables with channels");
  const std::unordered_set<std::string> expectedVarsWithChannels(
      configVarsWithChans.begin(), configVarsWithChans.end());
  const std::unordered_set<std::string> varsWithChannels = osdfMetadata.getVarsWithChans();
  EXPECT(varsWithChannels == expectedVarsWithChannels);

  // Number of vars
  const int expectedNumVars = testConfig.getInt("number of variables");
  const int numVars = osdfMetadata.getNumVars();
  EXPECT_EQUAL(numVars, expectedNumVars);

  // Date time epoch
  const std::string expectedDateTimeEpoch = testConfig.getString("date time epoch");
  const std::string dateTimeEpoch = osdfMetadata.getDateTimeEpoch();
  EXPECT_EQUAL(dateTimeEpoch, expectedDateTimeEpoch);
}

// -----------------------------------------------------------------------------
void testFrameRows() {
  // Configuration contains a list of subconfigs that each contain a file name,
  // start location index, and a location count
  const std::vector<eckit::LocalConfiguration> loadConfigs =
      ::test::TestEnvironment::config().getSubConfigurations("obs types");
  const eckit::LocalConfiguration timeWindowConfig =
      ::test::TestEnvironment::config().getSubConfiguration("time window");

  for (auto & config : loadConfigs) {
    // Create parameters for obsdatain and io pool
    oops::Log::info() << "testFrameRows: config = " << config << std::endl;
    const eckit::LocalConfiguration obsDataInConfig = config.getSubConfiguration("obsdatain");
    const eckit::LocalConfiguration testConfig = config.getSubConfiguration("test data");
    ioda::ObsDataInParameters dataInParams;
    dataInParams.deserialize(obsDataInConfig);

    const eckit::LocalConfiguration ioPoolConfig = config.getSubConfiguration("io pool");
    ioda::IoPool::IoPoolParameters ioPoolParams;
    ioPoolParams.validateAndDeserialize(ioPoolConfig);

    // Create a row-oriented OSDF
    std::unique_ptr<osdf::IFrame> testOsdf = std::make_unique<osdf::FrameRows>();

    // Collectively call the loadObs function with all io pool members
    // Need to set the frame type in the osdf metadata before calling loadObs.
    // In normal usage, the obs space would set the frame type prior to calling loadObs.
    osdf::FrameMetadata osdfMetadata;
    osdfMetadata.setFrameType("FrameRows");
    const eckit::mpi::Comm & commAll = oops::mpi::world();
    reader::loadObs(dataInParams, ioPoolParams, commAll, testOsdf, osdfMetadata);
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

  for (auto & config : loadConfigs) {
    // Create parameters for obsdatain and io pool
    oops::Log::info() << "testFrameCols: config = " << config << std::endl;
    const eckit::LocalConfiguration obsDataInConfig = config.getSubConfiguration("obsdatain");
    const eckit::LocalConfiguration testConfig = config.getSubConfiguration("test data");
    ioda::ObsDataInParameters dataInParams;
    dataInParams.deserialize(obsDataInConfig);

    const eckit::LocalConfiguration ioPoolConfig = config.getSubConfiguration("io pool");
    ioda::IoPool::IoPoolParameters ioPoolParams;
    ioPoolParams.validateAndDeserialize(ioPoolConfig);

    // Create a column-oriented OSDF
    std::unique_ptr<osdf::IFrame> testOsdf = std::make_unique<osdf::FrameCols>();

    // Collectively call the loadObs function with all io pool members
    // Need to set the frame type in the osdf metadata before calling loadObs.
    // In normal usage, the obs space would set the frame type prior to calling loadObs.
    osdf::FrameMetadata osdfMetadata;
    osdfMetadata.setFrameType("FrameCols");
    const eckit::mpi::Comm & commAll = oops::mpi::world();
    reader::loadObs(dataInParams, ioPoolParams, commAll, testOsdf, osdfMetadata);
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
