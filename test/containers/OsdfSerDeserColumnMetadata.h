/*
 * (C) Crown copyright 2026, Met Office
 * (C) Copyright 2026, UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

#define ECKIT_TESTING_SELF_REGISTER_CASES 0

#include "ioda/containers/ColumnMetadata.h"
#include "ioda/containers/ColumnMetadatum.h"
#include "ioda/containers/Constants.h"
#include "ioda/core/IodaUtils.h"
#include "ioda/test/containers/OsdfTestUtils.h"

#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"
#include "oops/util/Logger.h"

namespace ioda {
namespace test {

// -----------------------------------------------------------------------------
void testColumnMetadata() {
  const std::vector<eckit::LocalConfiguration> & testCasesConfig =
    ::test::TestEnvironment::config().getSubConfigurations("test cases");

  for (std::size_t i = 0; i < testCasesConfig.size(); ++i) {
    const eckit::LocalConfiguration & testCaseConfig = testCasesConfig[i];
    std::string testCaseName = testCaseConfig.getString("name");
    oops::Log::info() << "Testing: " << testCaseName << std::endl;

    // Grab the osdf config which contains the column names, types, and values
    // to be used in the test.
    const std::vector<eckit::LocalConfiguration> & osdfColConfig =
      testCaseConfig.getSubConfigurations("osdf columns");

    osdf::ColumnMetadata colMetadata;
    populateColumnMetadata(osdfColConfig, colMetadata);

    const size_t expectedBufferSize =
      testCaseConfig.getUnsigned("expected values.buffer size");
    size_t bufferSize = osdf::ColumnMetadata::bufrSize(colMetadata);
    EXPECT(bufferSize == expectedBufferSize);

    eckit::Buffer buffer(bufferSize);
    const size_t expectedBytesWritten =
      testCaseConfig.getUnsigned("expected values.bytes written");
    size_t bytesWritten = osdf::ColumnMetadata::serialize(buffer, colMetadata);
    EXPECT(bytesWritten == expectedBytesWritten);

    // Check deserialization.
    osdf::ColumnMetadata testColMetadata;
    testColMetadata.add(osdf::ColumnMetadata::deserialize(buffer));
    colMetadata.validateColumnMetadata(testColMetadata);
    colMetadata.validateColumnMetadataPermissions(testColMetadata);
  }
}

// -----------------------------------------------------------------------------
class OsdfSerDeserColumnMetadata : public oops::Test {
 public:
  OsdfSerDeserColumnMetadata() {}
  virtual ~OsdfSerDeserColumnMetadata() {}

 private:
  std::string testid() const override { return "test::OsdfSerDeserColumnMetadata"; }

  void register_tests() const override {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    ts.emplace_back(CASE("testColumnMetadata") { testColumnMetadata(); });
  }

  void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda
