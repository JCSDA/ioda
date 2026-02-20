/*
 * (C) Crown copyright 2026, Met Office
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
void testFrames() {
  const std::vector<std::string> frameTypes{ "FrameCols", "FrameRows" };
  const std::vector<eckit::LocalConfiguration> & testCasesConfig =
    ::test::TestEnvironment::config().getSubConfigurations("test cases");

  for (const std::string & frameType : frameTypes) {
    for (std::size_t i = 0; i < testCasesConfig.size(); ++i) {
      const eckit::LocalConfiguration & testCaseConfig = testCasesConfig[i];
      std::string testCaseName = testCaseConfig.getString("name");
      oops::Log::info() << "Testing: " << frameType << ": " << testCaseName << std::endl;

      // Grab the osdf config which contains the column names, types, and values
      // to be used in the test.
      const std::vector<eckit::LocalConfiguration> & osdfConfig =
        testCaseConfig.getSubConfigurations("osdf columns");

      // Create frame from test config, and test against expected sizes.
      std::unique_ptr<osdf::IFrame> testFrame = osdf::createIFrame(frameType);
      std::vector<std::string> testColumnNames;
      std::vector<std::string> testColumnTypes;
      populateFrame(osdfConfig, testFrame, testColumnNames, testColumnTypes);

      // Check against expected values
      const eckit::LocalConfiguration & expectedValuesConfig =
        testCaseConfig.getSubConfiguration("expected values");
      std::size_t expectedNumRows = expectedValuesConfig.getUnsigned("num rows");
      std::size_t expectedNumCols = expectedValuesConfig.getUnsigned("num columns");
      EXPECT(testFrame->numRows() == expectedNumRows);
      EXPECT(testFrame->numCols() == expectedNumCols);
    }
  }
}

// -----------------------------------------------------------------------------
class OsdfNumRowsCols : public oops::Test {
 public:
  OsdfNumRowsCols() {}
  virtual ~OsdfNumRowsCols() {}

 private:
  std::string testid() const override { return "test::OsdfNumRowsCols"; }

  void register_tests() const override {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    ts.emplace_back(CASE("ioda/OsdfNumRowsCols/testFrames") { testFrames(); });
  }

  void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda
