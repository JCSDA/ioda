/*
 * (C) Copyright 2026, UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#pragma once

#include <cmath>
#include <string>
#include <vector>

#define ECKIT_TESTING_SELF_REGISTER_CASES 0

#include "eckit/exception/Exceptions.h"

#include "ioda/containers/Constants.h"
#include "ioda/containers/FrameCols.h"
#include "ioda/containers/FrameRows.h"

#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"

namespace ioda {
namespace test {

// Each template function is instantiated for both FrameRows and FrameCols in
// register_tests. All five comparison operators from osdf::consts::eComparisons
// are exercised across the suite: eLessThan, eLessThanOrEqualTo, eEqualTo,
// eGreaterThanOrEqualTo, eGreaterThan.

// -----------------------------------------------------------------------------
template<typename FrameType>
void testSliceRows_int_equalTo() {
  FrameType frame;
  frame.appendNewColumn("rank",  std::vector<int>{1, 2, 1, 3, 1});
  frame.appendNewColumn("value", std::vector<int>{10, 20, 30, 40, 50});

  auto sliced = frame.sliceRows("rank", osdf::consts::eEqualTo, 1);

  EXPECT(sliced.numRows() == 3);
  EXPECT(sliced.numCols() == 2);

  std::vector<int> ranks;
  std::vector<int> values;
  sliced.getColumn("rank",  ranks);
  sliced.getColumn("value", values);
  EXPECT(ranks  == std::vector<int>({1, 1, 1}));
  EXPECT(values == std::vector<int>({10, 30, 50}));
}

// -----------------------------------------------------------------------------
template<typename FrameType>
void testSliceRows_int64_greaterThan() {
  FrameType frame;
  frame.appendNewColumn("obs_time",   std::vector<std::int64_t>{100, 200, 300, 400, 500});
  frame.appendNewColumn("station_id", std::vector<int>{1, 2, 3, 4, 5});

  auto sliced = frame.sliceRows("obs_time", osdf::consts::eGreaterThan,
                                static_cast<std::int64_t>(250));

  EXPECT(sliced.numRows() == 3);

  std::vector<std::int64_t> times;
  sliced.getColumn("obs_time", times);
  EXPECT(times == std::vector<std::int64_t>({300, 400, 500}));
}

// -----------------------------------------------------------------------------
template<typename FrameType>
void testSliceRows_float_lessThanOrEqualTo() {
  const float tolerance = 1.0e-6f;
  FrameType frame;
  frame.appendNewColumn("latitude",  std::vector<float>{-90.0f, -45.0f, 0.0f, 45.0f, 90.0f});
  frame.appendNewColumn("longitude", std::vector<float>{0.0f, 1.0f, 2.0f, 3.0f, 4.0f});

  auto sliced = frame.sliceRows("latitude", osdf::consts::eLessThanOrEqualTo, 0.0f);

  EXPECT(sliced.numRows() == 3);

  std::vector<float> lats;
  sliced.getColumn("latitude", lats);
  const std::vector<float> expected{-90.0f, -45.0f, 0.0f};
  for (std::size_t i = 0; i < lats.size(); ++i) {
    EXPECT(fabs(lats[i] - expected[i]) < tolerance);
  }
}

// -----------------------------------------------------------------------------
template<typename FrameType>
void testSliceRows_string_equalTo() {
  FrameType frame;
  frame.appendNewColumn("sensor",
                        std::vector<std::string>{"amsua", "atms", "amsua", "mhs"});
  frame.appendNewColumn("channel", std::vector<int>{1, 2, 3, 4});

  auto sliced = frame.sliceRows("sensor", osdf::consts::eEqualTo, std::string("amsua"));

  EXPECT(sliced.numRows() == 2);

  std::vector<std::string> sensors;
  std::vector<int>         channels;
  sliced.getColumn("sensor",  sensors);
  sliced.getColumn("channel", channels);
  EXPECT(sensors  == std::vector<std::string>({"amsua", "amsua"}));
  EXPECT(channels == std::vector<int>({1, 3}));
}

// -----------------------------------------------------------------------------
template<typename FrameType>
void testSliceRows_allRowsMatch() {
  FrameType frame;
  frame.appendNewColumn("value", std::vector<int>{5, 10, 15});
  frame.appendNewColumn("label", std::vector<std::string>{"a", "b", "c"});

  auto sliced = frame.sliceRows("value", osdf::consts::eGreaterThanOrEqualTo, 0);

  EXPECT(sliced.numRows() == 3);
  EXPECT(sliced.numCols() == 2);

  std::vector<int> values;
  sliced.getColumn("value", values);
  EXPECT(values == std::vector<int>({5, 10, 15}));
}

// -----------------------------------------------------------------------------
template<typename FrameType>
void testSliceRows_noRowsMatch() {
  FrameType frame;
  frame.appendNewColumn("value", std::vector<int>{1, 2, 3});
  frame.appendNewColumn("label", std::vector<std::string>{"a", "b", "c"});

  auto sliced = frame.sliceRows("value", osdf::consts::eLessThan, 0);

  EXPECT(sliced.numRows() == 0);
  EXPECT(sliced.numCols() == 2);
  EXPECT(sliced.columnNames() == frame.columnNames());
}

// -----------------------------------------------------------------------------
template<typename FrameType>
void testSliceRows_int_lessThan() {
  FrameType frame;
  frame.appendNewColumn("value", std::vector<int>{10, 20, 30, 40, 50});
  frame.appendNewColumn("label", std::vector<std::string>{"a", "b", "c", "d", "e"});

  // Threshold is 30: boundary row (value == 30) must be excluded to distinguish
  // eLessThan from eLessThanOrEqualTo.
  auto sliced = frame.sliceRows("value", osdf::consts::eLessThan, 30);

  EXPECT(sliced.numRows() == 2);

  std::vector<int> values;
  sliced.getColumn("value", values);
  EXPECT(values == std::vector<int>({10, 20}));
}

// -----------------------------------------------------------------------------
template<typename FrameType>
void testSliceRows_int_greaterThanOrEqualTo() {
  FrameType frame;
  frame.appendNewColumn("value", std::vector<int>{1, 2, 3, 4, 5});
  frame.appendNewColumn("label", std::vector<std::string>{"a", "b", "c", "d", "e"});

  // Threshold is 3: boundary row (value == 3) must be included to distinguish
  // eGreaterThanOrEqualTo from eGreaterThan.
  auto sliced = frame.sliceRows("value", osdf::consts::eGreaterThanOrEqualTo, 3);

  EXPECT(sliced.numRows() == 3);

  std::vector<int> values;
  sliced.getColumn("value", values);
  EXPECT(values == std::vector<int>({3, 4, 5}));
}

// -----------------------------------------------------------------------------
template<typename FrameType>
void testSliceRows_invalidColumn() {
  FrameType frame;
  frame.appendNewColumn("value", std::vector<int>{1, 2, 3});

  EXPECT_THROWS_AS(frame.sliceRows("nonexistent", osdf::consts::eEqualTo, 1),
                   eckit::BadParameter);
}

// -----------------------------------------------------------------------------
class OsdfSliceRows : public oops::Test {
 public:
  OsdfSliceRows() {}
  virtual ~OsdfSliceRows() {}

 private:
  std::string testid() const override { return "test::OsdfSliceRows"; }

  void register_tests() const override {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    ts.emplace_back(CASE("ioda/OsdfSliceRows/FrameRows/testIntEqualTo")
        { testSliceRows_int_equalTo<osdf::FrameRows>(); });
    ts.emplace_back(CASE("ioda/OsdfSliceRows/FrameCols/testIntEqualTo")
        { testSliceRows_int_equalTo<osdf::FrameCols>(); });

    ts.emplace_back(CASE("ioda/OsdfSliceRows/FrameRows/testInt64GreaterThan")
        { testSliceRows_int64_greaterThan<osdf::FrameRows>(); });
    ts.emplace_back(CASE("ioda/OsdfSliceRows/FrameCols/testInt64GreaterThan")
        { testSliceRows_int64_greaterThan<osdf::FrameCols>(); });

    ts.emplace_back(CASE("ioda/OsdfSliceRows/FrameRows/testFloatLessThanOrEqualTo")
        { testSliceRows_float_lessThanOrEqualTo<osdf::FrameRows>(); });
    ts.emplace_back(CASE("ioda/OsdfSliceRows/FrameCols/testFloatLessThanOrEqualTo")
        { testSliceRows_float_lessThanOrEqualTo<osdf::FrameCols>(); });

    ts.emplace_back(CASE("ioda/OsdfSliceRows/FrameRows/testStringEqualTo")
        { testSliceRows_string_equalTo<osdf::FrameRows>(); });
    ts.emplace_back(CASE("ioda/OsdfSliceRows/FrameCols/testStringEqualTo")
        { testSliceRows_string_equalTo<osdf::FrameCols>(); });

    ts.emplace_back(CASE("ioda/OsdfSliceRows/FrameRows/testAllRowsMatch")
        { testSliceRows_allRowsMatch<osdf::FrameRows>(); });
    ts.emplace_back(CASE("ioda/OsdfSliceRows/FrameCols/testAllRowsMatch")
        { testSliceRows_allRowsMatch<osdf::FrameCols>(); });

    ts.emplace_back(CASE("ioda/OsdfSliceRows/FrameRows/testNoRowsMatch")
        { testSliceRows_noRowsMatch<osdf::FrameRows>(); });
    ts.emplace_back(CASE("ioda/OsdfSliceRows/FrameCols/testNoRowsMatch")
        { testSliceRows_noRowsMatch<osdf::FrameCols>(); });

    ts.emplace_back(CASE("ioda/OsdfSliceRows/FrameRows/testIntLessThan")
        { testSliceRows_int_lessThan<osdf::FrameRows>(); });
    ts.emplace_back(CASE("ioda/OsdfSliceRows/FrameCols/testIntLessThan")
        { testSliceRows_int_lessThan<osdf::FrameCols>(); });

    ts.emplace_back(CASE("ioda/OsdfSliceRows/FrameRows/testIntGreaterThanOrEqualTo")
        { testSliceRows_int_greaterThanOrEqualTo<osdf::FrameRows>(); });
    ts.emplace_back(CASE("ioda/OsdfSliceRows/FrameCols/testIntGreaterThanOrEqualTo")
        { testSliceRows_int_greaterThanOrEqualTo<osdf::FrameCols>(); });

    ts.emplace_back(CASE("ioda/OsdfSliceRows/FrameRows/testInvalidColumn")
        { testSliceRows_invalidColumn<osdf::FrameRows>(); });
    ts.emplace_back(CASE("ioda/OsdfSliceRows/FrameCols/testInvalidColumn")
        { testSliceRows_invalidColumn<osdf::FrameCols>(); });
  }

  void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda
