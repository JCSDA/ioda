/*
 * (C) Crown copyright 2026, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#pragma once

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "OsdfCreateIFrame.h"
#include "eckit/exception/Exceptions.h"

#define ECKIT_TESTING_SELF_REGISTER_CASES 0

#include "ioda/containers/CreateIFrame.h"
#include "ioda/containers/IFrame.h"
#include "ioda/reader/ObsReader.hpp"
#include "ioda/test/containers/OsdfTestUtils.h"

#include "oops/mpi/mpi.h"
#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"

namespace ioda {
namespace test {

void runAppend(std::unique_ptr<osdf::IFrame>& frame0,
               const std::unique_ptr<osdf::IFrame>& frame1) {
  // IFrame::append copies the sourceLocationIndices of frame1 unchanged, so keeping them unique
  // is the caller's job. frame0 might have gaps in the sourceLocationIndices numbering, which
  // correponds to file location indices that were filtered out. We want to emulate the
  // production code which uses the number of source locations (before filtering) as the
  // offset for the offsetSourceLocationIndices call. In this test setup, we can calculate
  // what would have been the frame0 number of source locations by adding 1 to the maximum
  // sourceLocationIndices value in frame0.
  if ((frame0->numRows() > 0) && (frame1->numRows() > 0)) {
    std::vector<int> frame0SourceLocIndices;
    frame0->getColumn("sourceLocationIndices", frame0SourceLocIndices);
    reader::offsetSourceLocationIndices(oops::mpi::world(), frame1,
      *std::max_element(frame0SourceLocIndices.begin(), frame0SourceLocIndices.end()) + 1);
  }
  frame0->append(frame1);
}

void testOsdfAppendPasses(std::unique_ptr<osdf::IFrame>& frame0,
                          const std::unique_ptr<osdf::IFrame>& frame1,
                          const std::vector<int> expectedSourceLocIndices) {
  const double tolerance   = ::test::TestEnvironment::config().getDouble("tolerance");
  std::int64_t frame0Size = frame0->numRows();
  std::int64_t frame1Size = frame1->numRows();
  std::int64_t frame0MaxId = frame0->getData().getMaxId();

  runAppend(frame0, frame1);
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

  // Check that the resulting sourceLocationIndices match the expected values
  std::vector<int> sourceLocIndices;
  frame0->getColumn("sourceLocationIndices", sourceLocIndices);
  EXPECT_EQUAL(sourceLocIndices, expectedSourceLocIndices);
}

void testOsdfAppendFails(std::unique_ptr<osdf::IFrame>& frame0,
                         const std::unique_ptr<osdf::IFrame>& frame1) {
  std::int64_t frame0Size = frame0->numRows();
  EXPECT_THROWS(runAppend(frame0, frame1));
  EXPECT_EQUAL(frame0->numRows(), frame0Size);
}

std::unique_ptr<osdf::IFrame> testCreateAppendFrames(
  std::size_t frameIndex, const std::vector<eckit::LocalConfiguration>& testFramesConfig) {
  // For creating empty IFrames
  if (frameIndex == 0) {
    return osdf::createIFrame("FrameRows");
  }
  if (frameIndex == 1) {
    return osdf::createIFrame("FrameCols");
  }

  const std::vector<eckit::LocalConfiguration>& osdfColumnsConfig
    = testFramesConfig[frameIndex-2].getSubConfigurations("osdf columns");
  std::unique_ptr<osdf::IFrame> testFrame
    = osdf::createIFrame(testFramesConfig[frameIndex-2].getString("type"));
  std::vector<std::string> testColumnNames;
  std::vector<std::string> testColumnTypes;
  populateFrame(osdfColumnsConfig, testFrame, testColumnNames, testColumnTypes);
  return testFrame;
}

std::string testFrameNameString(std::size_t frameIndex,
                                const std::vector<eckit::LocalConfiguration>& testFramesConfig) {
  if (frameIndex == 0) {
    return "FrameRowsEmpty";
  }
  if (frameIndex == 1) {
    return "FrameColsEmpty";
  }
  return testFramesConfig[frameIndex-2].getString("name");
}

void testOsdfAppend(std::size_t frame0Index, std::size_t frame1Index,
                    const std::vector<eckit::LocalConfiguration>& testFramesConfig,
                    bool expectedResult,
                    const std::vector<int>& expectedSourceLocIndices) {
  oops::Log::info() << "Appending frames: "
                     << testFrameNameString(frame0Index, testFramesConfig)
                     << " and "
                     << testFrameNameString(frame1Index, testFramesConfig)
                     << " with expected outcome: " << (expectedResult ? "true" : "false")
                     << std::endl;

  std::unique_ptr<osdf::IFrame> frame0 = testCreateAppendFrames(frame0Index, testFramesConfig);
  std::unique_ptr<osdf::IFrame> frame1 = testCreateAppendFrames(frame1Index, testFramesConfig);

  if (expectedResult) {
    testOsdfAppendPasses(frame0, frame1, expectedSourceLocIndices);
  } else {
    testOsdfAppendFails(frame0, frame1);
  }
}

void testOsdfAppendFrames() {
  // Create configs for test cases
  const std::vector<eckit::LocalConfiguration> testCasesConfig
    = ::test::TestEnvironment::config().getSubConfigurations("test appends");

  // Create configs for IFrames
  const std::vector<eckit::LocalConfiguration> testFramesConfig
    = ::test::TestEnvironment::config().getSubConfigurations("test frames");

  // run test cases
  for (std::size_t i = 0; i < testCasesConfig.size(); ++i) {
    const eckit::LocalConfiguration& testCaseConfig = testCasesConfig[i];
    std::size_t testFrame0Index = testCaseConfig.getUnsigned("frame 0");
    std::size_t testFrame1Index = testCaseConfig.getUnsigned("frame 1");
    bool testExpectedResult = testCaseConfig.getBool("expectedResult");
    const std::vector<int> testExpectedSourceLocIndices
      = testCaseConfig.getIntVector("expectedSourceLocIndices");

    testOsdfAppend(testFrame0Index, testFrame1Index, testFramesConfig,
                     testExpectedResult, testExpectedSourceLocIndices);
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
