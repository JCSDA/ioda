/*
 * (C) Crown copyright 2026, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#pragma once

#include <string>
#include <vector>

#define ECKIT_TESTING_SELF_REGISTER_CASES 0

#include "ioda/containers/ColumnMetadata.h"
#include "ioda/containers/FrameUtils.h"
#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"

namespace ioda {
namespace test {

void testGetColumnMetadata() {
  const std::int32_t numMetadatum = 3;

  std::array<std::string, numMetadatum> names = {"name", "longname", "longername"};
  std::array<std::string, numMetadatum> units = {"K", "m", "ms-2"};
  std::array<std::int8_t, numMetadatum> types
    = {osdf::consts::eString, osdf::consts::eInt, osdf::consts::eInt};
  std::array<std::int8_t, numMetadatum> permissions
    = {osdf::consts::eReadWrite, osdf::consts::eReadWrite, osdf::consts::eReadOnly};

  osdf::ColumnMetadata newColumnMetadata;

  for (int32_t index = 0; index < numMetadatum; ++index) {
    osdf::ColumnMetadatum newColumnMetadatum(
      names[index], units[index], types[index], permissions[index]);
    newColumnMetadata.add(newColumnMetadatum);
  }

  EXPECT_EQUAL(newColumnMetadata.getSizeCols(), numMetadatum);

  // returns -1 as metadata does not correspond to a frame with rows
  EXPECT_EQUAL(newColumnMetadata.getMaxId(), -1);

  for (int32_t index = 0; index < numMetadatum; ++index) {
    EXPECT_EQUAL(newColumnMetadata.getName(index), names[index]);
    EXPECT_EQUAL(newColumnMetadata.getUnit(index), units[index]);
    EXPECT_EQUAL(newColumnMetadata.getType(index), types[index]);
    EXPECT_EQUAL(newColumnMetadata.getPermission(index), permissions[index]);
    EXPECT_EQUAL(newColumnMetadata.getWidth(index),
                 static_cast<std::int16_t>(newColumnMetadata.getName(index).size()));
    EXPECT_EQUAL(newColumnMetadata.getIndex(names[index]), index);
  }

  // Updating to smaller width should fail
  newColumnMetadata.updateColumnWidth(0, 0);
  EXPECT_EQUAL(newColumnMetadata.getWidth(0),
               static_cast<std::int16_t>(newColumnMetadata.getName(0).size()));

  // Updating to bigger width should succeed
  newColumnMetadata.updateColumnWidth(0,
    static_cast<std::int16_t>(newColumnMetadata.getName(0).size())+2);
  EXPECT_EQUAL(newColumnMetadata.getWidth(0),
               static_cast<std::int16_t>(newColumnMetadata.getName(0).size())+2);

  // Should be able to overwrite column unit
  newColumnMetadata.updateColumnUnit(0, "Celsius");
  EXPECT_EQUAL(newColumnMetadata.getUnit(0), "Celsius");

  // Setting unit of readOnly column should fail
  EXPECT_THROWS_AS(newColumnMetadata.updateColumnUnit(2, "m"), eckit::BadValue);
}

// -----------------------------------------------------------------------------
class OsdfGetColumnMetadata : public oops::Test {
 public:
  OsdfGetColumnMetadata() {}
  virtual ~OsdfGetColumnMetadata() {}

 private:
  std::string testid() const override { return "test::OsdfGetColumnMetadata"; }

  void register_tests() const override {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    ts.emplace_back(CASE("ioda/OsdfColumnMetadata/testGetColumnMetadata") {
      testGetColumnMetadata();
    });
  }

  void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda
