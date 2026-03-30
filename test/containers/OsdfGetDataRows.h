/*
 * (C) Crown copyright 2026, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#pragma once

#include <memory>
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

template <typename frameType>
void testIFrameGetDataRows(frameType& testFrame,
                           const std::vector<eckit::LocalConfiguration>& osdfConfig,
                           std::vector<osdf::DataRow>& referenceData) {
  // Load data into frame
  const double tolerance = ::test::TestEnvironment::config().getDouble("tolerance");
  std::vector<std::string> columnNames;
  std::vector<std::string> columnTypes;
  populateFrame(osdfConfig, testFrame, columnNames, columnTypes);

  // Call getDataRows
  std::vector<osdf::DataRow> testRows;
  testRows.reserve(testFrame.getData().getSizeRows());
  testFrame.getData().getDataRows(testRows);

  // Check number of entries correct
  std::size_t numRows = referenceData.size();
  EXPECT(numRows == testRows.size());

  // Test getDataRows fails if passed non-empty vector
  if (!testRows.empty()) {
    EXPECT_THROWS_AS(testFrame.getData().getDataRows(testRows), eckit::BadParameter);
  }

  for (std::size_t index = 0; index < numRows; ++index) {
    // Passing tests (compare to input data)
    EXPECT_EQUAL(testCompareTwoDataRows(referenceData[index], testRows[index], 1, tolerance), 1);
    EXPECT_EQUAL(testCompareTwoDataRows(referenceData[index], testFrame.getData().getDataRow(index),
                                        1, tolerance), 1);

    // Throwing as incorrect index
    std::size_t nextIndex = (index+1)%numRows;
    EXPECT_EQUAL(testCompareTwoDataRows(referenceData[index],
                                        testRows[nextIndex], 1, tolerance), 0);
    EXPECT_EQUAL(testCompareTwoDataRows(referenceData[index],
                                        testFrame.getData().getDataRow(nextIndex), 1, tolerance),
      0);

    // Throwing as incorrect contents
    EXPECT_EQUAL(testCompareTwoDataRows(referenceData[index],
                                        testRows[nextIndex], 0, tolerance), 0);
    EXPECT_EQUAL(testCompareTwoDataRows(referenceData[index],
                                        testFrame.getData().getDataRow(nextIndex),
                                        0, tolerance), 0);
  }
}

void testGetDataRows() {
  const std::vector<eckit::LocalConfiguration>& testCasesConfig
    = ::test::TestEnvironment::config().getSubConfigurations("test frame data");

  for (std::size_t index = 0; index < testCasesConfig.size(); ++index) {
    const eckit::LocalConfiguration& testCaseConfig = testCasesConfig[index];
    std::string testCaseName                        = testCaseConfig.getString("name");

    oops::Log::info() << "Running test: " << testCaseName << std::endl;
    const std::vector<eckit::LocalConfiguration>& osdfConfig
      = testCaseConfig.getSubConfigurations("osdf columns");

    // Store input data directly into DataRows for comparison
    const std::size_t numRows = osdfConfig[0].getStringVector("values").size();
    std::vector<osdf::DataRow> compareDataRows;
    compareDataRows.reserve(numRows);

    for (std::size_t index = 0; index < numRows; ++index) {
      osdf::DataRow tempDataRow(index);
      for (std::size_t i = 0; i < osdfConfig.size(); ++i) {
        std::string type                    = osdfConfig[i].getString("type");
        std::vector<std::string> stringVals = osdfConfig[i].getStringVector("values");
        std::string errMsg
          = std::string("Unrecognized data type: ") + type
            + std::string("\nMust use one of: 'int', 'int64', 'float', 'string', or 'char'");

        callWithSupportedTypeString(type, errMsg, [&](auto typeDiscriminator) {
          using T                       = decltype(typeDiscriminator);
          std::vector<T> expectedValues = replaceMissingValues<T>(stringVals);
          tempDataRow.insert(std::make_shared<osdf::Datum<T>>(expectedValues[index]));
        });
      }
      compareDataRows.emplace_back(tempDataRow);
    }

    // Create and test FrameRows and FrameCols
    osdf::FrameCols testFrameCols;
    testIFrameGetDataRows(testFrameCols, osdfConfig, compareDataRows);
    osdf::FrameRows testFrameRows;
    testIFrameGetDataRows(testFrameRows, osdfConfig, compareDataRows);
  }
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

    ts.emplace_back(CASE("ioda/ObsDataFrame/testGetDataRows") { testGetDataRows(); });
  }

  void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda
