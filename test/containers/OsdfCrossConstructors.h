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

namespace ioda {
namespace test {

void testOsdfCrossConstructorsEmpty() {
  const double tolerance = ::test::TestEnvironment::config().getDouble("tolerance");

  oops::Log::debug() << "Running test: Empty IFrame" << std::endl;
  osdf::FrameRows refEmptyFrameRows;
  osdf::FrameCols refEmptyFrameCols;

  osdf::FrameRows testEmptyFrameRows(refEmptyFrameCols);
  osdf::FrameCols testEmptyFrameCols(refEmptyFrameRows);

  compareFrames(testEmptyFrameCols, {}, {}, refEmptyFrameCols, {}, {}, tolerance, true);
  compareFrames(testEmptyFrameRows, {}, {}, refEmptyFrameRows, {}, {}, tolerance, true);
}

void testOsdfCrossConstructors() {
  const std::vector<eckit::LocalConfiguration>& testCasesConfig
    = ::test::TestEnvironment::config().getSubConfigurations("test frame data");
  const double tolerance = ::test::TestEnvironment::config().getDouble("tolerance");

  for (std::size_t index = 0; index < testCasesConfig.size(); ++index) {
    const eckit::LocalConfiguration& testCaseConfig = testCasesConfig[index];
    std::string testCaseName = testCaseConfig.getString("name");

    oops::Log::debug() << "Running test: " << testCaseName << std::endl;

    const std::vector<eckit::LocalConfiguration>& osdfConfig
      = testCaseConfig.getSubConfigurations("osdf columns");

    // Construct frameCols and frameRows from yaml
    osdf::FrameCols referenceFrameCols;
    std::vector<std::string> referenceColumnNamesFrameCols;
    std::vector<std::string> referenceColumnTypesFrameCols;
    populateFrame(osdfConfig, referenceFrameCols, referenceColumnNamesFrameCols,
                  referenceColumnTypesFrameCols);

    osdf::FrameRows referenceFrameRows;
    std::vector<std::string> referenceColumnNamesFrameRows;
    std::vector<std::string> referenceColumnTypesFrameRows;
    populateFrame<osdf::FrameRows>(osdfConfig, referenceFrameRows, referenceColumnNamesFrameRows,
                  referenceColumnTypesFrameRows);

    // Cross construct frameCols and frameRows
    osdf::FrameCols testFrameCols(referenceFrameRows);
    std::vector<std::string> testFrameColsNames = testFrameCols.columnNames();
    osdf::FrameRows testFrameRows(referenceFrameCols);
    std::vector<std::string> testFrameRowsNames = testFrameRows.columnNames();

    compareFrames(testFrameCols, testFrameColsNames, referenceColumnTypesFrameCols,
                  referenceFrameCols, referenceColumnNamesFrameCols, referenceColumnTypesFrameCols,
                  tolerance, true);

    compareFrames(testFrameRows, testFrameRowsNames, referenceColumnTypesFrameRows,
                  referenceFrameRows, referenceColumnNamesFrameRows, referenceColumnTypesFrameRows,
                  tolerance, true);
  }
}

// -----------------------------------------------------------------------------
class OsdfCrossConstructors : public oops::Test {
 public:
  OsdfCrossConstructors() {}
  virtual ~OsdfCrossConstructors() {}

 private:
  std::string testid() const override { return "test::OsdfCrossConstructors"; }

  void register_tests() const override {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    ts.emplace_back(CASE("ioda/OsdfCrossConstructors/testOsdfCrossConstructors") {
      testOsdfCrossConstructors();
    });
    ts.emplace_back(CASE("ioda/OsdfCrossConstructors/testOsdfCrossConstructorsEmpty") {
      testOsdfCrossConstructorsEmpty();
    });
  }

  void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda

