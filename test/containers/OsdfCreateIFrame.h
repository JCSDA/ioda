/*
 * (C) Crown copyright 2026, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#pragma once

#include <memory>
#include <string>
#include <vector>
#include "eckit/exception/Exceptions.h"

#define ECKIT_TESTING_SELF_REGISTER_CASES 0

#include "ioda/containers/CreateIFrame.h"
#include "ioda/containers/FrameCols.h"
#include "ioda/containers/FrameRows.h"
#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"

namespace ioda {
namespace test {

void testIFrameGetType() {
  std::unique_ptr<osdf::IFrame> frameCols;
  frameCols = std::make_unique<osdf::FrameCols>();
  EXPECT_EQUAL(frameCols->frameType(), "FrameCols");

  std::unique_ptr<osdf::IFrame> frameRows;
  frameRows = std::make_unique<osdf::FrameRows>();
  EXPECT_EQUAL(frameRows->frameType(), "FrameRows");
}

void testIFrameSetType() {
  std::unique_ptr<osdf::IFrame> frameCols = osdf::createIFrame("FrameCols");
  EXPECT_EQUAL(frameCols->frameType(), "FrameCols");

  std::unique_ptr<osdf::IFrame> frameRows = osdf::createIFrame("FrameRows");
  EXPECT_EQUAL(frameRows->frameType(), "FrameRows");

  EXPECT_THROWS_AS(std::unique_ptr<osdf::IFrame> badSetFrame
                     = osdf::createIFrame("notValidType"),
                   eckit::BadParameter);
}



// -----------------------------------------------------------------------------
class OsdfCreateIFrame : public oops::Test {
 public:
  OsdfCreateIFrame() {}
  virtual ~OsdfCreateIFrame() {}

 private:
  std::string testid() const override { return "test::OsdfCreateIFrame"; }

  void register_tests() const override {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    ts.emplace_back(CASE("ioda/OsdfStandaloneFunctions/testOsdfCreateIFrame")
                    { testIFrameGetType(); });
    ts.emplace_back(CASE("ioda/OsdfStandaloneFunctions/testOsdfCreateIFrame")
                    { testIFrameSetType(); });
  }

  void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda
