/*
 * (C) Copyright 2025-2026 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#pragma once

#include <string>
#include <unordered_set>
#include <vector>

#include "eckit/exception/Exceptions.h"

#define ECKIT_TESTING_SELF_REGISTER_CASES 0

#include "eckit/testing/Test.h"
#include "ioda/containers/FrameMetadata.h"
#include "oops/runs/Test.h"
#include "oops/util/Logger.h"

namespace ioda {
namespace test {

// -----------------------------------------------------------------------------
void testDimNums() {
  osdf::FrameMetadata md;

  // hasDim and getDimNums return sensible defaults before any dims are registered
  EXPECT(!md.hasDim("Channel"));
  EXPECT(md.getDimNums("Channel").empty());

  // setDimNums / getDimNums / hasDim round-trip
  const std::vector<int> chanNums = {1, 3, 6, 9, 11};
  md.setDimNums("Channel", chanNums);
  EXPECT(md.hasDim("Channel"));
  EXPECT(md.getDimNums("Channel") == chanNums);

  // a second independent dimension
  const std::vector<int> levelNums = {1, 2, 3, 4, 5};
  md.setDimNums("Level", levelNums);
  EXPECT(md.hasDim("Level"));
  EXPECT(md.getDimNums("Level") == levelNums);

  // Channel is unchanged after adding Level
  EXPECT(md.getDimNums("Channel") == chanNums);

  // setDimNums throws if the dimension is already registered
  EXPECT_THROWS_AS(md.setDimNums("Channel", {2, 4, 8}), eckit::BadValue);
  // Channel coordinate values are unchanged after the failed call
  EXPECT(md.getDimNums("Channel") == chanNums);

  // a dimension with no coordinate values (e.g. Location) is valid
  md.setDimNums("Location", {});
  EXPECT(md.hasDim("Location"));
  EXPECT(md.getDimNums("Location").empty());

  // an unregistered dim still returns false / empty
  EXPECT(!md.hasDim("Time"));
  EXPECT(md.getDimNums("Time").empty());
}

// -----------------------------------------------------------------------------
void testGetDimNames() {
  osdf::FrameMetadata md;

  // empty before any dims are registered
  EXPECT(md.getDimNames().empty());

  // register several dims and check all names are returned
  md.setDimNums("Channel", {1, 3, 6});
  md.setDimNums("Level", {10, 20, 30});
  md.setDimNums("Location", {});

  const std::vector<std::string> names = md.getDimNames();
  EXPECT_EQUAL(names.size(), std::size_t(3));

  // order is unspecified (unordered_map); use a set to check membership
  const std::unordered_set<std::string> nameSet(names.begin(), names.end());
  EXPECT(nameSet.count("Channel") == 1);
  EXPECT(nameSet.count("Level") == 1);
  EXPECT(nameSet.count("Location") == 1);
  EXPECT(nameSet.count("Time") == 0);
}

// -----------------------------------------------------------------------------
void testVarSliceDimName() {
  osdf::FrameMetadata md;

  // variable not registered returns ""
  EXPECT(md.varSliceDimName("someVar") == "");

  // 1D slice variable: the slice dim name is that dimension
  md.addVarDimNames("BT_1d", {"Channel"});
  EXPECT(md.varSliceDimName("BT_1d") == "Channel");

  // 2D Location x <dim> variable: the slice dim name is that dimension
  md.addVarDimNames("BT_2d", {"Location", "Channel"});
  EXPECT(md.varSliceDimName("BT_2d") == "Channel");
  md.addVarDimNames("tempProfile", {"Location", "Level"});
  EXPECT(md.varSliceDimName("tempProfile") == "Level");

  // 1D Location variable has no slice dimension
  md.addVarDimNames("airTemp", {"Location"});
  EXPECT(md.varSliceDimName("airTemp") == "");
}

// -----------------------------------------------------------------------------
void testGetMultiSliceVars() {
  osdf::FrameMetadata md;

  // empty result when no variables registered
  EXPECT(md.getMultiSliceVars().empty());

  // add a mix of variables with and without a slice dimension
  md.addVarDimNames("ObsValue/bt_1d",       {"Channel"});
  md.addVarDimNames("ObsValue/bt_2d",       {"Location", "Channel"});
  md.addVarDimNames("MetaData/airTemp",     {"Location"});
  md.addVarDimNames("MetaData/tempProfile", {"Location", "Level"});

  // getMultiSliceVars returns any variable with a slice dim
  // (Channel, Level, ...), not just Channel.
  const std::unordered_set<std::string> result = md.getMultiSliceVars();
  EXPECT_EQUAL(result.size(), std::size_t(3));
  EXPECT(result.count("ObsValue/bt_1d") == 1);
  EXPECT(result.count("ObsValue/bt_2d") == 1);
  EXPECT(result.count("MetaData/tempProfile") == 1);
  EXPECT(result.count("MetaData/airTemp") == 0);
}

// -----------------------------------------------------------------------------
void testOperatorEquals() {
  // two default-constructed objects are equal
  osdf::FrameMetadata md1, md2;
  EXPECT(md1 == md2);

  // numVars_ difference
  md1.setNumVars(3);
  EXPECT(!(md1 == md2));
  md2.setNumVars(3);
  EXPECT(md1 == md2);

  // dimNums_: one dim present, other absent
  md1.setDimNums("Channel", {1, 3, 6});
  EXPECT(!(md1 == md2));
  md2.setDimNums("Channel", {1, 3, 6});
  EXPECT(md1 == md2);

  // dimNums_: same key, different coordinate values — use fresh objects since
  // setDimNums does not allow overwriting a registered dimension
  {
    osdf::FrameMetadata a, b;
    a.setDimNums("Level", {10, 20});
    b.setDimNums("Level", {10, 30});
    EXPECT(!(a == b));
    osdf::FrameMetadata c, d;
    c.setDimNums("Level", {10, 20});
    d.setDimNums("Level", {10, 20});
    EXPECT(c == d);
  }

  // varDimNames_: one entry present, other absent
  md1.addVarDimNames("ObsValue/BT", {"Location", "Channel"});
  EXPECT(!(md1 == md2));
  md2.addVarDimNames("ObsValue/BT", {"Location", "Channel"});
  EXPECT(md1 == md2);

  // varDimNames_: same key, different dim list
  md1.addVarDimNames("MetaData/airTemp", {"Location"});
  md2.addVarDimNames("MetaData/airTemp", {"Location", "Level"});
  EXPECT(!(md1 == md2));
}

// -----------------------------------------------------------------------------
class OsdfFrameMetadata : public oops::Test {
 public:
  OsdfFrameMetadata() {}
  virtual ~OsdfFrameMetadata() {}

 private:
  std::string testid() const override { return "test::OsdfFrameMetadata"; }

  void register_tests() const override {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    ts.emplace_back(CASE("ioda/OsdfFrameMetadata/dimNums")
                    { testDimNums(); });
    ts.emplace_back(CASE("ioda/OsdfFrameMetadata/getDimNames")
                    { testGetDimNames(); });
    ts.emplace_back(CASE("ioda/OsdfFrameMetadata/varSliceDimName")
                    { testVarSliceDimName(); });
    ts.emplace_back(CASE("ioda/OsdfFrameMetadata/getMultiSliceVars")
                    { testGetMultiSliceVars(); });
    ts.emplace_back(CASE("ioda/OsdfFrameMetadata/operatorEquals")
                    { testOperatorEquals(); });
  }

  void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda
