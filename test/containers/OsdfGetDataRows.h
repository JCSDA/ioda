/*
 * (C) Crown copyright 2026, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#pragma once

#include <string>
#include <vector>

#define ECKIT_TESTING_SELF_REGISTER_CASES 0

#include "ioda/containers/FrameCols.h"
#include "ioda/containers/FrameRows.h"
#include "ioda/test/containers/OsdfTestUtils.h"
#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"
#include "oops/util/Logger.h"

namespace ioda {
namespace test {

std::int8_t testCompareTwoDataRows(osdf::DataRow testRow, osdf::DataRow compareRow) {
  const std::int32_t testSize    = testRow.getSize();
  const std::int32_t compareSize = compareRow.getSize();

  if (testSize != compareSize) {
    return false;
  }

  for (std::int32_t index = 0; index < testSize; ++index) {
    std::string testString    = testRow.getColumn(index)->getValueStr();
    std::string compareString = compareRow.getColumn(index)->getValueStr();
    if (testString != compareString) {
      return false;
    }
  }
  return true;
}

// Compares getDataRow function from FrameCols to that from FrameRows
void testGetDataRow(osdf::FrameCols& testFrame, std::int32_t testRowIndex,
                    std::int32_t compareRowIndex, std::int8_t isExpectRowsEqual) {
  osdf::DataRow testRow = testFrame.getData().getDataRow(testRowIndex);
  osdf::FrameRows compareFrameRows(testFrame);
  osdf::DataRow compareRow = testFrame.getData().getDataRow(compareRowIndex);

  oops::Log::info() << "FrameCol index " + std::to_string(testRowIndex) + ", FrameRow index "
                         + std::to_string(compareRowIndex) + ", Expect equal "
                         + std::to_string(isExpectRowsEqual)
                    << std::endl;
  EXPECT_EQUAL(testCompareTwoDataRows(testRow, compareRow), isExpectRowsEqual);
}

// Compares getDataRows function from frameCols to getDataRow function from same frameCols
void testGetDataRowsFrameCol(osdf::FrameCols& testFrame, std::int32_t testRowIndex,
                     std::int32_t compareRowIndex, std::int8_t isExpectRowsEqual) {
  std::vector<osdf::DataRow> testRows;
  testRows.reserve(testFrame.getData().getSizeRows());
  testFrame.getData().getDataRows(testRows);

  osdf::DataRow testRow = testRows[testRowIndex];
  osdf::DataRow compareRow = testFrame.getData().getDataRow(compareRowIndex);

  oops::Log::info() << "getDataRows index " + std::to_string(testRowIndex) + ", getDataRow index "
                         + std::to_string(compareRowIndex) + ", Expect equal "
                         + std::to_string(isExpectRowsEqual)
                    << std::endl;
  EXPECT_EQUAL(testCompareTwoDataRows(testRow, compareRow), isExpectRowsEqual);
}

// Compares getDataRows function from frameRows to getDataRow function from same frameRows
void testGetDataRowsFrameRow(osdf::FrameRows& testFrame, std::int32_t testRowIndex,
                     std::int32_t compareRowIndex, std::int8_t isExpectRowsEqual) {
  std::vector<osdf::DataRow> testRows;
  testRows.reserve(testFrame.getData().getSizeRows());
  testFrame.getData().getDataRows(testRows);

  osdf::DataRow testRow    = testRows[testRowIndex];
  osdf::DataRow compareRow = testFrame.getData().getDataRow(compareRowIndex);

  oops::Log::info() << "getDataRows index " + std::to_string(testRowIndex) + ", getDataRow index "
                         + std::to_string(compareRowIndex) + ", Expect equal "
                         + std::to_string(isExpectRowsEqual)
                    << std::endl;
  EXPECT_EQUAL(testCompareTwoDataRows(testRow, compareRow), isExpectRowsEqual);
}

void testGetDataRow_DoTest() {
  std::vector<float> lats          = {-65.0, -66.6};
  std::vector<std::string> statIds = {"00001", "00001"};
  std::vector<std::int64_t> times  = {1710460225, 1710460225};

  osdf::FrameCols testFrameCols;
  testFrameCols.appendNewColumn("lat", lats);
  testFrameCols.appendNewColumn("StatId", statIds);
  testFrameCols.appendNewColumn("time", times);

  testGetDataRow(testFrameCols, 0, 0, 1);
  testGetDataRow(testFrameCols, 0, 1, 0);
}

void testGetDataRows_DoTest() {
  std::vector<float> lats          = {-65.0, -66.6};
  std::vector<std::string> statIds = {"00001", "00001"};
  std::vector<std::int64_t> times  = {1710460225, 1710460225};

  // Test frameCols
  osdf::FrameCols frameCols1;
  frameCols1.appendNewColumn("lat", lats);
  frameCols1.appendNewColumn("StatId", statIds);
  frameCols1.appendNewColumn("time", times);
  testGetDataRowsFrameCol(frameCols1, 0, 0, 1);

  // Test frameRows
  osdf::FrameRows frameRows1;
  frameRows1.appendNewColumn("lat", lats);
  frameRows1.appendNewColumn("StatId", statIds);
  frameRows1.appendNewColumn("time", times);
  testGetDataRowsFrameRow(frameRows1, 0, 0, 1);
}

// -----------------------------------------------------------------------------
class OsdfGetDataRows : public oops::Test {
 public:
  OsdfGetDataRows() {}
  virtual ~OsdfGetDataRows() {}

 private:
  std::string testid() const override { return "test::OsdfGetDataRows"; }

  void register_tests() const override {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    ts.emplace_back(CASE("ioda/ReaderFilter/testGetDataRow") { testGetDataRow_DoTest(); });
    ts.emplace_back(CASE("ioda/ReaderFilter/testGetDataRowsIFrame") { testGetDataRows_DoTest(); });
  }

  void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda
