/*
 * (C) Copyright 2025 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#pragma once

#include <cstddef>
#include <memory>
#include <sstream>
#include <string>
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
#include "ioda/reader/filter/filterObs.hpp"
#include "ioda/reader/ObsReader.hpp"
#include "ioda/test/containers/OsdfTestUtils.h"

#include "oops/mpi/mpi.h"
#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"
#include "oops/util/Logger.h"
#include "oops/util/TimeWindow.h"

namespace ioda {
namespace test {

/// \brief global number of rows in a frame
std::size_t globalNumRows(const osdf::IFrame & osdf) {
  std::size_t numRows = 0;
  oops::mpi::world().allReduce(osdf.numRows(), numRows, eckit::mpi::sum());
  return numRows;
}

/// \brief check the filtered frame and the filter counts against the expected obs source stats
/// \param configObsSourceStats expected obs source stats
/// \param sourceNlocs global number of locations before filtering
/// \param filteredOsdf frame after filtering
/// \param filterCounts counts returned by filterObs
void compareObsSourceStats(const eckit::LocalConfiguration & configObsSourceStats,
                           const std::size_t sourceNlocs,
                           const osdf::IFrame & filteredOsdf,
                           const reader::FilterCounts & filterCounts) {
  const std::size_t expectedSourceNlocs = configObsSourceStats.getUnsigned("source nlocs");
  const std::size_t expectedNlocs = configObsSourceStats.getUnsigned("nlocs");
  const std::size_t expectedGnlocs = configObsSourceStats.getUnsigned("gnlocs");
  const std::size_t expectedGnlocsOutsideTimewindow =
    configObsSourceStats.getUnsigned("gnlocs outside timewindow");
  const std::size_t expectedGnlocsRejectQc =
    configObsSourceStats.getUnsigned("gnlocs reject qc");
  const std::vector<std::size_t> expectedLocIndices =
    configObsSourceStats.getUnsignedVector("loc indices");

  const std::size_t nlocs = filteredOsdf.numRows();
  const std::size_t gNlocs = globalNumRows(filteredOsdf);

  EXPECT_EQUAL(sourceNlocs, expectedSourceNlocs);
  EXPECT_EQUAL(nlocs, expectedNlocs);
  EXPECT_EQUAL(gNlocs, expectedGnlocs);
  EXPECT_EQUAL(filterCounts.gNlocsOutsideTimewindow, expectedGnlocsOutsideTimewindow);
  EXPECT_EQUAL(filterCounts.gNlocsRejectQc, expectedGnlocsRejectQc);
  EXPECT_EQUAL(reader::getSourceLocationIndices(filteredOsdf), expectedLocIndices);
  // Every location is either kept or counted by one of the two checks.
  EXPECT_EQUAL(gNlocs + filterCounts.gNlocsOutsideTimewindow + filterCounts.gNlocsRejectQc,
               sourceNlocs);
}

/// \brief collect the test configuration for a single test case
/// \details Each test case does the same collection of test configuration
/// which includes the time window specs, data for populating the test OSDF
/// object and the expected results.
/// \param testConfig Total configuration for this test
/// \param timeWindowConfig config for constructing the time window
/// \param columnDataConfig config for populating the test OSDF object
/// \param refDataConfig expected data for filtered data
/// \param refObsSrcStatsConfig expected data for obs source stats
void collectTestConfig(const eckit::Configuration & testConfig,
                       eckit::LocalConfiguration & timeWindowConfig,
                       std::vector<eckit::LocalConfiguration> & columnDataConfig,
                       std::vector<eckit::LocalConfiguration> & refDataConfig,
                       eckit::LocalConfiguration & refObsSrcStatsConfig) {
  timeWindowConfig = testConfig.getSubConfiguration("time window");
  columnDataConfig = testConfig.getSubConfigurations("test column data");
  refDataConfig = testConfig.getSubConfigurations("expected filtered data");
  refObsSrcStatsConfig = testConfig.getSubConfiguration("expected obs source stats data");
}

void testFilter(const std::string & frameType) {
  eckit::LocalConfiguration timeWinConfig;
  std::vector<eckit::LocalConfiguration> configColumnData;
  std::vector<eckit::LocalConfiguration> configRefData;
  eckit::LocalConfiguration configObsSourceStats;
  collectTestConfig(::test::TestEnvironment::config(), timeWinConfig, configColumnData,
                    configRefData, configObsSourceStats);

  const util::TimeWindow timeWindow(timeWinConfig);
  const double tolerance = ::test::TestEnvironment::config().getDouble("tolerance");

  // Create an instance of a row priority data frame and populate it with
  // test data from the config file.
  std::unique_ptr<osdf::IFrame> testOsdf;
  if (frameType == "FrameRows") {
    testOsdf = std::make_unique<osdf::FrameRows>();
  } else if (frameType == "FrameCols") {
    testOsdf = std::make_unique<osdf::FrameCols>();
  } else {
    throw eckit::BadParameter("Unknown frame type: " + frameType, Here());
  }
  oops::Log::info() << "Testing OSDF of type: " << testOsdf->frameType() << std::endl;
  std::vector<std::string> testColumnNames;
  std::vector<std::string> testColumnTypes;
  populateFrame(configColumnData, testOsdf, testColumnNames, testColumnTypes);

  // Read in the expected results after filtering
  std::unique_ptr<osdf::IFrame> refOsdf = std::make_unique<osdf::FrameRows>();
  std::vector<std::string> refColumnNames;
  std::vector<std::string> refColumnTypes;
  populateFrame(configRefData, refOsdf, refColumnNames, refColumnTypes);

  // Run the filters and check the results
  const std::size_t sourceNlocs = globalNumRows(*testOsdf);
  osdf::FrameMetadata osdfMetadata;
  const reader::FilterCounts filterCounts =
    reader::filterObs(timeWindow, oops::mpi::world(), testOsdf, osdfMetadata);
  oops::Log::info() << "testFrameRows: after filtering" << std::endl;
  compareFrames(testOsdf, testColumnNames, testColumnTypes, refOsdf, refColumnNames, refColumnTypes,
    tolerance, false);

  // Check the relevant obs source stats
  compareObsSourceStats(configObsSourceStats, sourceNlocs, *testOsdf, filterCounts);
}

// -----------------------------------------------------------------------------
// With the location checks switched off, every row survives and nothing is reported as
// rejected. The input frame is its own reference here: a generated obs source is handed
// the locations to produce, so the filter must hand them all back.
void testChecksDisabled() {
  eckit::LocalConfiguration timeWinConfig;
  std::vector<eckit::LocalConfiguration> configColumnData;
  std::vector<eckit::LocalConfiguration> configRefData;
  eckit::LocalConfiguration configObsSourceStats;
  collectTestConfig(::test::TestEnvironment::config(), timeWinConfig, configColumnData,
                    configRefData, configObsSourceStats);

  const util::TimeWindow timeWindow(timeWinConfig);
  const double tolerance = ::test::TestEnvironment::config().getDouble("tolerance");

  std::unique_ptr<osdf::IFrame> testOsdf = std::make_unique<osdf::FrameRows>();
  std::vector<std::string> testColumnNames;
  std::vector<std::string> testColumnTypes;
  populateFrame(configColumnData, testOsdf, testColumnNames, testColumnTypes);

  // Same data again, left untouched, to compare against.
  std::unique_ptr<osdf::IFrame> refOsdf = std::make_unique<osdf::FrameRows>();
  std::vector<std::string> refColumnNames;
  std::vector<std::string> refColumnTypes;
  populateFrame(configColumnData, refOsdf, refColumnNames, refColumnTypes);
  const std::size_t numInputRows = refOsdf->numRows();

  osdf::FrameMetadata osdfMetadata;
  const bool applyLocationChecks = false;
  const reader::FilterCounts filterCounts =
    reader::filterObs(timeWindow, oops::mpi::world(), testOsdf, osdfMetadata,
                      applyLocationChecks);
  oops::Log::info() << "testChecksDisabled: after filtering" << std::endl;
  compareFrames(testOsdf, testColumnNames, testColumnTypes, refOsdf, refColumnNames,
    refColumnTypes, tolerance, false);

  EXPECT_EQUAL(testOsdf->numRows(), numInputRows);
  EXPECT_EQUAL(filterCounts.gNlocsOutsideTimewindow, 0);
  EXPECT_EQUAL(filterCounts.gNlocsRejectQc, 0);
}

// -----------------------------------------------------------------------------
class ReaderFilter : public oops::Test {
 public:
  ReaderFilter() {}
  virtual ~ReaderFilter() {}

 private:
  std::string testid() const override {return "test::ReaderFilter";}

  void register_tests() const override {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    ts.emplace_back(CASE("ioda/ReaderFilter/testFrameRows")
      { testFilter("FrameRows"); });
    ts.emplace_back(CASE("ioda/ReaderFilter/testFrameCols")
      { testFilter("FrameCols"); });
    ts.emplace_back(CASE("ioda/ReaderFilter/testChecksDisabled")
      { testChecksDisabled(); });
  }

  void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda
