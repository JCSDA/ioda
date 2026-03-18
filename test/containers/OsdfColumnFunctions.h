/*
 * (C) Copyright 2026 UCAR
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

// This test is designed to test several column-related functions of the osdf::IFrame interface,
// using both the FrameCols and FrameRows implementations of that interface. It tests the
// appendNewColumn(),
// getColumn(),
// setColumn(),
// hasColumn(),
// getColumnType(),
// getColumnUnits(),
// and columnNames()
// functions, for all the supported column types.

template<typename T>
void testColumnType(const std::string& columnName, const std::vector<T>& origValues,
                    const std::vector<T>& newValues, std::unique_ptr<osdf::IFrame>& testFrame,
                    const std::int8_t expectedTypeEnum, const std::string & expectedUnits) {
  EXPECT(!testFrame->hasColumn(columnName));
  testFrame->appendNewColumn(columnName, origValues);
  EXPECT(testFrame->hasColumn(columnName));
  EXPECT(testFrame->getColumnType(columnName) == expectedTypeEnum);
  std::vector<T> testValues;
  testFrame->getColumn(columnName, testValues);
  EXPECT(testValues == origValues);
  testFrame->setColumn(columnName, newValues);
  testFrame->getColumn(columnName, testValues);
  EXPECT(testValues == newValues);
  EXPECT(testFrame->getColumnUnits(columnName) == expectedUnits);
}

void testOsdfColumnFunctions(std::unique_ptr<osdf::IFrame> & testFrame) {
  const std::vector<char> origCharValues{'a', 'b', 'c'};
  const std::vector<char> newCharValues{'d', 'e', 'b'};

  std::vector<std::int64_t> testInt64Values;
  std::vector<float> testFloatValues;
  std::vector<char> testCharValues;

  std::vector<std::string> expectedColumnNames;  // for testing columnNames() function

  // For now let the units be set using the defaut which is the JEDI missing string value.
  const std::string defaultUnits(util::missingValue<std::string>());

  // String column type
  const std::string stringColumnName = "test string column";
  testColumnType<std::string>(stringColumnName, {"string1", "string2", "string3"},
                            {"string4", "string5", "string2"}, testFrame, osdf::consts::eString,
                            defaultUnits);
  expectedColumnNames.push_back(stringColumnName);
  EXPECT(testFrame->columnNames() == expectedColumnNames);

  // int column type
  const std::string intColumnName = "test int column";
  const std::vector<int> newIntValues{4, 5, 2};
  testColumnType<int>(intColumnName, {1, 2, 3}, newIntValues, testFrame, osdf::consts::eInt,
                      defaultUnits);
  expectedColumnNames.push_back(intColumnName);
  EXPECT(testFrame->columnNames() == expectedColumnNames);

  // int64 column type
  const std::string int64ColumnName = "test int64 column";
  const std::vector<std::int64_t> origInt64Values{9000000000000000000, 9000000000000000001,
                                            9000000000000000002};
  const std::vector<std::int64_t> newInt64Values{9000000000000000003, 9000000000000000004,
                                           9000000000000000005};
  testColumnType<std::int64_t>(int64ColumnName, origInt64Values, newInt64Values, testFrame,
    osdf::consts::eInt64, defaultUnits);
  expectedColumnNames.push_back(int64ColumnName);
  EXPECT(testFrame->columnNames() == expectedColumnNames);

  // float column type
  const std::string floatColumnName = "test float column";
  testColumnType<float>(floatColumnName, {1.1f, 2.2f, 3.3f}, {4.4f, 5.5f, 2.2f}, testFrame,
    osdf::consts::eFloat, defaultUnits);
  expectedColumnNames.push_back(floatColumnName);
  EXPECT(testFrame->columnNames() == expectedColumnNames);

  // char column type
  const std::string charColumnName = "test char column";
  testColumnType<char>(charColumnName, {'a', 'b', 'c'}, {'d', 'e', 'b'}, testFrame,
    osdf::consts::eChar, defaultUnits);
  expectedColumnNames.push_back(charColumnName);
  EXPECT(testFrame->columnNames() == expectedColumnNames);

  // Test expected setColumnerror conditions
  // Wrong type
  EXPECT_THROWS_AS(testFrame->setColumn(stringColumnName, newIntValues),
                    eckit::BadParameter);
  // Wrong size
  const std::vector<int> wrongSizeIntValues{4, 5};
  EXPECT_THROWS_AS(testFrame->setColumn(intColumnName, wrongSizeIntValues),
                    eckit::BadParameter);
  // Read-only column
  // TODO(vahl): Finish test of read-only column functionality once it is more fully implemented.
  // Currently there is no way to create a read-only column using the IFrame interface.
  // testFrame->appendNewColumn("test read-only column", origIntValues);,
  //   osdf::consts::eReadOnly);
  // EXPECT_THROWS_AS(testFrame->setColumn("test read-only column", newIntValues),
  //                   eckit::BadParameter);
}

void testFrameColsColumnFunctions() {
  std::unique_ptr<osdf::IFrame> testFrame = std::make_unique<osdf::FrameCols>();
  testOsdfColumnFunctions(testFrame);
}

void testFrameRowsColumnFunctions() {
  std::unique_ptr<osdf::IFrame> testFrame = std::make_unique<osdf::FrameRows>();
  testOsdfColumnFunctions(testFrame);
}

// -----------------------------------------------------------------------------
class OsdfColumnFunctions : public oops::Test {
 public:
  OsdfColumnFunctions() {}
  virtual ~OsdfColumnFunctions() {}

 private:
  std::string testid() const override {return "test::OsdfColumnFunctions";}

  void register_tests() const override {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    ts.emplace_back(CASE("ioda/ReaderFilter/testFrameColsColumnFunctions")
      { testFrameColsColumnFunctions(); });
    ts.emplace_back(CASE("ioda/ReaderFilter/testFrameRowsColumnFunctions")
      { testFrameRowsColumnFunctions(); });
  }

  void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda
