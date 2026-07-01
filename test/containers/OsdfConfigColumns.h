/*
 * (C) Crown copyright 2026, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <memory>
#include <string>
#include <vector>

#include "OsdfTestUtils.h"
#include "eckit/exception/Exceptions.h"
#include "oops/util/missingValues.h"

#define ECKIT_TESTING_SELF_REGISTER_CASES 0

#include "ioda/containers/CreateIFrame.h"
#include "ioda/containers/FrameUtils.h"
#include "oops/runs/Test.h"

namespace ioda {
namespace test {

void testOsdfConfigColumns(std::string frameType) {
  // Initializer list for constructor
  std::initializer_list<osdf::ColumnMetadatum> configInitList
    = {{"testFloatDatum", "testFloatUnit", osdf::consts::eFloat},
       {"testIntDatum", "testIntUnit", osdf::consts::eInt},
       {"testInt64Datum", osdf::consts::eInt64},
       {"testStringDatum", osdf::consts::eString},
       {"testCharDatum", "testCharUnit", osdf::consts::eChar}};

  // Vector for constructor
  std::vector<osdf::ColumnMetadatum> configVectorDatum;
  configVectorDatum.reserve(configInitList.size());
  for (osdf::ColumnMetadatum item : configInitList) {
    configVectorDatum.emplace_back(item);
  }

  // Expected names and types
  std::vector<std::string> expectedColumnNames
    = {"testFloatDatum", "testIntDatum", "testInt64Datum", "testStringDatum", "testCharDatum"};
  std::vector<osdf::consts::eDataTypes> expectedDataTypes
    = {osdf::consts::eFloat, osdf::consts::eInt, osdf::consts::eInt64, osdf::consts::eString,
       osdf::consts::eChar};
  std::vector<std::string> expectedColumnUnits
    = {"testFloatUnit", "testIntUnit", "MISSING*", "MISSING*", "testCharUnit"};

  // Test both config columns constructors
  std::unique_ptr<osdf::IFrame> testFrameVector = osdf::createIFrame(frameType);
  testFrameVector->configColumns(configVectorDatum);
  EXPECT(testFrameVector->columnNames() == expectedColumnNames);
  EXPECT(testFrameVector->numCols() == expectedColumnNames.size());
  EXPECT(testFrameVector->numRows() == 0);

  for (size_t index = 0; index < expectedColumnNames.size(); ++index) {
    EXPECT(testFrameVector->getColumnType(expectedColumnNames[index]) == expectedDataTypes[index]);
    EXPECT(testFrameVector->getColumnUnits(expectedColumnNames[index])
           == expectedColumnUnits[index]);
  }

  std::unique_ptr<osdf::IFrame> testFrameInitList = osdf::createIFrame(frameType);
  testFrameInitList->configColumns(configInitList);
  compareFrames(testFrameInitList, {}, {}, testFrameVector, {}, {}, 1e-6, 1);

  // Expect pass if adding metadatum to existing frame with no data rows
  testFrameVector->configColumns(
    {osdf::ColumnMetadatum({"newTestInt64Datum", osdf::consts::eInt64})});
  EXPECT(testFrameVector->numCols() == expectedColumnNames.size() + 1);
  EXPECT(testFrameVector->numRows() == 0);
  std::vector<std::int64_t> dummyVector;
  testFrameVector->getColumn("newTestInt64Datum", dummyVector);
  EXPECT(dummyVector.empty());

  // Expect failure if adding column which already exists
  EXPECT_THROWS_AS(
    testFrameVector->configColumns({osdf::ColumnMetadatum{"testInt64Datum", osdf::consts::eInt64}}),
    eckit::BadParameter);

  // Expect pass if adding metadatum to existing frame with data rows
  std::unique_ptr<osdf::IFrame> testFilledFrame = osdf::createIFrame(frameType);
  testFilledFrame->appendNewColumn("fullTestStringDatum", {"alpha", "beta", "gamma"});
  std::vector<int> testIntColumn = {1, 2, 3};
  testFilledFrame->appendNewColumn("fullTestIntDatum", testIntColumn);
  std::size_t numRowsBeforeConfig = testFilledFrame->numRows();

  testFilledFrame->configColumns(configVectorDatum);

  // Check newly added columns are correct size and full of missing data
  for (size_t index = 0; index < expectedColumnNames.size(); ++index) {
    osdf::consts::eDataTypes columnType
      = testFilledFrame->getColumnType(expectedColumnNames[index]);
    EXPECT(columnType == expectedDataTypes[index]);
    std::string columnUnit = testFilledFrame->getColumnUnits(expectedColumnNames[index]);
    EXPECT(columnUnit == expectedColumnUnits[index]);

    osdf::FrameUtils::callWithSupportedType(columnType, [&](auto typeDiscriminator) {
      using T = decltype(typeDiscriminator);
      std::vector<T> expectedMissingVector(3);
      testFilledFrame->getColumn(expectedColumnNames[index], expectedMissingVector);
      EXPECT(expectedMissingVector.size() == numRowsBeforeConfig);

      for (T item : expectedMissingVector) {
        EXPECT(item == util::missingValue<T>());
      }
    });
  }
}

void testOsdfConfigColumnsFrameCols() { testOsdfConfigColumns("FrameCols"); }

void testOsdfConfigColumnsFrameRows() { testOsdfConfigColumns("FrameRows"); }

// -----------------------------------------------------------------------------
class OsdfConfigColumns : public oops::Test {
 public:
  OsdfConfigColumns() {}
  virtual ~OsdfConfigColumns() {}

 private:
  std::string testid() const override { return "test::OsdfConfigColumns"; }

  void register_tests() const override {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    ts.emplace_back(CASE("ioda/testOsdfConfigColumns/testOsdfConfigColumnsFrameRows") {
      testOsdfConfigColumnsFrameRows();
    });
    ts.emplace_back(CASE("ioda/testOsdfConfigColumns/testOsdfConfigColumnsFrameCols") {
      testOsdfConfigColumnsFrameCols();
    });
  }

  void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda
