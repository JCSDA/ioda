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

#include "ioda/containers/CreateIFrame.h"
#include "ioda/containers/FrameCols.h"
#include "ioda/containers/FrameRows.h"
#include "ioda/test/containers/OsdfTestUtils.h"
#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"

namespace ioda {
namespace test {

struct frameDataContainer {
  std::size_t numRows;
  std::size_t numCols;
  std::size_t maxId;
  std::vector<std::string> columnNames;
  std::vector<std::string> columnTypes;
  std::vector<std::string> columnUnits;
};

// This test checks that the size, types, units, names, maxId are correct in IFrameData
void testOsdfGetDataFrame(std::string frameType,
                          const std::vector<eckit::LocalConfiguration> osdfConfig,
                          frameDataContainer frameData) {
  oops::Log::info() << "Testing " << frameType << std::endl;
  std::unique_ptr<osdf::IFrame> testFrame = osdf::createIFrame(frameType);
  std::vector<std::string> columnTypes;
  populateFrame(osdfConfig, testFrame, frameData.columnNames, frameData.columnTypes);

  // Note testFrame->getData() cannot be stored as it returns pure virtual class IFrameData
  // rather than FrameRowsData/FrameColsData as in case where testFrame is FrameRows/FrameCols.
  EXPECT_EQUAL(testFrame->getData().getSizeCols(), frameData.numCols);
  EXPECT_EQUAL(testFrame->getData().getSizeRows(), frameData.numRows);
  EXPECT_EQUAL(testFrame->getData().getMaxId(), frameData.maxId);

  for (std::size_t index = 0; index < frameData.numCols; ++index) {
    EXPECT_EQUAL(testFrame->getData().getName(index), frameData.columnNames[index]);
    EXPECT_EQUAL(testFrame->getData().getUnits(index), frameData.columnUnits[index]);
    EXPECT_EQUAL(testFrame->getData().getPermission(index), osdf::consts::eReadWrite);

    if (frameData.columnTypes[index] == "int") {
      EXPECT_EQUAL(testFrame->getData().getType(index), osdf::consts::eInt);
    } else if (frameData.columnTypes[index] == "int64") {
      EXPECT_EQUAL(testFrame->getData().getType(index), osdf::consts::eInt64);
    } else if (frameData.columnTypes[index] == "float") {
      EXPECT_EQUAL(testFrame->getData().getType(index), osdf::consts::eFloat);
    } else if (frameData.columnTypes[index] == "string") {
      EXPECT_EQUAL(testFrame->getData().getType(index), osdf::consts::eString);
    } else if (frameData.columnTypes[index] == "char") {
      EXPECT_EQUAL(testFrame->getData().getType(index), osdf::consts::eChar);
    } else {
      throw eckit::BadParameter("Invalid column type in yaml.", Here());
    }
  }
}

void testOsdfGetDataEmptyFrame(std::string frameType) {
  std::shared_ptr<osdf::IFrame> testFrame = osdf::createIFrame(frameType);
  EXPECT_EQUAL(testFrame->getData().getSizeRows(), 0);
  EXPECT_EQUAL(testFrame->getData().getSizeCols(), 0);
  EXPECT_EQUAL(testFrame->getData().getMaxId(), -1);
}

void testOsdfGetData() {
  const std::vector<eckit::LocalConfiguration>& testCasesConfig
    = ::test::TestEnvironment::config().getSubConfigurations("test frame data");

  for (std::size_t index = 0; index < testCasesConfig.size(); ++index) {
    const eckit::LocalConfiguration& testCaseConfig = testCasesConfig[index];
    std::string testCaseName = testCaseConfig.getString("name");

    oops::Log::info() << "Running test: " << testCaseName << std::endl;

    const std::vector<eckit::LocalConfiguration>& osdfConfig
      = testCaseConfig.getSubConfigurations("osdf columns");

    // Get data directly from yaml (don't need to get names or types as populateFrame does this)
    std::size_t numColumns = osdfConfig.size();
    std::vector<std::string> columnUnits(numColumns);

    for (std::size_t index = 0; index < numColumns; ++index) {
      try {
        columnUnits[index] = osdfConfig[index].getString("unit");
      } catch (eckit::Exception&) {
        columnUnits[index] = "MISSING*";
      }
    }

    frameDataContainer newContainer = frameDataContainer();
    newContainer.numRows = osdfConfig[0].getStringVector("values").size();
    newContainer.numCols = numColumns;
    newContainer.columnUnits = columnUnits;
    newContainer.maxId = newContainer.numRows - 1;

    // Run tests for row and column frames
    testOsdfGetDataFrame("FrameRows", osdfConfig, newContainer);
    testOsdfGetDataFrame("FrameCols", osdfConfig, newContainer);
  }
}

void testOsdfGetDataEmpty() {
  testOsdfGetDataEmptyFrame("FrameRows");
  testOsdfGetDataEmptyFrame("FrameCols");
}

// -----------------------------------------------------------------------------
class OsdfGetData : public oops::Test {
 public:
  OsdfGetData() {}
  virtual ~OsdfGetData() {}

 private:
  std::string testid() const override { return "test::OsdfGetData"; }

  void register_tests() const override {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    ts.emplace_back(CASE("ioda/OsdfGetData/tetOsdfGetData") { testOsdfGetData(); });
    ts.emplace_back(CASE("ioda/OsdfGetData/tetOsdfGetDataEmpty") { testOsdfGetDataEmpty(); });
  }

  void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda
