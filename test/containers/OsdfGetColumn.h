/*
 * (C) Copyright 2026 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#pragma once

#include <memory>
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
    // Try to get the column data using a different vector type. Numeric<->numeric requests are
    // transparently coerced now (see the OsdfTypeCoercion test for value-level checks); only a
    // request that crosses the string/numeric boundary is still rejected.
    const std::size_t expectedSize =
      ::test::TestEnvironment::config().getStringVector("expected column " +
      std::to_string(i)).size();
    // select the next type in the enum, wrapping around to 0 if necessary, to ensure that
    // we are testing a different type than the one used to store the column data.
    const osdf::consts::eDataTypes wrongType
      = static_cast<osdf::consts::eDataTypes>((columnType + 1) % osdf::consts::eNumberOfDataTypes);
    osdf::FrameUtils::callWithSupportedType(
      wrongType,
      [&](auto typeDiscriminator) {
        using T = decltype(typeDiscriminator);
        std::vector<T> wrongTypeColumn;
        if (columnType == osdf::consts::eString || wrongType == osdf::consts::eString) {
          EXPECT_THROWS_AS(testFrame->getColumn(testColumnNames[i], wrongTypeColumn),
            eckit::BadParameter);
        } else {
          // Coercion between numeric types must succeed and preserve the element count.
          testFrame->getColumn(testColumnNames[i], wrongTypeColumn);
          EXPECT(wrongTypeColumn.size() == expectedSize);
        }
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
