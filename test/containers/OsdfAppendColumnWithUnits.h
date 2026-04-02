/*
 * (C) Crown copyright 2026, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#pragma once

#include <memory>
#include <sstream>
#include <string>
#include <vector>

#define ECKIT_TESTING_SELF_REGISTER_CASES 0

#include <boost/noncopyable.hpp>

#include "eckit/exception/Exceptions.h"
#include "eckit/testing/Test.h"
#include "ioda/containers/FrameCols.h"
#include "ioda/containers/FrameRows.h"
#include "ioda/containers/FrameUtils.h"
#include "ioda/containers/IFrame.h"
#include "ioda/test/containers/OsdfTestUtils.h"
#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"
#include "oops/util/missingValues.h"

namespace ioda {
namespace test {

template <typename T>
void testColumn(const std::string& columnName, const std::vector<T>& values,
                std::unique_ptr<osdf::IFrame>& testFrame, const std::int8_t expectedTypeEnum,
                const std::string& expectedUnits) {
  EXPECT(!testFrame->hasColumn(columnName));

  std::size_t numCols = testFrame->numCols();
  testFrame->appendNewColumn(columnName, values, expectedUnits);
  EXPECT(numCols+1 == testFrame->numCols());

  EXPECT(testFrame->hasColumn(columnName));
  EXPECT(testFrame->getColumnType(columnName) == expectedTypeEnum);
  EXPECT(testFrame->getColumnUnits(columnName) == expectedUnits);

  std::vector<T> testValues;
  testFrame->getColumn(columnName, testValues);
  EXPECT(testValues == values);
}

void testOsdfColumnUnits(std::unique_ptr<osdf::IFrame>& testFrame) {
  std::vector<char> testCharValues{'a', 'b', 'c'};
  std::vector<std::int64_t> testInt64Values;
  std::vector<float> testFloatValues;

  std::vector<std::string> expectedColumnNames;  // for testing columnNames() function

  // String column type
  const std::string stringColumnName = "test string column";
  const std::string stringUnits = "stringUnits";
  testColumn<std::string>(stringColumnName, {"string1", "string2", "string3"},
                          testFrame, osdf::consts::eString, stringUnits);
  expectedColumnNames.push_back(stringColumnName);
  EXPECT(testFrame->columnNames() == expectedColumnNames);

  // int column type
  const std::string intColumnName = "test int column";
  const std::string intUnits = "intUnits";
  testColumn<int>(intColumnName, {1, 2, 3}, testFrame,
                  osdf::consts::eInt, intUnits);
  expectedColumnNames.push_back(intColumnName);
  EXPECT(testFrame->columnNames() == expectedColumnNames);

  // int64 column type
  const std::string int64ColumnName = "test int64 column";
  const std::vector<std::int64_t> origInt64Values{9000000000000000000, 9000000000000000001,
                                                  9000000000000000002};
  const std::string blankUnits = "";
  testColumn<std::int64_t>(int64ColumnName, origInt64Values, testFrame,
                               osdf::consts::eInt64, blankUnits);
  expectedColumnNames.push_back(int64ColumnName);
  EXPECT(testFrame->columnNames() == expectedColumnNames);

  // float column type
  const std::string floatColumnName = "test float column";
  testColumn<float>(floatColumnName, {1.1f, 2.2f, 3.3f}, testFrame,
                        osdf::consts::eFloat, blankUnits);
  expectedColumnNames.push_back(floatColumnName);
  EXPECT(testFrame->columnNames() == expectedColumnNames);

  // char column type
  const std::string charColumnName = "test char column";
  const std::string charUnits = "charUnits";
  testColumn<char>(charColumnName, {'a', 'b', 'c'}, testFrame,
                       osdf::consts::eChar, charUnits);
  expectedColumnNames.push_back(charColumnName);
  EXPECT(testFrame->columnNames() == expectedColumnNames);
}

void testFrameColsColumnUnits() {
  std::unique_ptr<osdf::IFrame> testFrame = std::make_unique<osdf::FrameCols>();
  testOsdfColumnUnits(testFrame);
}

void testFrameRowsColumnUnits() {
  std::unique_ptr<osdf::IFrame> testFrame = std::make_unique<osdf::FrameRows>();
  testOsdfColumnUnits(testFrame);
}

// -----------------------------------------------------------------------------
class OsdfAppendColumnWithUnits : public oops::Test {
 public:
  OsdfAppendColumnWithUnits() {}
  virtual ~OsdfAppendColumnWithUnits() {}

 private:
  std::string testid() const override { return "test::OsdfAppendColumnWithUnits"; }

  void register_tests() const override {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    ts.emplace_back(
      CASE("ioda/ReaderFilter/testFrameColsAppendColumnWithUnits") { testFrameColsColumnUnits(); });
    ts.emplace_back(
      CASE("ioda/ReaderFilter/testFrameRowsAppendColumnWithUnits") { testFrameRowsColumnUnits(); });
  }

  void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda
