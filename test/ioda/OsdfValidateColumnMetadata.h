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
  // can write to all columns of blank frame
  osdf::ColumnMetadata columnMetadata;
  EXPECT_EQUAL(columnMetadata.canWriteAllData(), true);

  // can write to all columns of frame with default initialised metadatum
  osdf::ColumnMetadatum writeMetadatum("write_data", osdf::consts::eInt,
                                          osdf::consts::eReadWrite);
  osdf::ColumnMetadatum anotherWriteMetadatum("another_write_data", osdf::consts::eInt,
                                              osdf::consts::eReadWrite);
  columnMetadata.add(writeMetadatum);
  columnMetadata.add(anotherWriteMetadatum);
  EXPECT_EQUAL(columnMetadata.canWriteAllData(), true);

  // can not write to all columns of frame with one permission set to eReadOnly
  osdf::ColumnMetadatum readOnlyMetadatum("data", osdf::consts::eInt, osdf::consts::eReadOnly);
  columnMetadata.add(readOnlyMetadatum);
  EXPECT_EQUAL(columnMetadata.canWriteAllData(), false);

  // can not write to all columns of frame with all permissions set to eReadOnly
  osdf::ColumnMetadata readOnlyColumnMetadata;
  osdf::ColumnMetadatum anotherReadOnlyMetadatum("another_data", osdf::consts::eString,
                                                 osdf::consts::eReadOnly);
  readOnlyColumnMetadata.add(readOnlyMetadatum);
  readOnlyColumnMetadata.add(anotherReadOnlyMetadatum);
  EXPECT_EQUAL(readOnlyColumnMetadata.canWriteAllData(), false);
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


  bool compareEqualMetadata
    = allWriteColumnMetadata.compareColumnMetadataPermissions(allWriteColumnMetadata);
  EXPECT_EQUAL(compareEqualMetadata, true);

  bool compareUnequalMetadataSamePermissions
    = readOnlyColumnMetadataOne.compareColumnMetadataPermissions(readOnlyColumnMetadataTwo);
  EXPECT_EQUAL(compareUnequalMetadataSamePermissions, true);

  bool compareUnequalMetadata
    = allWriteColumnMetadata.compareColumnMetadataPermissions(readOnlyColumnMetadataOne);
  EXPECT_EQUAL(compareUnequalMetadata, false);

  bool compareDifferentSizeMetadata = true;
  try {
    blankColumnMetadata.compareColumnMetadataPermissions(allWriteColumnMetadata);
  } catch (eckit::BadParameter) {
    compareDifferentSizeMetadata = false;
  }
  EXPECT_EQUAL(compareDifferentSizeMetadata, false);
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

  bool isBlankEqualOneTwo = true;
  try {
    namesOneTwoMetadata.compareColumnMetadata(blankMetadata);
  } catch (eckit::BadParameter) {
    isBlankEqualOneTwo = false;
  }
  EXPECT_EQUAL(isBlankEqualOneTwo, false);

  // metadata equal
  std::int8_t isEqualItself = namesOneTwoMetadata.compareColumnMetadata(namesOneTwoMetadata);
  EXPECT_EQUAL(isEqualItself, true);

  // metadata with different names in second slot
  osdf::ColumnMetadata namesOneThreeMetadata;
  namesOneThreeMetadata.add(nameOneMetadatum);
  namesOneThreeMetadata.add(nameThreeMetadatum);
  std::int8_t isOneTwoEqualOneThree
    = namesOneTwoMetadata.compareColumnMetadata(namesOneThreeMetadata);
  EXPECT_EQUAL(isOneTwoEqualOneThree, false);

  // metadata with different types in first slot
  osdf::ColumnMetadatum typeStringMetadatum("name_1", osdf::consts::eString);
  osdf::ColumnMetadata typeStringMetadata;
  typeStringMetadata.add(typeStringMetadatum);
  typeStringMetadata.add(nameTwoMetadatum);
  std::int8_t isIntEqualString = typeStringMetadata.compareColumnMetadata(namesOneTwoMetadata);
  EXPECT_EQUAL(isIntEqualString, false);
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

    ts.emplace_back(CASE("ioda/ReaderFilter/testColumnMetadataValidateWrite") {
      testColumnMetadataValidateWrite();
    });
    ts.emplace_back(CASE("ioda/ReaderFilter/testColumnMetadataComparePermissions") {
      testColumnMetadataComparePermissions();
    });
    ts.emplace_back(CASE("ioda/ReaderFilter/testColumnMetadataCompare") {
      testColumnMetadataCompare();
    });
  }

  void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda
