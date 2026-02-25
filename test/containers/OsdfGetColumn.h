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

void testGetColumn(std::unique_ptr<osdf::IFrame> & testFrame) {
  const std::vector<eckit::LocalConfiguration> configColumnData =
      ::test::TestEnvironment::config().getSubConfigurations("test column data");
  std::vector<std::string> testColumnNames, testColumnTypes;
  populateFrame(configColumnData, testFrame, testColumnNames, testColumnTypes);
  for (std::size_t i = 0; i < testColumnNames.size(); ++i) {
    const auto columnType = testFrame->getColumnType(testColumnNames[i]);
    oops::Log::info() << "testGetColumn: column name: " << testColumnNames[i] << std::endl;
    // Get the column data and compare to expected values.
    osdf::FrameUtils::callWithSupportedType(
      columnType,
      [&](auto typeDiscriminator) {
        using T = decltype(typeDiscriminator);
        const std::vector<T>& expectedColumn = replaceMissingValues<T>(
          ::test::TestEnvironment::config().getStringVector("expected column " +
          std::to_string(i)));
        std::vector<T> testColumn;
        testFrame->getColumn(testColumnNames[i], testColumn);
        EXPECT(testColumn == expectedColumn);
      });
    // Try to get the column data using the wrong vector type and check that an error is thrown.
    const int8_t wrongType = (columnType + 1) % osdf::consts::eNumberOfDataTypes;
    osdf::FrameUtils::callWithSupportedType(
      wrongType,
      [&](auto typeDiscriminator) {
        using T = decltype(typeDiscriminator);
        std::vector<T> wrongTypeColumn;
        EXPECT_THROWS_AS(testFrame->getColumn(testColumnNames[i], wrongTypeColumn),
          eckit::BadParameter);
      });
  }
}

void testFrameColsGetColumn() {
  std::unique_ptr<osdf::IFrame> testFrame = std::make_unique<osdf::FrameCols>();
  testGetColumn(testFrame);
}

void testFrameRowsGetColumn() {
  std::unique_ptr<osdf::IFrame> testFrame = std::make_unique<osdf::FrameRows>();
  testGetColumn(testFrame);
}

// -----------------------------------------------------------------------------
class OsdfGetColumn : public oops::Test {
 public:
  OsdfGetColumn() {}
  virtual ~OsdfGetColumn() {}

 private:
  std::string testid() const override {return "test::OsdfGetColumn";}

  void register_tests() const override {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    ts.emplace_back(CASE("ioda/ReaderFilter/testFrameColsGetColumn")
      { testFrameColsGetColumn(); });
    ts.emplace_back(CASE("ioda/ReaderFilter/testFrameRowsGetColumn")
      { testFrameRowsGetColumn(); });
  }

  void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda
