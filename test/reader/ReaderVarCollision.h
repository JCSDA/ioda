/*
 * (C) Copyright 2026 UCAR
 * (C) Crown copyright 2026, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#pragma once

#include <string>
#include <vector>

#define ECKIT_TESTING_SELF_REGISTER_CASES 0

#include "eckit/testing/Test.h"

#include "ioda/test/reader/ReaderTestUtils.h"

#include "oops/runs/Test.h"

namespace ioda {
namespace test {

// -----------------------------------------------------------------------------
// Tests for the OSDF column name collision check.
//
// A variable dimensioned by a slice dimension is expanded into one column per slice, named
// "<variable>_<slice index>". Nothing in that name distinguishes it from the name of a plain
// 1-D variable ending in the same suffix, so a netcdf file may legally hold a pair of
// variables that the OSDF cannot hold: "X[Location, Channel]" with channel 2 registered, and
// "X_2[Location]", both of which claim the column "X_2". The reader has to reject such a file
// up front, naming all of the colliding variables.
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
/// \brief file containing pairs of variables - some collide and others do not
const char * collidingFile = "osdf_reader_var_collision_two_pairs.nc4";

// -----------------------------------------------------------------------------
void testCollisionIsReported() {
  expectLoadThrowsBadValue(dataInConfigFor(collidingFile),
                           {"ioda::reader::loadObsBlockFromNetcdf",
                            "column: ObsValue/brightnessTemperature_1",
                            "claimed by variable: ObsValue/brightnessTemperature",
                            "and by variable:     ObsValue/brightnessTemperature_1",
                            collidingFile});
}

// -----------------------------------------------------------------------------
void testAllCollisionsAreReported() {
  expectLoadThrowsBadValue(dataInConfigFor(collidingFile),
                           {"column: ObsValue/brightnessTemperature_1",
                            "column: ObsValue/brightnessTemperature_3"});
}

// -----------------------------------------------------------------------------
void testCollisionIsPerGroup() {
  expectLoadThrowsBadValue(dataInConfigFor(collidingFile),
                           {"column: ObsValue/brightnessTemperature_1"},
                           {"ObsError"});
}

// -----------------------------------------------------------------------------
void testUnusedSuffixIsNotACollision() {
  expectLoadSucceeds(dataInConfigFor("osdf_reader_var_collision_unused_suffix.nc4"),
                     {"ObsValue/brightnessTemperature_1",
                      "ObsValue/brightnessTemperature_2",
                      "ObsValue/brightnessTemperature_3",
                      "ObsValue/brightnessTemperature_7"});
}

// -----------------------------------------------------------------------------
void testSuffixedNameWithoutSliceDimIsNotACollision() {
  expectLoadSucceeds(dataInConfigFor("osdf_reader_var_collision_no_slice_dim.nc4"),
                     {"ObsValue/brightnessTemperature_1"});
}

// -----------------------------------------------------------------------------
class ReaderVarCollision : public oops::Test {
 public:
  ReaderVarCollision() {}
  virtual ~ReaderVarCollision() {}

 private:
  std::string testid() const override {return "test::ReaderVarCollision";}

  void register_tests() const override {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    ts.emplace_back(CASE("ioda/ReaderVarCollision/testCollisionIsReported")
      { testCollisionIsReported(); });
    ts.emplace_back(CASE("ioda/ReaderVarCollision/testAllCollisionsAreReported")
      { testAllCollisionsAreReported(); });
    ts.emplace_back(CASE("ioda/ReaderVarCollision/testCollisionIsPerGroup")
      { testCollisionIsPerGroup(); });
    ts.emplace_back(CASE("ioda/ReaderVarCollision/testUnusedSuffixIsNotACollision")
      { testUnusedSuffixIsNotACollision(); });
    ts.emplace_back(CASE("ioda/ReaderVarCollision/testSuffixedNameWithoutSliceDimIsNotACollision")
      { testSuffixedNameWithoutSliceDimIsNotACollision(); });
  }

  void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda
