/*
 * (C) Copyright 2026, UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#pragma once

#include <cmath>
#include <memory>
#include <string>
#include <vector>

#define ECKIT_TESTING_SELF_REGISTER_CASES 0

#include "eckit/io/Buffer.h"

#include "ioda/containers/Constants.h"
#include "ioda/containers/CreateIFrame.h"
#include "ioda/containers/IFrame.h"

#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"

namespace ioda {
namespace test {

// All tests operate exclusively through IFrame* (unique_ptr<IFrame>). No casts
// to concrete types are made. Each test is called for both "FrameRows" and
// "FrameCols" in register_tests.

// -----------------------------------------------------------------------------
void testSliceFrame_correctResult(const std::string& frameType) {
  const float tolerance = 1.0e-6f;
  std::unique_ptr<osdf::IFrame> frame = osdf::createIFrame(frameType);
  frame->appendNewColumn("rank",  std::vector<int>{1, 2, 1, 3, 1});
  frame->appendNewColumn("value", std::vector<float>{1.0f, 2.0f, 3.0f, 4.0f, 5.0f});

  std::unique_ptr<osdf::IFrame> sliced =
      frame->sliceFrame("rank", osdf::consts::eEqualTo, 1);

  EXPECT(sliced->numRows() == 3);
  EXPECT(sliced->numCols() == 2);

  std::vector<float> values;
  sliced->getColumn("value", values);
  const std::vector<float> expected{1.0f, 3.0f, 5.0f};
  for (std::size_t i = 0; i < values.size(); ++i) {
    EXPECT(fabs(values[i] - expected[i]) < tolerance);
  }
}

// -----------------------------------------------------------------------------
void testSliceFrame_frameTypePreserved(const std::string& frameType) {
  std::unique_ptr<osdf::IFrame> frame = osdf::createIFrame(frameType);
  frame->appendNewColumn("rank", std::vector<int>{1, 2, 1});

  std::unique_ptr<osdf::IFrame> sliced =
      frame->sliceFrame("rank", osdf::consts::eEqualTo, 1);

  EXPECT(sliced->frameType() == frameType);
}

// -----------------------------------------------------------------------------
void testSliceFrame_chaining(const std::string& frameType) {
  std::unique_ptr<osdf::IFrame> frame = osdf::createIFrame(frameType);
  frame->appendNewColumn("rank",  std::vector<int>{1, 2, 1, 2, 1});
  frame->appendNewColumn("value", std::vector<int>{10, 20, 30, 40, 50});

  // First slice: rank == 1 -> rows with values {10, 30, 50}
  // Second slice: value > 20 -> rows with values {30, 50}
  std::unique_ptr<osdf::IFrame> sliced =
      frame->sliceFrame("rank",  osdf::consts::eEqualTo,     1)
           ->sliceFrame("value", osdf::consts::eGreaterThan, 20);

  EXPECT(sliced->numRows() == 2);

  std::vector<int> values;
  sliced->getColumn("value", values);
  EXPECT(values == std::vector<int>({30, 50}));
}

// -----------------------------------------------------------------------------
void testSliceFrame_emptyResult(const std::string& frameType) {
  std::unique_ptr<osdf::IFrame> frame = osdf::createIFrame(frameType);
  frame->appendNewColumn("rank",  std::vector<int>{1, 2, 3});
  frame->appendNewColumn("label", std::vector<std::string>{"a", "b", "c"});

  std::unique_ptr<osdf::IFrame> sliced =
      frame->sliceFrame("rank", osdf::consts::eGreaterThan, 100);

  EXPECT(sliced->numRows() == 0);
  EXPECT(sliced->numCols() == 2);
  EXPECT(sliced->columnNames() == frame->columnNames());
}

// -----------------------------------------------------------------------------
class OsdfIFrameSliceFrame : public oops::Test {
 public:
  OsdfIFrameSliceFrame() {}
  virtual ~OsdfIFrameSliceFrame() {}

 private:
  std::string testid() const override { return "test::OsdfIFrameSliceFrame"; }

  void register_tests() const override {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    ts.emplace_back(CASE("ioda/OsdfIFrameSliceFrame/FrameRows/testCorrectResult")
        { testSliceFrame_correctResult("FrameRows"); });
    ts.emplace_back(CASE("ioda/OsdfIFrameSliceFrame/FrameCols/testCorrectResult")
        { testSliceFrame_correctResult("FrameCols"); });

    ts.emplace_back(CASE("ioda/OsdfIFrameSliceFrame/FrameRows/testFrameTypePreserved")
        { testSliceFrame_frameTypePreserved("FrameRows"); });
    ts.emplace_back(CASE("ioda/OsdfIFrameSliceFrame/FrameCols/testFrameTypePreserved")
        { testSliceFrame_frameTypePreserved("FrameCols"); });

    ts.emplace_back(CASE("ioda/OsdfIFrameSliceFrame/FrameRows/testChaining")
        { testSliceFrame_chaining("FrameRows"); });
    ts.emplace_back(CASE("ioda/OsdfIFrameSliceFrame/FrameCols/testChaining")
        { testSliceFrame_chaining("FrameCols"); });

    ts.emplace_back(CASE("ioda/OsdfIFrameSliceFrame/FrameRows/testEmptyResult")
        { testSliceFrame_emptyResult("FrameRows"); });
    ts.emplace_back(CASE("ioda/OsdfIFrameSliceFrame/FrameCols/testEmptyResult")
        { testSliceFrame_emptyResult("FrameCols"); });
  }

  void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda
