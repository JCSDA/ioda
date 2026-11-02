/*
 * (C) Crown copyright 2026 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#define ECKIT_TESTING_SELF_REGISTER_CASES 0

#include "eckit/testing/Test.h"

#include "ioda/containers/Constants.h"
#include "ioda/containers/CreateIFrame.h"
#include "ioda/containers/IFrame.h"
#include "ioda/core/IodaUtils.h"
#include "oops/runs/Test.h"
#include "oops/util/Logger.h"
#include "oops/util/missingValues.h"

namespace ioda {
namespace test {

// -----------------------------------------------------------------------------
// The utility under test, osdfAlignColumnsForAppend, prepares a frame for appending to a
// target frame that has more columns (eg, derived variables created after the target
// was read in). The OSDF append matches columns by position, so the prepared frame must
// hold the target's columns in the target's order.

// -----------------------------------------------------------------------------
/// \brief create the target frame
/// \details The columns are deliberately not in alphabetical order, so that a test
///          cannot pass by sorting the column names, and they cover every column type.
std::unique_ptr<osdf::IFrame> createTargetFrame(const std::string & frameType) {
  std::unique_ptr<osdf::IFrame> target = osdf::createIFrame(frameType);
  target->appendNewColumn("MetaData/latitude", std::vector<float>{ 10.0, 20.0, 30.0 },
                          "degrees_north");
  target->appendNewColumn("ObsValue/airTemperature", std::vector<float>{ 270.0, 271.0, 272.0 },
                          "K");
  target->appendNewColumn("MetaData/stationIdentification",
                          std::vector<std::string>{ "stn1", "stn2", "stn3" });
  target->appendNewColumn("EffectiveQC0/airTemperature", std::vector<int>{ 0, 1, 2 });
  target->appendNewColumn("MetaData/dateTime", std::vector<std::int64_t>{ 100, 200, 300 },
                          "seconds since 1970-01-01T00:00:00Z");
  target->appendNewColumn("DiagnosticFlags/airTemperature", std::vector<char>{ 0, 1, 0 });
  target->appendNewColumn("DerivedObsValue/airTemperature",
                          std::vector<float>{ 280.0, 281.0, 282.0 }, "K");
  return target;
}

// -----------------------------------------------------------------------------
/// \brief check that a column holds only missing values, with the target's type and units
template <typename T>
void checkMissingColumn(const osdf::IFrame & target, const osdf::IFrame & aligned,
                        const std::string & column) {
  EXPECT_EQUAL(aligned.getColumnType(column), target.getColumnType(column));
  EXPECT_EQUAL(aligned.getColumnUnits(column), target.getColumnUnits(column));
  std::vector<T> values;
  aligned.getColumn(column, values);
  EXPECT_EQUAL(values.size(), aligned.numRows());
  for (const T & value : values) {
    EXPECT(value == util::missingValue<T>());
  }
}

// -----------------------------------------------------------------------------
void testAlignColumns() {
  for (const std::string frameType : { "FrameCols", "FrameRows" }) {
    oops::Log::info() << "Testing frame type: " << frameType << std::endl;
    const std::unique_ptr<osdf::IFrame> target = createTargetFrame(frameType);

    // The source frame holds two of the target columns, neither adjacent to the other in
    // the target, and given in the opposite order to the target. The source units for
    // latitude differ from the target to show the source keeps its own units.
    std::unique_ptr<osdf::IFrame> src = osdf::createIFrame(frameType);
    src->appendNewColumn("MetaData/dateTime", std::vector<std::int64_t>{ 400, 500 },
                         "seconds since 1970-01-01T00:00:00Z");
    src->appendNewColumn("MetaData/latitude", std::vector<float>{ 40.0, 50.0 }, "degrees");

    osdfAlignColumnsForAppend(*target, src);

    // The columns are those of the target, in the target's order, and no rows were added.
    EXPECT(src->columnNames() == target->columnNames());
    EXPECT_EQUAL(src->numRows(), 2);
    EXPECT_EQUAL(src->frameType(), frameType);

    // The source columns keep their own values, type and units.
    std::vector<std::int64_t> dateTimes;
    src->getColumn("MetaData/dateTime", dateTimes);
    EXPECT(dateTimes == std::vector<std::int64_t>({ 400, 500 }));
    EXPECT_EQUAL(src->getColumnType("MetaData/dateTime"), osdf::consts::eInt64);
    EXPECT_EQUAL(src->getColumnUnits("MetaData/dateTime"),
                 std::string("seconds since 1970-01-01T00:00:00Z"));

    std::vector<float> latitudes;
    src->getColumn("MetaData/latitude", latitudes);
    EXPECT(latitudes == std::vector<float>({ 40.0, 50.0 }));
    EXPECT_EQUAL(src->getColumnType("MetaData/latitude"), osdf::consts::eFloat);
    EXPECT_EQUAL(src->getColumnUnits("MetaData/latitude"), std::string("degrees"));

    // The columns that were not in the source hold missing values, with the target's
    // type and units.
    checkMissingColumn<float>(*target, *src, "ObsValue/airTemperature");
    checkMissingColumn<std::string>(*target, *src, "MetaData/stationIdentification");
    checkMissingColumn<int>(*target, *src, "EffectiveQC0/airTemperature");
    checkMissingColumn<char>(*target, *src, "DiagnosticFlags/airTemperature");
    checkMissingColumn<float>(*target, *src, "DerivedObsValue/airTemperature");
  }
}

// -----------------------------------------------------------------------------
void testColumnNotInTarget() {
  for (const std::string frameType : { "FrameCols", "FrameRows" }) {
    oops::Log::info() << "Testing frame type: " << frameType << std::endl;
    const std::unique_ptr<osdf::IFrame> target = createTargetFrame(frameType);

    // A source column the target does not have is an error, even when the source has
    // fewer columns than the target.
    std::unique_ptr<osdf::IFrame> src = osdf::createIFrame(frameType);
    src->appendNewColumn("MetaData/latitude", std::vector<float>{ 40.0 }, "degrees_north");
    src->appendNewColumn("fish", std::vector<float>{ 1.0 }, "fishUnit");
    EXPECT_THROWS_AS(osdfAlignColumnsForAppend(*target, src), eckit::BadParameter);
  }
}

// -----------------------------------------------------------------------------
void testSourceNotFewerColumns() {
  for (const std::string frameType : { "FrameCols", "FrameRows" }) {
    oops::Log::info() << "Testing frame type: " << frameType << std::endl;
    const std::unique_ptr<osdf::IFrame> target = createTargetFrame(frameType);

    // When the source has as many or more columns than the target, the source is left
    // as is (in its own column order) for the append itself to handle.
    std::unique_ptr<osdf::IFrame> src = createTargetFrame(frameType);
    src->appendNewColumn("fish", std::vector<float>{ 1.0, 2.0, 3.0 }, "fishUnit");
    const osdf::IFrame * srcBefore = src.get();
    const std::vector<std::string> srcColumnsBefore = src->columnNames();

    osdfAlignColumnsForAppend(*target, src);

    EXPECT(src.get() == srcBefore);
    EXPECT(src->columnNames() == srcColumnsBefore);
  }
}

// -----------------------------------------------------------------------------
class OsdfAlignColumnsForAppend : public oops::Test {
 public:
  OsdfAlignColumnsForAppend() = default;
  ~OsdfAlignColumnsForAppend() override = default;

 private:
  std::string testid() const override { return "test::OsdfAlignColumnsForAppend"; }

  void register_tests() const override {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    ts.emplace_back(CASE("ioda/OsdfAlignColumnsForAppend/alignColumns")
                    { testAlignColumns(); });
    ts.emplace_back(CASE("ioda/OsdfAlignColumnsForAppend/columnNotInTarget")
                    { testColumnNotInTarget(); });
    ts.emplace_back(CASE("ioda/OsdfAlignColumnsForAppend/sourceNotFewerColumns")
                    { testSourceNotFewerColumns(); });
  }

  void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda
