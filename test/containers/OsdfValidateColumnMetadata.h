/*
 * (C) Crown copyright 2026, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#pragma once

#include <string>
#include <vector>

#include "eckit/exception/Exceptions.h"

#define ECKIT_TESTING_SELF_REGISTER_CASES 0

#include "ioda/containers/ColumnMetadata.h"
#include "ioda/containers/FrameUtils.h"
#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"

namespace ioda {
namespace test {

void testColumnMetadataValidateWrite() {
  // can write to all columns of blank frame (will throw exception if not)
  osdf::ColumnMetadata columnMetadata;
  columnMetadata.validateCanWriteAllData();

  // can write to all columns of frame with default initialised metadatum
  osdf::ColumnMetadatum writeMetadatum("write_data", osdf::consts::eInt,
                                          osdf::consts::eReadWrite);
  osdf::ColumnMetadatum anotherWriteMetadatum("another_write_data", osdf::consts::eInt,
                                              osdf::consts::eReadWrite);
  columnMetadata.add(writeMetadatum);
  columnMetadata.add(anotherWriteMetadatum);
  columnMetadata.validateCanWriteAllData();

  // can not write to all columns of frame with one permission set to eReadOnly
  osdf::ColumnMetadatum readOnlyMetadatum("data", osdf::consts::eInt, osdf::consts::eReadOnly);
  columnMetadata.add(readOnlyMetadatum);
  EXPECT_THROWS_AS(columnMetadata.validateCanWriteAllData(), eckit::BadParameter);

  // can not write to all columns of frame with all permissions set to eReadOnly
  osdf::ColumnMetadata readOnlyColumnMetadata;
  osdf::ColumnMetadatum anotherReadOnlyMetadatum("another_data", osdf::consts::eString,
                                                 osdf::consts::eReadOnly);
  readOnlyColumnMetadata.add(readOnlyMetadatum);
  readOnlyColumnMetadata.add(anotherReadOnlyMetadatum);
  EXPECT_THROWS_AS(readOnlyColumnMetadata.validateCanWriteAllData(), eckit::BadParameter);
}

void testColumnMetadataComparePermissions() {
  osdf::ColumnMetadatum writeDataOne("data_1", osdf::consts::eString, osdf::consts::eReadWrite);
  osdf::ColumnMetadatum writeDataTwo("data_2", osdf::consts::eInt, osdf::consts::eReadWrite);
  osdf::ColumnMetadatum readDataOne("read_data_1", osdf::consts::eString, osdf::consts::eReadOnly);

  osdf::ColumnMetadata blankColumnMetadata;

  osdf::ColumnMetadata allWriteColumnMetadata;
  allWriteColumnMetadata.add(writeDataOne);
  allWriteColumnMetadata.add(writeDataTwo);

  osdf::ColumnMetadata readOnlyColumnMetadataOne;
  readOnlyColumnMetadataOne.add(writeDataOne);
  readOnlyColumnMetadataOne.add(readDataOne);

  osdf::ColumnMetadata readOnlyColumnMetadataTwo;
  readOnlyColumnMetadataTwo.add(writeDataTwo);
  readOnlyColumnMetadataTwo.add(readDataOne);

  // The ctest failing looks like these throwing an exception
  allWriteColumnMetadata.validateColumnMetadataPermissions(allWriteColumnMetadata);
  readOnlyColumnMetadataOne.validateColumnMetadataPermissions(readOnlyColumnMetadataTwo);

  // Compare metadata with different permissions
  EXPECT_THROWS_AS(
    allWriteColumnMetadata.validateColumnMetadataPermissions(readOnlyColumnMetadataOne),
    eckit::BadParameter);

  // Compare metadata with different numbers of columns
  EXPECT_THROWS_AS(blankColumnMetadata.validateColumnMetadataPermissions(allWriteColumnMetadata),
                   eckit::BadParameter);
}

void testColumnMetadataCompare() {
  osdf::ColumnMetadatum nameOneMetadatum("name_1", osdf::consts::eInt);
  osdf::ColumnMetadatum nameTwoMetadatum("name_2", osdf::consts::eInt);
  osdf::ColumnMetadatum nameThreeMetadatum("name_3", osdf::consts::eInt);

  // metadata of different lengths
  osdf::ColumnMetadata blankMetadata;
  osdf::ColumnMetadata namesOneTwoMetadata;
  namesOneTwoMetadata.add(nameOneMetadatum);
  namesOneTwoMetadata.add(nameTwoMetadatum);
  EXPECT_THROWS_AS(namesOneTwoMetadata.validateColumnMetadata(blankMetadata), eckit::BadParameter);

  // metadata equal (ctest fails if this throws an exception)
  namesOneTwoMetadata.validateColumnMetadata(namesOneTwoMetadata);

  // metadata with different names in second slot
  osdf::ColumnMetadata namesOneThreeMetadata;
  namesOneThreeMetadata.add(nameOneMetadatum);
  namesOneThreeMetadata.add(nameThreeMetadatum);
  EXPECT_THROWS_AS(namesOneTwoMetadata.validateColumnMetadata(namesOneThreeMetadata),
                   eckit::BadParameter);

  // metadata with different types in first slot
  osdf::ColumnMetadatum typeStringMetadatum("name_1", osdf::consts::eString);
  osdf::ColumnMetadata typeStringMetadata;
  typeStringMetadata.add(typeStringMetadatum);
  typeStringMetadata.add(nameTwoMetadatum);
  EXPECT_THROWS_AS(typeStringMetadata.validateColumnMetadata(namesOneTwoMetadata),
                   eckit::BadParameter);

  // metadata with different units in second slot
  // (the case where units match is covered above, default is MISSING)
  osdf::ColumnMetadatum unitKelvinMetadatum("name_2", "K", osdf::consts::eString);
  osdf::ColumnMetadatum unitCelsiusMetadatum("name_2", "Celsius", osdf::consts::eString);

  osdf::ColumnMetadata unitKelvinMetadata;
  unitKelvinMetadata.add(nameOneMetadatum);
  unitKelvinMetadata.add(unitKelvinMetadatum);
  osdf::ColumnMetadata unitCelsiusMetadata;
  unitCelsiusMetadata.add(nameOneMetadatum);
  unitCelsiusMetadata.add(unitCelsiusMetadatum);

  EXPECT_THROWS_AS(unitKelvinMetadata.validateColumnMetadata(unitCelsiusMetadata),
                   eckit::BadParameter);
}



  // -----------------------------------------------------------------------------
class OsdfValidateColumnMetadata : public oops::Test {
 public:
  OsdfValidateColumnMetadata() {}
  virtual ~OsdfValidateColumnMetadata() {}

 private:
  std::string testid() const override { return "test::OsdfValidateColumnMetadata"; }

  void register_tests() const override {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    ts.emplace_back(CASE("ioda/OsdfColumnMetadata/testColumnMetadataValidateWrite") {
      testColumnMetadataValidateWrite();
    });
    ts.emplace_back(CASE("ioda/OsdfColumnMetadata/testColumnMetadataComparePermissions") {
      testColumnMetadataComparePermissions();
    });
    ts.emplace_back(CASE("ioda/OsdfColumnMetadata/testColumnMetadataCompare") {
      testColumnMetadataCompare();
    });
  }

  void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda
