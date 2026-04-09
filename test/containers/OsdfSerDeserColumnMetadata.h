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
#include "ioda/containers/FrameUtils.h"
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

    // Serialize the column metadata and check against expected values
    const std::string serializedColumnMetadata =
      osdf::FrameUtils::serializeColumnMetadata(colMetadata.get());
    const std::vector<std::string> serializedTokens =
      ioda::splitString(serializedColumnMetadata, osdf::consts::columnSeparatorChar);

    const std::vector<std::string> expectedTokens =
      testCaseConfig.getStringVector("expected values.serialized column metadata.tokens");
    EXPECT(serializedTokens == expectedTokens);

    // Check deserialization. For this join the expected tokens from the test
    // configuration into a space separated string, and deserialize that.
    const std::string testSerString =
      ioda::joinString(expectedTokens, osdf::consts::columnSeparatorString);
    osdf::ColumnMetadata testColMetadata;
    testColMetadata.add(osdf::FrameUtils::deserializeColumnMetadataTokens(testSerString));
    colMetadata.validateColumnMetadata(testColMetadata);
    colMetadata.validateColumnMetadataPermissions(testColMetadata);
  }
}

// -----------------------------------------------------------------------------
void testColumnMetadataWithErrors() {
  // At this point only the deserializeColumnMetadataTokens function detects
  // error conditions.
  const std::vector<eckit::LocalConfiguration> & testCasesConfig =
    ::test::TestEnvironment::config().getSubConfigurations("test cases");

  for (std::size_t i = 0; i < testCasesConfig.size(); ++i) {
    const eckit::LocalConfiguration & testCaseConfig = testCasesConfig[i];
    std::string testCaseName = testCaseConfig.getString("name");
    oops::Log::info() << "Testing: " << testCaseName << std::endl;

    // Grab the tokens for deserialization from the test configuration
    const std::vector<std::string> deserializationTokens =
      testCaseConfig.getStringVector("expected values.serialized column metadata.tokens");

    // The first token needs to be set to "columns:<number of columns>"
    // An exception is throw for the following:
    //  1. if the first part is not equal to "columns"
    //  2. if the first part is "columns", but the number in the second part is
    //     not set to the number of remaining tokens (ie the eventual number of columns).
    std::vector<std::string> testTokens = deserializationTokens;
    testTokens[0] = std::string("XXX:1");  // not equal to "columns in first part"
    std::string testSerString = ioda::joinString(testTokens, " ");
    osdf::ColumnMetadata testColMetadata;
    EXPECT_THROWS_AS(
      testColMetadata.add(osdf::FrameUtils::deserializeColumnMetadataTokens(testSerString)),
      eckit::BadValue);

    testTokens = deserializationTokens;
    testTokens[0] = std::string("columns:-1");  // not the number of remaining tokens
    testSerString = ioda::joinString(testTokens, " ");
    EXPECT_THROWS_AS(
      testColMetadata.add(osdf::FrameUtils::deserializeColumnMetadataTokens(testSerString)),
      eckit::BadValue);

    // One more error condition is that each remaining token (after the first token)
    // needs to have exactly 5 parts (which line up with all of the column metadata
    // pieces).
    testTokens = deserializationTokens;
    testTokens[1] += std::string(":xxx:yyy");  // two extra pieces (7 instead of 5 pieces)
    testSerString = ioda::joinString(testTokens, " ");
    EXPECT_THROWS_AS(
      testColMetadata.add(osdf::FrameUtils::deserializeColumnMetadataTokens(testSerString)),
      eckit::BadValue);
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
    ts.emplace_back(CASE("testColumnMetatdataWithErrors") { testColumnMetadataWithErrors(); });
  }

  void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda
