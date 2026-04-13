/*
 * (C) Crown copyright 2026, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#define ECKIT_TESTING_SELF_REGISTER_CASES 0

#include "ioda/containers/CreateIFrame.h"
#include "ioda/containers/FrameCols.h"
#include "ioda/containers/FrameRows.h"
#include "ioda/test/containers/OsdfTestUtils.h"
#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"
#include "oops/util/Logger.h"

namespace ioda {
namespace test {

void testOsdfAppendNewRowEmpty() {
  oops::Log::info() << "Running test: Empty IFrame" << std::endl;
  osdf::FrameRows refEmptyFrameRows;
  osdf::FrameCols refEmptyFrameCols;

  EXPECT_THROWS_AS(
    refEmptyFrameRows.appendNewRow(-25.6568f, "00010", static_cast<int64_t>(1710460270), 11, 'd'),
    eckit::BadValue);
  EXPECT_THROWS_AS(
    refEmptyFrameCols.appendNewRow(-25.6568f, "00010", static_cast<int64_t>(1710460270), 11, 'd'),
    eckit::BadValue);
}

template <typename T>
void testOsdfAppendNewRowFrame(const std::vector<eckit::LocalConfiguration>& osdfConfig,
                               const double tolerance) {
  // Construct framerows from yaml
  T testFrame;
  std::vector<std::string> columnNamesFrame;
  std::vector<std::string> columnTypesFrame;
  populateFrame(osdfConfig, testFrame, columnNamesFrame, columnTypesFrame);

  // Store column metadata
  osdf::ColumnMetadata testFrameColumnMetadata = testFrame.getData().getColumnMetadata();

  // Test pass if adding in correct format
  std::size_t numRows = testFrame.numRows();
  std::size_t maxId   = testFrame.getData().getMaxId();
  testFrame.appendNewRow(-25.6568f, "00010", static_cast<int64_t>(1710460270), 11, 'd');
  EXPECT_EQUAL(testFrame.numRows(), numRows + 1);
  EXPECT_EQUAL(testFrame.getData().getMaxId(), maxId + 1);

  T refFrame;
  const std::vector<eckit::LocalConfiguration>& testRowConfig
    = ::test::TestEnvironment::config().getSubConfigurations("test row append");
  populateFrame(testRowConfig, refFrame, columnNamesFrame, columnTypesFrame);
  testCompareTwoDataRows(testFrame.getData().getDataRow(maxId + 1),
                         refFrame.getData().getDataRow(0), 0, tolerance);

  // Check if appendnewrow correctly updates widths when column in new row is wider
  osdf::ColumnMetadata refFrameColumnMetadata = refFrame.getData().getColumnMetadata();
  osdf::ColumnMetadata updatedTestFrameColumnMetadata = testFrame.getData().getColumnMetadata();

  for (size_t index = 0; index < testFrame.numCols(); ++index) {
    oops::Log::error() << "Checking width of column: " << testFrame.getData().getName(index)
                       << std::endl;
    int16_t refColumnWidth = refFrameColumnMetadata.getWidth(index);
    int16_t initialColumnWidth = testFrameColumnMetadata.getWidth(index);
    int16_t updatedColumnWidth = updatedTestFrameColumnMetadata.getWidth(index);

    if (refColumnWidth > initialColumnWidth) {
      EXPECT_EQUAL(refColumnWidth, updatedColumnWidth);
    } else {
      EXPECT_EQUAL(initialColumnWidth, updatedColumnWidth);
    }
  }

  // Throw if data types don't match
  EXPECT_THROWS_AS(testFrame.appendNewRow(-25.6568f, "00010", 1710460270, 11, 'd'),
                     eckit::BadParameter);
  EXPECT_THROWS_AS(
    testFrame.appendNewRow(-25.6568f, "00010", static_cast<int64_t>(1710460270), 11.2f, 'd'),
    eckit::BadParameter);

  // Throw if more/fewer arguments than number of columns
  EXPECT_THROWS_AS(
    testFrame.appendNewRow(-25.6568f, "00010", static_cast<int64_t>(1710460270), 11, 'd', 20),
    eckit::BadParameter);
  EXPECT_THROWS_AS(
    testFrame.appendNewRow(-25.6568f, "00010", static_cast<int64_t>(1710460270), 11),
    eckit::BadParameter);

  // Throws if config new readonly column and attempt append
  testFrame.configColumns({{"MetaData/eReadOnly", osdf::consts::eInt, osdf::consts::eReadOnly}});
  EXPECT_THROWS_AS(
    testFrame.appendNewRow(-25.6568f, "00010", static_cast<int64_t>(1710460270), 11, 'd', 20),
    eckit::BadParameter);
}


void testOsdfAppendNewRow() {
  const double tolerance = ::test::TestEnvironment::config().getDouble("tolerance");
  const std::vector<eckit::LocalConfiguration>& testCasesConfig
    = ::test::TestEnvironment::config().getSubConfigurations("test frame data");

  for (std::size_t index = 0; index < testCasesConfig.size(); ++index) {
    const eckit::LocalConfiguration& testCaseConfig = testCasesConfig[index];
    std::string testCaseName = testCaseConfig.getString("name");

    oops::Log::info() << "Running test: " << testCaseName << std::endl;

    const std::vector<eckit::LocalConfiguration>& osdfConfig
      = testCaseConfig.getSubConfigurations("osdf columns");

    oops::Log::info() << "Testing FrameRows" << std::endl;
    testOsdfAppendNewRowFrame<osdf::FrameRows>(osdfConfig, tolerance);

    oops::Log::info() << "Testing FrameCols" << std::endl;
    testOsdfAppendNewRowFrame<osdf::FrameCols>(osdfConfig, tolerance);
  }
}

// -----------------------------------------------------------------------------
class OsdfAppendNewRow : public oops::Test {
 public:
  OsdfAppendNewRow() {}
  virtual ~OsdfAppendNewRow() {}

 private:
  std::string testid() const override { return "test::OsdfAppendNewRow"; }

  void register_tests() const override {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    ts.emplace_back(
      CASE("ioda/ObsDataFrame/testAppendNewRowEmpty") { testOsdfAppendNewRowEmpty(); });
    ts.emplace_back(
      CASE("ioda/ObsDataFrame/testAppendNewRow") { testOsdfAppendNewRow(); });
  }

  void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda
