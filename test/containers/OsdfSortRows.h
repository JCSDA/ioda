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

#include "ioda/containers/Constants.h"
#include "ioda/containers/CreateIFrame.h"
#include "ioda/test/containers/OsdfTestUtils.h"

#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"
#include "oops/util/Logger.h"

namespace ioda {
namespace test {

// -----------------------------------------------------------------------------
void testSortRows(std::string frameType) {
  const std::vector<eckit::LocalConfiguration> & testCasesConfig =
    ::test::TestEnvironment::config().getSubConfigurations("test cases");
  double tolerance = ::test::TestEnvironment::config().getDouble("tolerance", 1e-6);
  for (std::size_t jj = 0; jj < testCasesConfig.size(); ++jj) {
    eckit::LocalConfiguration testCaseConfig = testCasesConfig[jj];
    oops::Log::info() << "Testing: " << testCaseConfig.getString("name") << std::endl;

    std::vector<eckit::LocalConfiguration> sortsConfig =
      testCaseConfig.getSubConfigurations("sorts to perform");
    std::vector<eckit::LocalConfiguration> originalConfig =
      testCaseConfig.getSubConfigurations("original columns");

    std::unique_ptr<osdf::IFrame> testFrame = osdf::createIFrame(frameType);
    std::vector<std::string> originalColumnNames;
    std::vector<std::string> originalColumnTypes;
    populateFrame(originalConfig, testFrame, originalColumnNames, originalColumnTypes);
    ASSERT_MSG(testFrame->numRows() > 0,
               "Test frame must have at least one row for sortRows tests.");
    ASSERT_MSG(testFrame->numCols() > 0,
               "Test frame must have at least one column for sortRows tests.");

    // Find a column name not in the frame and try to sort by it to check that the correct
    // exception is thrown
    std::string nonExistentColumn = "non_existent_column";
    while (testFrame->hasColumn(nonExistentColumn)) {
      nonExistentColumn += "_x";
    }
    EXPECT_THROWS_AS(testFrame->sortRows(nonExistentColumn, osdf::consts::eAscending),
                      eckit::BadParameter);

    // Make sure exception is thrown if invalid sort order is provided
    EXPECT_THROWS_AS(testFrame->sortRows(originalColumnNames.at(0),
                                         static_cast<osdf::consts::eSortOrders>(99)),
                      eckit::BadParameter);

    for (std::size_t sortIndex = 0; sortIndex < sortsConfig.size(); ++sortIndex) {
      eckit::LocalConfiguration sortConfig = sortsConfig[sortIndex];
      std::string sortColumn = sortConfig.getString("column");
      std::string sortOrderStr = sortConfig.getString("order");
      osdf::consts::eSortOrders sortOrder;
      if (sortOrderStr == "ascending") {
        sortOrder = osdf::consts::eAscending;
      } else if (sortOrderStr == "descending") {
        sortOrder = osdf::consts::eDescending;
      } else {
        throw eckit::BadParameter("Invalid sort order: " + sortOrderStr, Here());
      }

      // Perform the sort
      testFrame->sortRows(sortColumn, sortOrder);

      // Get the expected configuration for this sort
      std::string expectedKey = "expected sorted columns " + std::to_string(sortIndex + 1);
      std::vector<eckit::LocalConfiguration> expectedConfig =
        testCaseConfig.getSubConfigurations(expectedKey);
      std::unique_ptr<osdf::IFrame> expectedFrame = osdf::createIFrame(frameType);
      std::vector<std::string> expectedColumnNames;
      std::vector<std::string> expectedColumnTypes;
      populateFrame(expectedConfig, expectedFrame, expectedColumnNames, expectedColumnTypes);

      // Compare the sorted frame with the expected frame
      compareFrames(testFrame, originalColumnNames, originalColumnTypes,
                    expectedFrame, expectedColumnNames, expectedColumnTypes, tolerance, true);
    }
  }
}

// -----------------------------------------------------------------------------
class OsdfSortRows : public oops::Test {
 public:
  OsdfSortRows() {}
  virtual ~OsdfSortRows() {}

 private:
  std::string testid() const override { return "test::OsdfSortRows"; }

  void register_tests() const override {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    ts.emplace_back(CASE("ioda/OsdfSortRows/testFrameCols") { testSortRows("FrameCols"); });
    ts.emplace_back(CASE("ioda/OsdfSortRows/testFrameRows") { testSortRows("FrameRows"); });
  }

  void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda
