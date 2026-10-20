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

#include "eckit/testing/Test.h"

#include "ioda/test/reader/ReaderTestUtils.h"

#include "oops/runs/Test.h"

namespace ioda {
namespace test {

// Tests for the dimension checks made on a netcdf file when it is loaded into an OSDF.
// The checks are hit in the following order:
//  - checkRequiredVarDims: MetaData/dateTime, MetaData/latitude and
// MetaData/longitude must be 1D [Location] variables.
//  - checkVarDimsRegistered: every dimension used by a variable that will be loaded
// must be Location or a registered slice dimension. Dimensions are only registered
// when they have a coordinate variable.

// -----------------------------------------------------------------------------

// Test case where required variable is 1D and dimensioned by registered slice.
// checkRequiredVarDims fails and throws.
void testRequiredVarOnRegisteredNonLocationDim() {
  expectLoadThrowsBadValue(
    dataInConfigFor("osdf_reader_var_dims_required_registered.nc4"),
    {"ioda::reader::checkRequiredVarDims", "MetaData/latitude has dimensions (Channel)"},
    {"MetaData/dateTime", "MetaData/longitude"});
}

// Test the case where a required variable is 2D instead of 1D
// checkRequiredVarDims fails and throws
void testRequiredVar2D() {
  expectLoadThrowsBadValue(
    dataInConfigFor("osdf_reader_var_dims_required_2d.nc4"),
    {"ioda::reader::checkRequiredVarDims", "MetaData/dateTime has dimensions (Location, Channel)"},
    {"MetaData/latitude", "MetaData/longitude"});
}

// Test case where required variable has unregistered dimension instead of Location
// checkRequiredVarDims fails and throws as it is hit first
// checkVarDimsRegistered fails but does not throw as it is not reached.
void testRequiredVarOnUnregisteredDim() {
  expectLoadThrowsBadValue(dataInConfigFor("osdf_reader_var_dims_required_unregistered.nc4"),
                           {"ioda::reader::checkRequiredVarDims",
                            "MetaData/dateTime has dimensions (LocationAll)",
                            "MetaData/latitude has dimensions (LocationAll)",
                            "MetaData/longitude has dimensions (LocationAll)",
                            "osdf_reader_var_dims_required_unregistered.nc4"},
                           {"checkVarDimsRegistered"});
}

// Test case where variable non-required variable has non-registered dimension
// checkVarDimsRegistered fails and throws
void testUnregisteredDimReported() {
  expectLoadThrowsBadValue(dataInConfigFor("osdf_reader_var_dims_unregistered.nc4"),
                           {"ioda::reader::checkVarDimsRegistered",
                            "ObsValue/bar uses dimension 'Level'",
                            "ObsValue/baz uses dimension 'Level'",
                            "osdf_reader_var_dims_unregistered.nc4"},
                           {"ObsValue/foo", "checkRequiredVarDims"});
}

// All checks pass.
void testValidFileLoads() {
  expectLoadSucceeds(dataInConfigFor("osdf_reader_var_dims_valid.nc4"),
                     {"MetaData/dateTime", "MetaData/latitude", "MetaData/longitude",
                      "ObsValue/bt_1", "ObsValue/bt_2", "ObsValue/bt_3"});
}

// Test case where required variable is missing.
// This is not an error as empty files are allowed in the load step.
void testMissingRequiredVarIsNotADimError() {
  expectLoadSucceeds(dataInConfigFor("osdf_reader_var_dims_required_missing.nc4"),
                     {"MetaData/dateTime", "MetaData/longitude"}, {"MetaData/latitude"});
}

// Test that variables the loader skips (e.g. [Location, Location],
// [Channel, Location]) do not cause checkVarDimsRegistered to fail.
void testSkippedVarsAreIgnored() {
  expectLoadSucceeds(dataInConfigFor("osdf_reader_keepvar.nc4"),
                     {"MetaData/latitude", "ObsValue/brightnessTemperature_1"},
                     {"ObsError/badLocLoc", "ObsError/badChanLoc"});
}

// -----------------------------------------------------------------------------
class ReaderVarDims : public oops::Test {
 public:
  ReaderVarDims() {}
  virtual ~ReaderVarDims() {}

 private:
  std::string testid() const override {return "test::ReaderVarDims";}

  void register_tests() const override {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    ts.emplace_back(CASE("ioda/ReaderVarDims/testRequiredVarOnUnregisteredDim")
      { testRequiredVarOnUnregisteredDim(); });
    ts.emplace_back(CASE("ioda/ReaderVarDims/testRequiredVarOnRegisteredNonLocationDim")
      { testRequiredVarOnRegisteredNonLocationDim(); });
    ts.emplace_back(CASE("ioda/ReaderVarDims/testRequiredVar2D")
      { testRequiredVar2D(); });
    ts.emplace_back(CASE("ioda/ReaderVarDims/testUnregisteredDimReported")
      { testUnregisteredDimReported(); });
    ts.emplace_back(CASE("ioda/ReaderVarDims/testValidFileLoads")
      { testValidFileLoads(); });
    ts.emplace_back(CASE("ioda/ReaderVarDims/testMissingRequiredVarIsNotADimError")
      { testMissingRequiredVarIsNotADimError(); });
    ts.emplace_back(CASE("ioda/ReaderVarDims/testSkippedVarsAreIgnored")
      { testSkippedVarsAreIgnored(); });
  }

  void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda
