/*
 * (C) Crown copyright 2026, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#pragma once

#include <string>
#include <utility>
#include <vector>

#define ECKIT_TESTING_SELF_REGISTER_CASES 0

#include "ioda/test/containers/OsdfTestUtils.h"
#include "oops/runs/Test.h"

namespace ioda {
namespace test {

void testVarDimNames() {
  std::vector<std::pair<std::string, std::vector<std::string>>> varDimNamesToTest
    = {{"channelOnlyVar", {"Channel"}},
       {"chanLocVar", {"Location", "Channel"}},
       {"chanOtherVar", {"Channel", "Other"}},
       {"chanMultiOtherVar", {"Channel", "Other1", "Other2"}}};

  osdf::FrameMetadata testOsdfMetadata;

  // add varDimNames
  for (auto &item : varDimNamesToTest) {
    testOsdfMetadata.addVarDimNames(item.first, item.second);
  }

  // getVarDimNames for each of the test items
  for (auto &item : varDimNamesToTest) {
    std::vector<std::string> testVarDimNames = testOsdfMetadata.getVarDimNames(item.first);
    std::vector<std::string> refVarDimNames  = item.second;
    EXPECT(testVarDimNames.size() == refVarDimNames.size());

    for (size_t index = 0; index < testVarDimNames.size(); ++index) {
      EXPECT(testVarDimNames[index] == refVarDimNames[index]);
    }
  }

  // test getVarDimNames for variable not present
  EXPECT_THROWS_AS(testOsdfMetadata.getVarDimNames("missingVar"), eckit::BadValue);

  // test remove variable not present
  EXPECT(!testOsdfMetadata.removeVarDimNames("missingVar"));

  // test removeVarDimNames by calling this and then getVarDimNames
  for (auto &item : varDimNamesToTest) {
    EXPECT(testOsdfMetadata.removeVarDimNames(item.first));
    EXPECT_THROWS_AS(testOsdfMetadata.getVarDimNames(item.first), eckit::BadValue);
  }
}

// -----------------------------------------------------------------------------
class OsdfMetadataVarDimNames : public oops::Test {
 public:
  OsdfMetadataVarDimNames() {}
  virtual ~OsdfMetadataVarDimNames() {}

 private:
  std::string testid() const override { return "test::OsdfMetadataVarDimNames"; }

  void register_tests() const override {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    ts.emplace_back(CASE("ioda/OsdfFrameMetadata/varDimNames") { testVarDimNames(); });
  }

  void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda
