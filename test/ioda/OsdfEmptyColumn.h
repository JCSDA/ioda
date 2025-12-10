/*
 * (C) Copyright 2025 UCAR
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

#include "ioda/containers/Constants.h"
#include "ioda/containers/FrameCols.h"
#include "ioda/containers/FrameMetadata.h"
#include "ioda/containers/FrameRows.h"
#include "ioda/containers/IFrame.h"
#include "ioda/test/ioda/OsdfTestUtils.h"

#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"
#include "oops/util/Logger.h"

namespace ioda {
namespace test {

void testEmptyColumn(std::unique_ptr<osdf::IFrame> & testFrame) {
    testFrame->appendNewColumn("EmptyColumnInt", std::vector<int>{});
    EXPECT_EQUAL(testFrame->columnNames().back(), "EmptyColumnInt");
    EXPECT_EQUAL(testFrame->getColumnType("EmptyColumnInt"), osdf::consts::eDataTypes::eInt);
    EXPECT_EQUAL(testFrame->numRows(), 0);
    EXPECT_EQUAL(testFrame->numCols(), 1);
    testFrame->appendNewColumn("EmptyColumnString", std::vector<std::string>{});
    EXPECT_EQUAL(testFrame->columnNames().back(), "EmptyColumnString");
    EXPECT_EQUAL(testFrame->numRows(), 0);
    EXPECT_EQUAL(testFrame->getColumnType("EmptyColumnString"), osdf::consts::eDataTypes::eString);
    EXPECT_EQUAL(testFrame->getColumnType("EmptyColumnInt"), osdf::consts::eInt);
    EXPECT_EQUAL(testFrame->numCols(), 2);
}

void testFrameColsEmptyColumn() {
  std::unique_ptr<osdf::IFrame> testFrame = std::make_unique<osdf::FrameCols>();
  testEmptyColumn(testFrame);
}

void testFrameRowsEmptyColumn() {
  std::unique_ptr<osdf::IFrame> testFrame = std::make_unique<osdf::FrameRows>();
  testEmptyColumn(testFrame);
}

// -----------------------------------------------------------------------------
class OsdfEmptyColumn : public oops::Test {
 public:
  OsdfEmptyColumn() {}
  virtual ~OsdfEmptyColumn() {}

 private:
  std::string testid() const override {return "test::OsdfEmptyColumn";}

  void register_tests() const override {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    ts.emplace_back(CASE("ioda/ReaderFilter/testFrameColsEmptyColumn")
      { testFrameColsEmptyColumn(); });
    ts.emplace_back(CASE("ioda/ReaderFilter/testFrameRowsEmptyColumn")
      { testFrameRowsEmptyColumn(); });
  }

  void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda
