/*
 * (C) Copyright 2026, UCAR
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
#include "ioda/containers/DataRow.h"
#include "ioda/containers/FrameCols.h"
#include "ioda/test/containers/OsdfTestUtils.h"

#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"
#include "oops/util/Logger.h"

namespace ioda {
namespace test {

// -----------------------------------------------------------------------------
// Stored row ids in row order. FrameCols keeps its ids outside of the data rows (getDataRows()
// labels rows by position), so read them directly.
std::vector<std::int64_t> getRowIds(const std::unique_ptr<osdf::IFrame> & frame) {
  if (const auto * frameCols = dynamic_cast<const osdf::FrameCols *>(frame.get())) {
    return frameCols->getData().getIds();
  }
  std::vector<osdf::DataRow> dataRows;
  dataRows.reserve(frame->numRows());
  frame->getData().getDataRows(dataRows);
  std::vector<std::int64_t> ids;
  ids.reserve(dataRows.size());
  for (const osdf::DataRow & dataRow : dataRows) {
    ids.push_back(dataRow.getId());
  }
  return ids;
}

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
    const std::vector<std::int64_t> idsBefore = getRowIds(testFrame);
    std::vector<std::int64_t> expectedIds;
    for (std::size_t i = 0; i < keepRows.size(); ++i) {
      if (keepRows[i]) expectedIds.push_back(idsBefore[i]);
    }
    testFrame->removeRows(keepRows);
    compareFrames(testFrame, originalColumnNames, originalColumnTypes,
                  expectedFrame2, expected2ColumnNames, expected2ColumnTypes, tolerance, 1);
    // Row ids of the kept rows are preserved
    EXPECT(getRowIds(testFrame) == expectedIds);
  }
}

// -----------------------------------------------------------------------------
void testRemoveRowsEdgeCases(std::string frameType) {
  const std::vector<eckit::LocalConfiguration> & testCasesConfig =
    ::test::TestEnvironment::config().getSubConfigurations("test cases");
  double tolerance = ::test::TestEnvironment::config().getDouble("tolerance", 1e-6);
  for (std::size_t jj = 0; jj < testCasesConfig.size(); ++jj) {
    eckit::LocalConfiguration testCaseConfig = testCasesConfig[jj];
    oops::Log::info() << "Testing edge cases: " << testCaseConfig.getString("name") << std::endl;
    std::vector<eckit::LocalConfiguration> originalConfig =
      testCaseConfig.getSubConfigurations("original columns");

    std::unique_ptr<osdf::IFrame> testFrame = osdf::createIFrame(frameType);
    std::vector<std::string> columnNames;
    std::vector<std::string> columnTypes;
    populateFrame(originalConfig, testFrame, columnNames, columnTypes);
    std::unique_ptr<osdf::IFrame> originalFrame = osdf::createIFrame(frameType);
    populateFrame(originalConfig, originalFrame, columnNames, columnTypes);
    const std::size_t numRows = testFrame->numRows();
    const std::size_t numCols = testFrame->numCols();
    const std::vector<std::int64_t> originalIds = getRowIds(testFrame);

    // Mask of the wrong size
    EXPECT_THROWS_AS(testFrame->removeRows(std::vector<bool>(numRows + 1, true)),
                     eckit::Exception);

    // Keep all rows: frame is unchanged
    testFrame->removeRows(std::vector<bool>(numRows, true));
    EXPECT(testFrame->numRows() == numRows);
    compareFrames(testFrame, columnNames, columnTypes,
                  originalFrame, columnNames, columnTypes, tolerance, true);
    EXPECT(getRowIds(testFrame) == originalIds);

    // Read-only column: removing rows throws and leaves the frame unchanged,
    // but a mask that removes nothing is still accepted
    std::unique_ptr<osdf::IFrame> readOnlyFrame = osdf::createIFrame(frameType);
    populateFrame(originalConfig, readOnlyFrame, columnNames, columnTypes);
    readOnlyFrame->configColumns(
      {{"MetaData/readOnlyVariable", osdf::consts::eInt, osdf::consts::eReadOnly}});
    std::vector<bool> keepRows(numRows, true);
    keepRows[0] = false;
    EXPECT_THROWS_AS(readOnlyFrame->removeRows(keepRows), eckit::BadParameter);
    EXPECT(readOnlyFrame->numRows() == numRows);
    compareFrames(readOnlyFrame, columnNames, columnTypes,
                  originalFrame, columnNames, columnTypes, tolerance, true);
    readOnlyFrame->removeRows(std::vector<bool>(numRows, true));
    EXPECT(readOnlyFrame->numRows() == numRows);

    // Remove all rows: columns are retained but empty
    testFrame->removeRows(std::vector<bool>(numRows, false));
    EXPECT(testFrame->numRows() == 0);
    EXPECT(testFrame->numCols() == numCols);
    EXPECT(testFrame->columnNames() == originalFrame->columnNames());
    EXPECT(getRowIds(testFrame).empty());
    for (std::size_t i = 0; i < columnNames.size(); ++i) {
      std::string errMsg = std::string("Unrecognized data type: ") + columnTypes[i];
      callWithSupportedTypeString(columnTypes[i], errMsg, [&](auto typeDiscriminator) {
        using T = decltype(typeDiscriminator);
        std::vector<T> values;
        testFrame->getColumn(columnNames[i], values);
        EXPECT(values.empty());
      });
    }
  }
}


void testFrameColsRemoveRows() {
  testRemoveRows("FrameCols");
}

void testFrameRowsRemoveRows() {
  testRemoveRows("FrameRows");
}

void testFrameColsRemoveRowsEdgeCases() {
  testRemoveRowsEdgeCases("FrameCols");
}

void testFrameRowsRemoveRowsEdgeCases() {
  testRemoveRowsEdgeCases("FrameRows");
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
    ts.emplace_back(CASE("ioda/OsdfRemoveRows/testFrameColsEdgeCases")
      { testFrameColsRemoveRowsEdgeCases(); });
    ts.emplace_back(CASE("ioda/OsdfRemoveRows/testFrameRowsEdgeCases")
      { testFrameRowsRemoveRowsEdgeCases(); });
  }

  void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda
