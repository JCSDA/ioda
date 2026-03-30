/*
 * (C) Copyright 2026, UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

#define ECKIT_TESTING_SELF_REGISTER_CASES 0

#include "ioda/containers/CreateIFrame.h"
#include "ioda/test/containers/OsdfTestUtils.h"

#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"
#include "oops/util/Logger.h"

namespace ioda {
namespace test {

// -----------------------------------------------------------------------------
void testRemoveRows(std::string frameType) {
  const std::vector<eckit::LocalConfiguration> & testCasesConfig =
    ::test::TestEnvironment::config().getSubConfigurations("test cases");
  double tolerance = ::test::TestEnvironment::config().getDouble("tolerance", 1e-6);
  for (std::size_t jj = 0; jj < testCasesConfig.size(); ++jj) {
    eckit::LocalConfiguration testCaseConfig = testCasesConfig[jj];
    oops::Log::info() << "Testing: " << testCaseConfig.getString("name") << std::endl;

    std::unique_ptr<osdf::IFrame> testFrame = osdf::createIFrame(frameType);
    std::vector<eckit::LocalConfiguration> originalConfig =
      testCaseConfig.getSubConfigurations("original columns");
    std::vector<std::string> originalColumnNames;
    std::vector<std::string> originalColumnTypes;
    populateFrame(originalConfig, testFrame, originalColumnNames, originalColumnTypes);
    std::vector<eckit::LocalConfiguration> expectedConfig1 =
      testCaseConfig.getSubConfigurations("expected removerow columns");
    std::unique_ptr<osdf::IFrame> expectedFrame1 = osdf::createIFrame(frameType);
    std::vector<std::string> expected1ColumnNames;
    std::vector<std::string> expected1ColumnTypes;
    populateFrame(expectedConfig1, expectedFrame1, expected1ColumnNames, expected1ColumnTypes);

    // Test removing a single row with removeRow
    size_t rowToRemove = testCaseConfig.getInt("row to remove");
    size_t originalNumRows = testFrame->numRows();
    EXPECT_THROWS_AS(testFrame->removeRow(originalNumRows), eckit::OutOfRange);
    testFrame->removeRow(rowToRemove);
    compareFrames(testFrame, originalColumnNames, originalColumnTypes,
                  expectedFrame1, expected1ColumnNames, expected1ColumnTypes, tolerance, true);

    // Test removing a multiple rows with removeRows
    std::vector<eckit::LocalConfiguration> expectedConfig2 =
      testCaseConfig.getSubConfigurations("expected removerows columns");
    std::unique_ptr<osdf::IFrame> expectedFrame2 = osdf::createIFrame(frameType);
    std::vector<std::string> expected2ColumnNames;
    std::vector<std::string> expected2ColumnTypes;
    populateFrame(expectedConfig2, expectedFrame2, expected2ColumnNames, expected2ColumnTypes);
    std::vector<size_t> rowsToRemove = testCaseConfig.getUnsignedVector("rows to remove");
    std::vector<bool> keepRows(originalNumRows - 1, true);
    for (auto rowIndex : rowsToRemove) {
      keepRows[rowIndex] = false;
    }
    testFrame->removeRows(keepRows);
    compareFrames(testFrame, originalColumnNames, originalColumnTypes,
                  expectedFrame2, expected2ColumnNames, expected2ColumnTypes, tolerance, 1);
  }
}


void testFrameColsRemoveRows() {
  testRemoveRows("FrameCols");
}

void testFrameRowsRemoveRows() {
  testRemoveRows("FrameRows");
}

// -----------------------------------------------------------------------------
class OsdfRemoveRows : public oops::Test {
 public:
  OsdfRemoveRows() {}
  virtual ~OsdfRemoveRows() {}

 private:
  std::string testid() const override { return "test::OsdfRemoveRows"; }

  void register_tests() const override {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    ts.emplace_back(CASE("ioda/OsdfRemoveRows/testFrameCols") { testFrameColsRemoveRows(); });
    ts.emplace_back(CASE("ioda/OsdfRemoveRows/testFrameRows") { testFrameRowsRemoveRows(); });
  }

  void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda
