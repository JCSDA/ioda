/*
 * (C) Copyright 2026 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

#define ECKIT_TESTING_SELF_REGISTER_CASES 0

#include "eckit/testing/Test.h"

#include "ioda/containers/CreateIFrame.h"
#include "ioda/containers/FrameMetadata.h"
#include "ioda/containers/IFrame.h"
#include "ioda/core/IodaUtils.h"
#include "oops/runs/Test.h"

namespace ioda {
namespace test {

// -----------------------------------------------------------------------------
// The two utilities under test are the single OSDF variable naming/existence rule:
//    osdfVarColumnNames -- the column names a variable occupies, existing or not
//    osdfVarColumns     -- those names, or empty when the variable is not present
//
// Both are driven by the slice dimension registered for a variable in the frame
// metadata, which is what distinguishes a 1-D variable whose name happens to end in
// a numeric suffix ("ObsValue/var_1") from one slice of a 2-D variable ("ObsValue/var"
// stored as "ObsValue/var_1", "ObsValue/var_2", ...).

// -----------------------------------------------------------------------------
/// \brief register the test variables in a frame metadata object
/// \details Registers three 2-D variables sharing a "Channel" dimension with
///          non-contiguous, non-1-based index values (so that a test cannot pass by
///          assuming 0..n-1), one 2-D variable on a second dimension named "Level",
///          and one 2-D variable whose slice dimension has no registered index values.
void registerTestVars(osdf::FrameMetadata & metadata) {
  metadata.setDimNums("Channel", { 3, 5, 9 });
  metadata.setDimNums("Level", { 1, 2 });

  metadata.addVarDimNames("ObsValue/brightnessTemperature", { "Location", "Channel" });
  metadata.addVarDimNames("DerivedObsValue/brightnessTemperature", { "Location", "Channel" });
  metadata.addVarDimNames("ObsValue/channelOnly", { "Channel" });
  metadata.addVarDimNames("ObsValue/temperature", { "Location", "Level" });

  // A variable whose slice dimension was never given index values. "Unregistered" is
  // deliberately not passed to setDimNums.
  metadata.addVarDimNames("ObsValue/noDimNums", { "Location", "Unregistered" });

  // A 1-D variable is registered with Location only, and a variable can also be absent
  // from the metadata altogether -- both must resolve to their verbatim name.
  metadata.addVarDimNames("MetaData/latitude", { "Location" });
}

// -----------------------------------------------------------------------------
void testOsdfVarColumnNames() {
  osdf::FrameMetadata metadata;
  registerTestVars(metadata);

  // A variable with a registered slice dimension occupies one column per slice, named
  // with that dimension's index values, in the order the dimension registered them.
  const std::vector<std::string> refChanCols = { "ObsValue/brightnessTemperature_3",
                                                "ObsValue/brightnessTemperature_5",
                                                "ObsValue/brightnessTemperature_9" };
  EXPECT(osdfVarColumnNames(metadata, "ObsValue/brightnessTemperature") == refChanCols);

  // The rule keys off the variable name in full, so the same variable in the Derived
  // group resolves independently.
  const std::vector<std::string> refDerivedCols
    = { "DerivedObsValue/brightnessTemperature_3",
        "DerivedObsValue/brightnessTemperature_5",
        "DerivedObsValue/brightnessTemperature_9" };
  EXPECT(osdfVarColumnNames(metadata, "DerivedObsValue/brightnessTemperature")
         == refDerivedCols);

  // A slice-dimension-only variable (no Location) is named the same way.
  const std::vector<std::string> refChanOnlyCols = { "ObsValue/channelOnly_3",
                                                    "ObsValue/channelOnly_5",
                                                    "ObsValue/channelOnly_9" };
  EXPECT(osdfVarColumnNames(metadata, "ObsValue/channelOnly") == refChanOnlyCols);

  // The slice dimension need not be Channel.
  const std::vector<std::string> refLevelCols = { "ObsValue/temperature_1",
                                                 "ObsValue/temperature_2" };
  EXPECT(osdfVarColumnNames(metadata, "ObsValue/temperature") == refLevelCols);

  // A variable registered with Location only is named verbatim.
  const std::vector<std::string> refLatCols = { "MetaData/latitude" };
  EXPECT(osdfVarColumnNames(metadata, "MetaData/latitude") == refLatCols);

  // So is a variable that is not registered at all, even when its name ends in a
  // numeric suffix. This is what keeps a 1-D "var_1" distinct from slice 1 of "var".
  const std::vector<std::string> refUnregCols = { "ObsValue/unregistered" };
  EXPECT(osdfVarColumnNames(metadata, "ObsValue/unregistered") == refUnregCols);

  const std::vector<std::string> refSuffixCols = { "ObsValue/oneDimVar_1" };
  EXPECT(osdfVarColumnNames(metadata, "ObsValue/oneDimVar_1") == refSuffixCols);

  // Asking for a single slice of a 2-D variable gives that slice's column verbatim,
  // since the suffixed name is not itself registered.
  const std::vector<std::string> refOneSliceCols = { "ObsValue/brightnessTemperature_5" };
  EXPECT(osdfVarColumnNames(metadata, "ObsValue/brightnessTemperature_5") == refOneSliceCols);

  // A registered slice dimension carrying no index values falls back to the verbatim
  // name, so the result is never empty and front() is always safe to take.
  const std::vector<std::string> refNoDimNumsCols = { "ObsValue/noDimNums" };
  EXPECT(osdfVarColumnNames(metadata, "ObsValue/noDimNums") == refNoDimNumsCols);

  // The result is never empty for any input.
  const std::vector<std::string> namesToTest = { "ObsValue/brightnessTemperature",
                                                "ObsValue/channelOnly",
                                                "ObsValue/temperature",
                                                "MetaData/latitude",
                                                "ObsValue/noDimNums",
                                                "ObsValue/neverHeardOf" };
  for (const std::string & name : namesToTest) {
    EXPECT(!osdfVarColumnNames(metadata, name).empty());
  }
}

// -----------------------------------------------------------------------------
void testOsdfVarColumns() {
  osdf::FrameMetadata metadata;
  registerTestVars(metadata);

  std::unique_ptr<osdf::IFrame> frame = osdf::createIFrame("FrameCols");

  const std::vector<float> floatData(4, 0.0);
  const std::vector<int> intData(4, 0);

  // Create the columns for the 2-D Channel variable, its Derived counterpart, the
  // Level variable and the 1-D variable. Deliberately leave "ObsValue/channelOnly"
  // and "ObsValue/noDimNums" registered in the metadata but absent from the frame.
  for (const std::string & columnName :
           osdfVarColumnNames(metadata, "ObsValue/brightnessTemperature")) {
    frame->appendNewColumn(columnName, floatData);
  }
  for (const std::string & columnName :
           osdfVarColumnNames(metadata, "DerivedObsValue/brightnessTemperature")) {
    frame->appendNewColumn(columnName, floatData);
  }
  for (const std::string & columnName : osdfVarColumnNames(metadata, "ObsValue/temperature")) {
    frame->appendNewColumn(columnName, floatData);
  }
  frame->appendNewColumn("MetaData/latitude", floatData);

  // A 1-D variable whose name ends in a numeric suffix, stored verbatim.
  frame->appendNewColumn("ObsValue/oneDimVar_1", intData);

  // A present 2-D variable resolves to all of its slice columns.
  const std::vector<std::string> refChanCols = { "ObsValue/brightnessTemperature_3",
                                                "ObsValue/brightnessTemperature_5",
                                                "ObsValue/brightnessTemperature_9" };
  EXPECT(osdfVarColumns(*frame, metadata, "ObsValue/brightnessTemperature") == refChanCols);

  const std::vector<std::string> refLevelCols = { "ObsValue/temperature_1",
                                                 "ObsValue/temperature_2" };
  EXPECT(osdfVarColumns(*frame, metadata, "ObsValue/temperature") == refLevelCols);

  // Present in the Derived group as well, resolved independently.
  EXPECT(osdfVarColumns(*frame, metadata, "DerivedObsValue/brightnessTemperature").size() == 3);

  // A present 1-D variable resolves to its single column.
  const std::vector<std::string> refLatCols = { "MetaData/latitude" };
  EXPECT(osdfVarColumns(*frame, metadata, "MetaData/latitude") == refLatCols);

  // A present 1-D variable whose name ends in a numeric suffix resolves verbatim -- it
  // must not be mistaken for a slice of a 2-D variable.
  const std::vector<std::string> refSuffixCols = { "ObsValue/oneDimVar_1" };
  EXPECT(osdfVarColumns(*frame, metadata, "ObsValue/oneDimVar_1") == refSuffixCols);

  // An individual slice of a present 2-D variable is a column in its own right.
  const std::vector<std::string> refOneSliceCols = { "ObsValue/brightnessTemperature_5" };
  EXPECT(osdfVarColumns(*frame, metadata, "ObsValue/brightnessTemperature_5")
         == refOneSliceCols);

  // A slice index the dimension does not carry is absent.
  EXPECT(osdfVarColumns(*frame, metadata, "ObsValue/brightnessTemperature_4").empty());

  // Registered in the metadata but never created in the frame: absent. This is the
  // check that stops a caller being handed names that are not columns.
  EXPECT(osdfVarColumns(*frame, metadata, "ObsValue/channelOnly").empty());
  EXPECT(osdfVarColumns(*frame, metadata, "ObsValue/noDimNums").empty());

  // Neither registered nor created: absent.
  EXPECT(osdfVarColumns(*frame, metadata, "ObsValue/neverHeardOf").empty());
  EXPECT(osdfVarColumns(*frame, metadata, "MetaData/latitude_1").empty());

  // The same group/name in a group that holds nothing: absent.
  EXPECT(osdfVarColumns(*frame, metadata, "ObsError/brightnessTemperature").empty());

  // Wherever osdfVarColumns reports a variable present, the names it returns are all
  // real columns, and they are exactly the names osdfVarColumnNames gives.
  const std::vector<std::string> namesToTest = { "ObsValue/brightnessTemperature",
                                                "DerivedObsValue/brightnessTemperature",
                                                "ObsValue/temperature",
                                                "MetaData/latitude",
                                                "ObsValue/oneDimVar_1",
                                                "ObsValue/channelOnly",
                                                "ObsValue/noDimNums",
                                                "ObsValue/neverHeardOf" };
  for (const std::string & name : namesToTest) {
    const std::vector<std::string> columnNames = osdfVarColumns(*frame, metadata, name);
    if (!columnNames.empty()) {
      EXPECT(columnNames == osdfVarColumnNames(metadata, name));
      for (const std::string & columnName : columnNames) {
        EXPECT(frame->hasColumn(columnName));
      }
    }
  }
}

// -----------------------------------------------------------------------------
class OsdfVarColumns : public oops::Test {
 public:
  OsdfVarColumns() {}
  virtual ~OsdfVarColumns() {}

 private:
  std::string testid() const override { return "test::OsdfVarColumns"; }

  void register_tests() const override {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    // Note that testOsdfVarColumnNames and testOsdfVarColumns need to be kept
    // in sync with the test variables registered in registerTestVars, above.
    ts.emplace_back(CASE("ioda/OsdfVarColumns/osdfVarColumnNames")
                    { testOsdfVarColumnNames(); });
    ts.emplace_back(CASE("ioda/OsdfVarColumns/osdfVarColumns")
                    { testOsdfVarColumns(); });
  }

  void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda
