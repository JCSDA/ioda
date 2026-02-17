/*
 * (C) Crown copyright 2026, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "eckit/exception/Exceptions.h"

#define ECKIT_TESTING_SELF_REGISTER_CASES 0

#include "ioda/containers/FrameCols.h"
#include "ioda/containers/FrameRows.h"
#include "ioda/test/containers/OsdfTestUtils.h"
#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"
#include "oops/util/Logger.h"

namespace ioda {
namespace test {

std::int8_t testCompareTwoDataRows(osdf::DataRow testRow, osdf::DataRow compareRow) {
  const std::int32_t testSize = testRow.getSize();
  const std::int32_t compareSize = compareRow.getSize();

  if (testSize != compareSize) {
    return false;
  }

  for (std::int32_t index = 0; index < testSize; ++index) {
    std::string testString    = testRow.getColumn(index)->getValueStr();
    std::string compareString = compareRow.getColumn(index)->getValueStr();
    if (testString != compareString) {
      return false;
    }
  }
  return true;
}

void testOsdfAppend(std::unique_ptr<osdf::IFrame>& frame1,
                    const std::unique_ptr<osdf::IFrame>& frame2) {
  std::int64_t frame1Size = frame1->numRows();
  std::int64_t frame2Size = frame2->numRows();
  std::int64_t frame1MaxId = frame1->getData().getMaxId();

  frame1->append(frame2);
  EXPECT_EQUAL(frame1->numRows(), frame1Size + frame2Size);  // check size
  EXPECT_EQUAL(frame1->getData().getMaxId(), frame1MaxId + frame2Size);

  std::vector<osdf::DataRow> dataRows1;
  dataRows1.reserve(frame1->getData().getSizeRows());
  frame1->getData().getDataRows(dataRows1);

  std::vector<osdf::DataRow> dataRows2;
  dataRows2.reserve(frame2->getData().getSizeRows());
  frame2->getData().getDataRows(dataRows2);

  for (std::int64_t index = 0; index < frame2Size; ++index) {
    EXPECT_EQUAL(testCompareTwoDataRows(dataRows1.at(frame1Size + index),
                                        dataRows2.at(index)), 1);  // check entries match
  }
}

void testOsdfAppendEmpty(std::unique_ptr<osdf::IFrame>& frame1) {
  std::int64_t frame1Size = frame1->numRows();

  std::unique_ptr<osdf::IFrame> emptyFrame;
  emptyFrame = std::make_unique<osdf::FrameRows>();

  bool didAppendWork = true;
  try {
    frame1->append(emptyFrame);
  } catch (eckit::BadParameter) {
    didAppendWork = false;
  }
  EXPECT_EQUAL(didAppendWork, false);
  EXPECT_EQUAL(frame1->numRows(), frame1Size);  // Check no additional rows added
}

void testOsdfAppendPasses() {
  // Set up frameCols1
  std::unique_ptr<osdf::IFrame> frameCols1;
  frameCols1 = std::make_unique<osdf::FrameCols>();
  std::vector<float> lats1          = {-65.0, -66.6};
  std::vector<std::string> statIds1 = {"00001", "00001"};
  std::vector<std::int64_t> times1  = {1710460225, 1710460225};
  frameCols1->appendNewColumn("lat", lats1);
  frameCols1->appendNewColumn("StatId", statIds1);
  frameCols1->appendNewColumn("time", times1);

  // Set up frameCols2
  std::unique_ptr<osdf::IFrame> frameCols2;
  frameCols2 = std::make_unique<osdf::FrameCols>();
  std::vector<float> lats2          = {-67.2, -68.6, -64.8};
  std::vector<std::string> statIds2 = {"00002", "00001", "00001"};
  std::vector<std::int64_t> times2  = {1710460225, 1710460225, 1710460225};
  frameCols2->appendNewColumn("lat", lats2);
  frameCols2->appendNewColumn("StatId", statIds2);
  frameCols2->appendNewColumn("time", times2);

  // Set up frameCols3
  std::unique_ptr<osdf::IFrame> frameCols3;
  frameCols3 = std::make_unique<osdf::FrameCols>();
  std::vector<float> lats3 = {};
  std::vector<std::string> statIds3 = {};
  std::vector<std::int64_t> times3 = {};
  frameCols3->appendNewColumn("lat", lats3);
  frameCols3->appendNewColumn("StatId", statIds3);
  frameCols3->appendNewColumn("time", times3);

  // Set up frameRows1
  std::unique_ptr<osdf::IFrame> frameRows1;
  frameRows1 = std::make_unique<osdf::FrameRows>();
  frameRows1->appendNewColumn("lat", lats1);
  frameRows1->appendNewColumn("StatId", statIds1);
  frameRows1->appendNewColumn("time", times1);

  // Set up frameRows2
  std::unique_ptr<osdf::IFrame> frameRows2;
  frameRows2 = std::make_unique<osdf::FrameRows>();
  frameRows2->appendNewColumn("lat", lats2);
  frameRows2->appendNewColumn("StatId", statIds2);
  frameRows2->appendNewColumn("time", times2);

  // Make calls to testOsdfAppend...
  testOsdfAppend(frameCols1, frameCols2);
  testOsdfAppend(frameCols1, frameRows2);
  testOsdfAppend(frameRows1, frameRows2);
  testOsdfAppend(frameRows1, frameCols2);

  // Test when one of the frames is empty, with metadata
  testOsdfAppend(frameCols3, frameCols1);
  testOsdfAppend(frameCols1, frameCols3);
}

void testOsdfAppendFails() {
  std::unique_ptr<osdf::IFrame> emptyFrameCols;
  emptyFrameCols = std::make_unique<osdf::FrameCols>();

  // Set up frameCols1
  std::unique_ptr<osdf::IFrame> frameCols1;
  frameCols1 = std::make_unique<osdf::FrameCols>();
  std::vector<float> lats1          = {-65.0, -66.6};
  std::vector<std::string> statIds1 = {"00001", "00001"};
  std::vector<std::int64_t> times1  = {1710460225, 1710460225};
  frameCols1->appendNewColumn("lat", lats1);
  frameCols1->appendNewColumn("StatId", statIds1);
  frameCols1->appendNewColumn("time", times1);

  // Set up frameRows1
  std::unique_ptr<osdf::IFrame> frameRows1;
  frameRows1 = std::make_unique<osdf::FrameRows>();
  frameRows1->appendNewColumn("lat", lats1);
  frameRows1->appendNewColumn("StatId", statIds1);
  frameRows1->appendNewColumn("time", times1);

  // Set up frameCols2
  std::unique_ptr<osdf::IFrame> frameCols2;
  frameCols2 = std::make_unique<osdf::FrameCols>();
  std::vector<float> lats2          = {-67.2, -68.6, -64.8};
  std::vector<std::string> statIds2 = {"00002", "00001", "00001"};
  std::vector<std::int64_t> times2  = {1710460225, 1710460225, 1710460225};
  frameCols2->appendNewColumn("latitude", lats2);
  frameCols2->appendNewColumn("StatId", statIds2);
  frameCols2->appendNewColumn("time", times2);

  // Append to empty frame without metadata fails
  bool didAppendWork = true;
  try {
    emptyFrameCols->append(frameCols1);
  }
  catch(eckit::BadParameter) {
    didAppendWork = false;
  }
  EXPECT_EQUAL(didAppendWork, false);
  EXPECT_EQUAL(emptyFrameCols->numRows(), 0);

  // Appending an empty frame without metadata to current frame fails
  testOsdfAppendEmpty(frameCols1);
  testOsdfAppendEmpty(frameRows1);

  // Append to frame with different metadata (names) fails
  std::int64_t frameCols2Size = frameCols2->numRows();
  didAppendWork               = true;
  try {
    frameCols2->append(frameCols1);
  } catch (eckit::BadParameter) {
    didAppendWork = false;
  }
  EXPECT_EQUAL(didAppendWork, false);
  EXPECT_EQUAL(frameCols2Size, frameCols2->numRows());
}

// -----------------------------------------------------------------------------
class OsdfAppend : public oops::Test {
 public:
  OsdfAppend() {}
  virtual ~OsdfAppend() {}

 private:
  std::string testid() const override { return "test::OsdfAppend"; }

  void register_tests() const override {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    ts.emplace_back(CASE("ioda/ReaderFilter/testOsdfAppend") { testOsdfAppendPasses(); });
    ts.emplace_back(CASE("ioda/ReaderFilter/testOsdfAppendFails") { testOsdfAppendFails(); });
  }

  void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda
