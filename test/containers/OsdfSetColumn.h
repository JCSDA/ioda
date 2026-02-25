/*
 * (C) Copyright 2026 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#pragma once

#include <memory>
#include <sstream>
#include <string>
#include <vector>

#define ECKIT_TESTING_SELF_REGISTER_CASES 0

#include <boost/noncopyable.hpp>

#include "eckit/exception/Exceptions.h"
#include "eckit/testing/Test.h"

#include "ioda/containers/FrameCols.h"
#include "ioda/containers/FrameRows.h"
#include "ioda/containers/FrameUtils.h"
#include "ioda/containers/IFrame.h"
#include "ioda/test/containers/OsdfTestUtils.h"

#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"

namespace ioda {
namespace test {

void testSetColumn(std::unique_ptr<osdf::IFrame> & testFrame) {
  // Set up test column data
  const std::vector<std::string> origStringValues{"string1", "string2", "string3"};
  const std::vector<std::string> newStringValues{"string4", "string5", "string2"};
  const std::vector<int> origIntValues{1, 2, 3};
  const std::vector<int> newIntValues{4, 5, 2};
  const std::vector<std::int64_t> origInt64Values{9000000000000000000, 9000000000000000001,
                                            9000000000000000002};
  const std::vector<std::int64_t> newInt64Values{9000000000000000003, 9000000000000000004,
                                           9000000000000000005};
  const std::vector<float> origFloatValues{1.1f, 2.2f, 3.3f};
  const std::vector<float> newFloatValues{4.4f, 5.5f, 2.2f};
  const std::vector<char> origCharValues{'a', 'b', 'c'};
  const std::vector<char> newCharValues{'d', 'e', 'b'};

  std::vector<std::string> testStringValues;
  std::vector<int> testIntValues;
  std::vector<std::int64_t> testInt64Values;
  std::vector<float> testFloatValues;
  std::vector<char> testCharValues;

  // Set up original columns in the frame
  testFrame->appendNewColumn("test string column", origStringValues);
  testFrame->appendNewColumn("test int column", origIntValues);
  testFrame->appendNewColumn("test int64 column", origInt64Values);
  testFrame->appendNewColumn("test float column", origFloatValues);
  testFrame->appendNewColumn("test char column", origCharValues);
  // TODO(vahl): Finish test of read-only column functionality once it is more fully implemented.
  // Currently there is no way to create a read-only column using the IFrame interface.
  // testFrame->appendNewColumn("test read-only column", origIntValues);,
  //   osdf::consts::eReadOnly);

  // Test basic setColumn functionality for each type
  testFrame->setColumn("test string column", newStringValues);
  testFrame->getColumn("test string column", testStringValues);
  EXPECT(testStringValues == newStringValues);
  testFrame->setColumn("test int column", newIntValues);
  testFrame->getColumn("test int column", testIntValues);
  EXPECT(testIntValues == newIntValues);
  testFrame->setColumn("test int64 column", newInt64Values);
  testFrame->getColumn("test int64 column", testInt64Values);
  EXPECT(testInt64Values == newInt64Values);
  testFrame->setColumn("test float column", newFloatValues);
  testFrame->getColumn("test float column", testFloatValues);
  EXPECT(testFloatValues == newFloatValues);
  testFrame->setColumn("test char column", newCharValues);
  testFrame->getColumn("test char column", testCharValues);
  EXPECT(testCharValues == newCharValues);

  // Test expected error conditions
  // Wrong type
  EXPECT_THROWS_AS(testFrame->setColumn("test string column", newIntValues),
                    eckit::BadParameter);
  // Wrong size
  const std::vector<int> wrongSizeIntValues{4, 5};
  EXPECT_THROWS_AS(testFrame->setColumn("test int column", wrongSizeIntValues),
                    eckit::BadParameter);
  // Read-only column
  // EXPECT_THROWS_AS(testFrame->setColumn("test read-only column", newIntValues),
  //                   eckit::BadParameter);
}

void testFrameColsSetColumn() {
  std::unique_ptr<osdf::IFrame> testFrame = std::make_unique<osdf::FrameCols>();
  testSetColumn(testFrame);
}

void testFrameRowsSetColumn() {
  std::unique_ptr<osdf::IFrame> testFrame = std::make_unique<osdf::FrameRows>();
  testSetColumn(testFrame);
}

// -----------------------------------------------------------------------------
class OsdfSetColumn : public oops::Test {
 public:
  OsdfSetColumn() {}
  virtual ~OsdfSetColumn() {}

 private:
  std::string testid() const override {return "test::OsdfSetColumn";}

  void register_tests() const override {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    ts.emplace_back(CASE("ioda/ReaderFilter/testFrameColsSetColumn")
      { testFrameColsSetColumn(); });
    ts.emplace_back(CASE("ioda/ReaderFilter/testFrameRowsSetColumn")
      { testFrameRowsSetColumn(); });
  }

  void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda
