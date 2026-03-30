/*
 * (C) Crown copyright 2026, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "eckit/exception/Exceptions.h"

#define ECKIT_TESTING_SELF_REGISTER_CASES 0

#include "ioda/containers/CreateIFrame.h"
#include "ioda/containers/IFrame.h"
#include "ioda/test/containers/OsdfTestUtils.h"
#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"

namespace ioda {
namespace test {

void testOsdfAppendPasses(std::unique_ptr<osdf::IFrame>& frame0,
                          const std::unique_ptr<osdf::IFrame>& frame1) {
  const double tolerance   = ::test::TestEnvironment::config().getDouble("tolerance");
  std::int64_t frame0Size = frame0->numRows();
  std::int64_t frame1Size = frame1->numRows();
  std::int64_t frame0MaxId = frame0->getData().getMaxId();

  frame0->append(frame1);
  EXPECT_EQUAL(frame0->numRows(), frame0Size + frame1Size);  // check size
  EXPECT_EQUAL(frame0->getData().getMaxId(), frame0MaxId + frame1Size);

  std::vector<osdf::DataRow> dataRows0;
  dataRows0.reserve(frame0->getData().getSizeRows());
  frame0->getData().getDataRows(dataRows0);

  std::vector<osdf::DataRow> dataRows1;
  dataRows1.reserve(frame1->getData().getSizeRows());
  frame1->getData().getDataRows(dataRows1);

  for (std::int64_t index = 0; index < frame1Size; ++index) {
    // Check that row contents match (do not check Ids as these are different)
    EXPECT_EQUAL(testCompareTwoDataRows(dataRows0.at(frame0Size + index),
                                        dataRows1.at(index), 0, tolerance), 1);
  }
}

void testOsdfAppendFails(std::unique_ptr<osdf::IFrame>& frame0,
                         const std::unique_ptr<osdf::IFrame>& frame1) {
  std::int64_t frame0Size = frame0->numRows();
  EXPECT_THROWS_AS(frame0->append(frame1), eckit::BadParameter);
  EXPECT_EQUAL(frame0->numRows(), frame0Size);
}

void testOsdfAppend(std::unique_ptr<osdf::IFrame>& frame0,
                    const std::unique_ptr<osdf::IFrame>& frame1, bool expectedResult) {
  if (expectedResult) {
    testOsdfAppendPasses(frame0, frame1);
  } else {
    testOsdfAppendFails(frame0, frame1);
  }
}

void testOsdfAppendFrames() {
  /// construct frames to test
  const std::vector<eckit::LocalConfiguration> testFramesConfig
    = ::test::TestEnvironment::config().getSubConfigurations("test frames");

  std::vector<std::unique_ptr<osdf::IFrame>> testFrames;
  std::vector<std::string> testFramesNames;
  oops::Log::error() << "Creating test IFrames" << std::endl;

  // construct empty IFrames for testing
  testFrames.emplace_back(osdf::createIFrame("FrameRows"));
  testFramesNames.emplace_back("EmptyFrameRows");
  testFrames.emplace_back(osdf::createIFrame("FrameCols"));
  testFramesNames.emplace_back("EmptyFramesCols");

  for (std::size_t i = 0; i < testFramesConfig.size(); ++i) {
    const eckit::LocalConfiguration& testFrameConfig = testFramesConfig[i];
    testFramesNames.emplace_back(testFrameConfig.getString("name"));

    const std::vector<eckit::LocalConfiguration>& osdfColumnsConfig
      = testFrameConfig.getSubConfigurations("osdf columns");

    std::unique_ptr<osdf::IFrame> testFrame = osdf::createIFrame(testFrameConfig.getString("type"));
    std::vector<std::string> testColumnNames;
    std::vector<std::string> testColumnTypes;
    populateFrame(osdfColumnsConfig, testFrame, testColumnNames, testColumnTypes);

    testFrames.emplace_back(std::move(testFrame));
  }

  oops::Log::error() << "Test IFrames created" << std::endl;

  // run test cases
  // - config contains index of frames to append in testFrames vector and expected result boolean
  const std::vector<eckit::LocalConfiguration> testCasesConfig
    = ::test::TestEnvironment::config().getSubConfigurations("test appends");

  for (std::size_t i = 0; i < testCasesConfig.size(); ++i) {
    const eckit::LocalConfiguration& testCaseConfig = testCasesConfig[i];
    std::size_t testFrame0Index = testCaseConfig.getUnsigned("frame 0");
    std::size_t testFrame1Index = testCaseConfig.getUnsigned("frame 1");
    bool testExpectation = testCaseConfig.getBool("expect");

    oops::Log::error() << "Appending frames: " << testFramesNames[testFrame0Index] << " and "
                       << testFramesNames[testFrame1Index]
                       << " with expected outcome: " << (testExpectation ? "true" : "false")
                       << std::endl;

    testOsdfAppend(testFrames[testFrame0Index], testFrames[testFrame1Index], testExpectation);
  }
}

// -----------------------------------------------------------------------------
class OsdfAppend : public oops::Test {
 public:
  OsdfAppend() {}
  virtual ~OsdfAppend() {}

 private:
  std::string testid() const override { return "test::OsdfAppend"; }

  void register_tests() const override {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    ts.emplace_back(CASE("ioda/OsdfAppend/testOsdfAppend") { testOsdfAppendFrames(); });
  }

  void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda
