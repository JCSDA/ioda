/*
 * (C) Copyright 2026, UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#pragma once

#include <string>
#include <vector>

#define ECKIT_TESTING_SELF_REGISTER_CASES 0

#include "eckit/config/LocalConfiguration.h"
#include "eckit/io/Buffer.h"
#include "eckit/serialisation/ResizableMemoryStream.h"

#include "ioda/containers/FrameMetadata.h"
#include "ioda/test/containers/OsdfTestUtils.h"

#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"
#include "oops/util/Logger.h"

namespace ioda {
namespace test {

// -----------------------------------------------------------------------------
void testFrameMetadataEquality() {
  // Testing the FrameData == operator.
  const std::vector<eckit::LocalConfiguration> testConfigs =
      ::test::TestEnvironment::config().getSubConfigurations("test cases");
  for (const auto & testConfig : testConfigs) {
    const std::string testCaseName = testConfig.getString("name");
    oops::Log::info() << "Test case: " << testCaseName << std::endl;

    // Create matching frame data objects and check that they do
    // indeed match. Then change one and check that they do not
    // match.
    const eckit::LocalConfiguration frameMetadataConfig =
      testConfig.getSubConfiguration("frame metadata");
    osdf::FrameMetadata frameMetadata1;
    osdf::FrameMetadata frameMetadata2;
    populateFrameMetadata(frameMetadataConfig, frameMetadata1);
    populateFrameMetadata(frameMetadataConfig, frameMetadata2);
    EXPECT(frameMetadata1 == frameMetadata2);

    frameMetadata2.addVarToVarsWithChans("MetaData/myVarWithChans");
    frameMetadata2.addVarDimNames("MetaData/myVarWithChans", { "Location", "Channel" });
    EXPECT(!(frameMetadata1 == frameMetadata2));
  }
}

// -----------------------------------------------------------------------------
void testSerDeserFrameMetadata() {
  const std::vector<eckit::LocalConfiguration> testConfigs =
      ::test::TestEnvironment::config().getSubConfigurations("test cases");
  for (const auto & testConfig : testConfigs) {
    const std::string testCaseName = testConfig.getString("name");
    oops::Log::info() << "Test case: " << testCaseName << std::endl;

    // Create a source frame metadata, serialize it, then deserialize
    // into a destination frame metadata, and compare the two.
    const eckit::LocalConfiguration frameMetadataConfig =
      testConfig.getSubConfiguration("frame metadata");
    osdf::FrameMetadata srcFrameMetadata;
    populateFrameMetadata(frameMetadataConfig, srcFrameMetadata);

    // Serialize
    const std::size_t expectedBytesSerialized = testConfig.getUnsigned("bytes serialized");
    const std::size_t bufrSize = srcFrameMetadata.bufrSize();
    eckit::Buffer bufr(bufrSize);
    const std::size_t bytesSerialized = srcFrameMetadata.serialize(bufr);
    EXPECT_EQUAL(bytesSerialized, expectedBytesSerialized);

    // Deserialize
    osdf::FrameMetadata destFrameMetadata;
    destFrameMetadata.deserialize(bufr);

    // Check if serialize/deserialize worked
    EXPECT(destFrameMetadata == srcFrameMetadata);
  }
}

// -----------------------------------------------------------------------------
class OsdfSerDeserFrameMetadata : public oops::Test {
 public:
  OsdfSerDeserFrameMetadata() {}
  virtual ~OsdfSerDeserFrameMetadata() {}

 private:
  std::string testid() const override { return "test::OsdfSerDeserFrameMetadata"; }

  void register_tests() const override {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    ts.emplace_back(CASE("testFrameMetadataEquality") { testFrameMetadataEquality(); });
    ts.emplace_back(CASE("testSerDeserFrameMetadata") { testSerDeserFrameMetadata(); });
  }

  void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda
