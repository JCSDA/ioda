/*
 * (C) Copyright 2026 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#define ECKIT_TESTING_SELF_REGISTER_CASES 0

#include <boost/noncopyable.hpp>

#include "eckit/exception/Exceptions.h"
#include "eckit/testing/Test.h"

#include "ioda/containers/Constants.h"
#include "ioda/containers/FrameCols.h"
#include "ioda/containers/FrameRows.h"
#include "ioda/containers/IFrame.h"

#include "oops/runs/Test.h"
#include "oops/util/Logger.h"
#include "oops/util/missingValues.h"

namespace ioda {
namespace test {

// The row at this index is set to the (type-specific) JEDI missing value in every numeric column,
// so every conversion also exercises missing-value remapping.
constexpr std::size_t kMissingRow = 2;

// -----------------------------------------------------------------------------
// Build a frame with one column per supported type. Each numeric column carries a
// missing entry at kMissingRow. Values are chosen so numeric conversions are observable
// (e.g. float -> int truncates) and independent per type (the test never assumes two
// columns hold the same logical values).
void buildFrame(std::unique_ptr<osdf::IFrame> & frame) {
  const int missingInt = util::missingValue<int>();
  const std::int64_t missingInt64 = util::missingValue<std::int64_t>();
  const float missingFloat = util::missingValue<float>();
  const char missingChar = util::missingValue<char>();

  std::vector<int> intCol            = {10, 20, missingInt, 40};
  std::vector<std::int64_t> int64Col = {100, 200, missingInt64, 400};
  std::vector<float> floatCol        = {1.5f, 2.5f, missingFloat, 4.5f};
  std::vector<char> charCol          = {static_cast<char>(1), static_cast<char>(2),
                                        missingChar, static_cast<char>(4)};
  std::vector<std::string> strCol    = {"w", "x", "y", "z"};

  frame->appendNewColumn("intCol", intCol);
  frame->appendNewColumn("int64Col", int64Col);
  frame->appendNewColumn("floatCol", floatCol);
  frame->appendNewColumn("charCol", charCol);
  frame->appendNewColumn("strCol", strCol);
}

// -----------------------------------------------------------------------------
// Read column \p name (stored type S, whose values are \p stored) back as requested type T and
// verify each element matches the coercion contract: a stored missing marker maps to the requested
// type's missing marker, everything else is a static_cast.
template <typename S, typename T>
void expectReadCoerces(const std::unique_ptr<osdf::IFrame> & frame, const std::string & name,
                       const std::vector<S> & stored) {
  const S srcMissing = util::missingValue<S>();
  const T dstMissing = util::missingValue<T>();

  std::vector<T> got;
  frame->getColumn(name, got);
  EXPECT(got.size() == stored.size());
  for (std::size_t r = 0; r < stored.size(); ++r) {
    const T expected = (stored[r] == srcMissing) ? dstMissing : static_cast<T>(stored[r]);
    EXPECT(got[r] == expected);
  }
}

// Read column \p name back as every supported numeric type and confirm coercion; then confirm that
// requesting it as a string (string<->numeric) is rejected.
template <typename S>
void expectAllNumericReads(const std::unique_ptr<osdf::IFrame> & frame, const std::string & name,
                           const std::vector<S> & stored) {
  oops::Log::info() << "  reading column '" << name << "' as each numeric type" << std::endl;
  expectReadCoerces<S, int>(frame, name, stored);
  expectReadCoerces<S, std::int64_t>(frame, name, stored);
  expectReadCoerces<S, float>(frame, name, stored);
  expectReadCoerces<S, char>(frame, name, stored);

  std::vector<std::string> asString;
  EXPECT_THROWS_AS(frame->getColumn(name, asString), eckit::BadParameter);
}

// -----------------------------------------------------------------------------
void testReadCoercion(std::unique_ptr<osdf::IFrame> & frame) {
  buildFrame(frame);

  const int missingInt = util::missingValue<int>();
  const std::int64_t missingInt64 = util::missingValue<std::int64_t>();
  const float missingFloat = util::missingValue<float>();
  const char missingChar = util::missingValue<char>();

  expectAllNumericReads<int>(frame, "intCol",
                             {10, 20, missingInt, 40});
  expectAllNumericReads<std::int64_t>(frame, "int64Col",
                             {100, 200, missingInt64, 400});
  expectAllNumericReads<float>(frame, "floatCol",
                             {1.5f, 2.5f, missingFloat, 4.5f});
  expectAllNumericReads<char>(frame, "charCol",
                             {static_cast<char>(1), static_cast<char>(2),
                              missingChar, static_cast<char>(4)});

  // Reading a string column into any numeric vector must be rejected, not silently coerced.
  std::vector<int> strAsInt;
  EXPECT_THROWS_AS(frame->getColumn("strCol", strAsInt), eckit::BadParameter);
  std::vector<float> strAsFloat;
  EXPECT_THROWS_AS(frame->getColumn("strCol", strAsFloat), eckit::BadParameter);

  // The exact-type read remains available and unchanged (fast path).
  std::vector<float> floatExact;
  frame->getColumn("floatCol", floatExact);
  EXPECT(floatExact.size() == 4);
  EXPECT(floatExact[0] == 1.5f);
  EXPECT(floatExact[kMissingRow] == missingFloat);
}

// -----------------------------------------------------------------------------
void testWriteCoercion(std::unique_ptr<osdf::IFrame> & frame) {
  buildFrame(frame);

  const int missingInt = util::missingValue<int>();
  const float missingFloat = util::missingValue<float>();

  // Overwrite the int column using a float vector. Values must be coerced to the stored int type,
  // truncating fractionals and remapping the float missing marker to the int missing marker.
  std::vector<float> newValues = {7.9f, missingFloat, 9.1f, 10.0f};
  frame->setColumn("intCol", newValues);

  std::vector<int> readBack;
  frame->getColumn("intCol", readBack);   // stored type is still int: exact-type read
  EXPECT(readBack.size() == 4);
  EXPECT(readBack[0] == static_cast<int>(7.9f));   // 7
  EXPECT(readBack[1] == missingInt);               // float missing -> int missing
  EXPECT(readBack[2] == static_cast<int>(9.1f));   // 9
  EXPECT(readBack[3] == 10);

  // The column's stored type is unchanged by a coerced write.
  EXPECT(frame->getColumnType("intCol") == osdf::consts::eInt);

  // Writing strings into a numeric column crosses the string/numeric boundary and must throw.
  std::vector<std::string> strValues = {"1", "2", "3", "4"};
  EXPECT_THROWS_AS(frame->setColumn("intCol", strValues), eckit::BadParameter);
}

// -----------------------------------------------------------------------------
void testFrameColsReadCoercion() {
  std::unique_ptr<osdf::IFrame> frame = std::make_unique<osdf::FrameCols>();
  testReadCoercion(frame);
}

void testFrameRowsReadCoercion() {
  std::unique_ptr<osdf::IFrame> frame = std::make_unique<osdf::FrameRows>();
  testReadCoercion(frame);
}

void testFrameColsWriteCoercion() {
  std::unique_ptr<osdf::IFrame> frame = std::make_unique<osdf::FrameCols>();
  testWriteCoercion(frame);
}

void testFrameRowsWriteCoercion() {
  std::unique_ptr<osdf::IFrame> frame = std::make_unique<osdf::FrameRows>();
  testWriteCoercion(frame);
}

// -----------------------------------------------------------------------------
class OsdfTypeCoercion : public oops::Test {
 public:
  OsdfTypeCoercion() {}
  virtual ~OsdfTypeCoercion() {}

 private:
  std::string testid() const override {return "test::OsdfTypeCoercion";}

  void register_tests() const override {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    ts.emplace_back(CASE("ioda/TypeCoercion/testFrameColsReadCoercion")
      { testFrameColsReadCoercion(); });
    ts.emplace_back(CASE("ioda/TypeCoercion/testFrameRowsReadCoercion")
      { testFrameRowsReadCoercion(); });
    ts.emplace_back(CASE("ioda/TypeCoercion/testFrameColsWriteCoercion")
      { testFrameColsWriteCoercion(); });
    ts.emplace_back(CASE("ioda/TypeCoercion/testFrameRowsWriteCoercion")
      { testFrameRowsWriteCoercion(); });
  }

  void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda
