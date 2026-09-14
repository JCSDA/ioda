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

#include "eckit/config/LocalConfiguration.h"
#include "eckit/exception/Exceptions.h"
#include "eckit/testing/Test.h"

#include "ioda/containers/FrameMetadata.h"
#include "ioda/containers/FrameRows.h"
#include "ioda/containers/IFrame.h"
#include "ioda/ioPool/IoPoolParameters.h"
#include "ioda/ObsDataIoParameters.h"
#include "ioda/reader/load/loadObs.hpp"

#include "oops/mpi/mpi.h"
#include "oops/runs/Test.h"
#include "oops/util/Logger.h"
#include "oops/util/TimeWindow.h"

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
/// \brief the obsdatain configuration naming one of the committed input files
eckit::LocalConfiguration dataInConfigFor(const std::string & fileName) {
  eckit::LocalConfiguration config;
  config.set("engine.type", "H5File");
  config.set("engine.obsfile", "Data/testinput_tier_1/" + fileName);
  return config;
}

// -----------------------------------------------------------------------------
/// \brief run the reader over the given input file, into the given container
/// \details The load is driven through reader::loadObs so that the check is
/// reached the way an obs space reaches it.
void loadObsFromFile(const eckit::LocalConfiguration & dataInConfig,
                     std::unique_ptr<osdf::IFrame> & destOsdf,
                     osdf::FrameMetadata & osdfMetadata) {
  ioda::ObsDataInParameters dataInParams;
  dataInParams.deserialize(dataInConfig);

  eckit::LocalConfiguration ioPoolConfig;
  ioPoolConfig.set("max pool size", 1);
  ioda::IoPool::IoPoolParameters ioPoolParams;
  ioPoolParams.validateAndDeserialize(ioPoolConfig);

  const std::vector<std::string> obsVarNames;
  eckit::LocalConfiguration timeWindowConfig;
  timeWindowConfig.set("begin", "2018-01-01T00:00:00Z");
  timeWindowConfig.set("end", "2018-01-01T06:00:00Z");
  const util::TimeWindow timeWindow(timeWindowConfig);

  reader::loadObs(dataInParams, ioPoolParams, oops::mpi::world(), obsVarNames, timeWindow,
                  "ReaderVarCollision test", destOsdf, osdfMetadata);
}

// -----------------------------------------------------------------------------
/// \brief the load must fail with an eckit::BadValue naming every expected string and no
/// unexpected one
/// \details The messaging is what makes this check worth having. Without this check,
/// the column lookup fails without reporting the collisions.
void expectCollisionReported(const eckit::LocalConfiguration & dataInConfig,
                             const std::vector<std::string> & expectedInMessage,
                             const std::vector<std::string> & unexpectedInMessage = {}) {
  std::unique_ptr<osdf::IFrame> testOsdf = std::make_unique<osdf::FrameRows>();
  osdf::FrameMetadata osdfMetadata;
  bool threwBadValue = false;
  std::string errMsg;
  try {
    loadObsFromFile(dataInConfig, testOsdf, osdfMetadata);
  } catch (const eckit::BadValue & e) {
    threwBadValue = true;
    errMsg = e.what();
  }
  EXPECT(threwBadValue);
  if (!threwBadValue) {
    return;
  }
  for (const auto & expected : expectedInMessage) {
    const bool found = (errMsg.find(expected) != std::string::npos);
    if (!found) {
      oops::Log::info() << "ReaderVarCollision: error message is missing: " << expected
                        << std::endl << "    message was: " << errMsg << std::endl;
    }
    EXPECT(found);
  }
  for (const auto & unexpected : unexpectedInMessage) {
    const bool absent = (errMsg.find(unexpected) == std::string::npos);
    if (!absent) {
      oops::Log::info() << "ReaderVarCollision: error message should not name: " << unexpected
                        << std::endl << "    message was: " << errMsg << std::endl;
    }
    EXPECT(absent);
  }
}

// -----------------------------------------------------------------------------
/// \brief the load must succeed, and each of the named columns must be there
void expectNoCollision(const eckit::LocalConfiguration & dataInConfig,
                       const std::vector<std::string> & expectedColumns) {
  std::unique_ptr<osdf::IFrame> testOsdf = std::make_unique<osdf::FrameRows>();
  osdf::FrameMetadata osdfMetadata;
  EXPECT_NO_THROW((loadObsFromFile(dataInConfig, testOsdf, osdfMetadata)));
  for (const auto & columnName : expectedColumns) {
    const bool found = testOsdf->hasColumn(columnName);
    if (!found) {
      oops::Log::info() << "ReaderVarCollision: expected column is missing: " << columnName
                        << std::endl;
    }
    EXPECT(found);
  }
}

// -----------------------------------------------------------------------------
/// \brief file containing pairs of variables - some collide and others do not
const char * collidingFile = "osdf_reader_var_collision_two_pairs.nc4";

// -----------------------------------------------------------------------------
void testCollisionIsReported() {
  expectCollisionReported(dataInConfigFor(collidingFile),
                          {"ioda::reader::loadObsBlockFromNetcdf",
                           "column: ObsValue/brightnessTemperature_1",
                           "claimed by variable: ObsValue/brightnessTemperature",
                           "and by variable:     ObsValue/brightnessTemperature_1",
                           collidingFile});
}

// -----------------------------------------------------------------------------
void testAllCollisionsAreReported() {
  expectCollisionReported(dataInConfigFor(collidingFile),
                          {"column: ObsValue/brightnessTemperature_1",
                           "column: ObsValue/brightnessTemperature_3"});
}

// -----------------------------------------------------------------------------
void testCollisionIsPerGroup() {
  expectCollisionReported(dataInConfigFor(collidingFile),
                          {"column: ObsValue/brightnessTemperature_1"},
                          {"ObsError"});
}

// -----------------------------------------------------------------------------
void testUnusedSuffixIsNotACollision() {
  expectNoCollision(dataInConfigFor("osdf_reader_var_collision_unused_suffix.nc4"),
                    {"ObsValue/brightnessTemperature_1",
                     "ObsValue/brightnessTemperature_2",
                     "ObsValue/brightnessTemperature_3",
                     "ObsValue/brightnessTemperature_7"});
}

// -----------------------------------------------------------------------------
void testSuffixedNameWithoutSliceDimIsNotACollision() {
  expectNoCollision(dataInConfigFor("osdf_reader_var_collision_no_slice_dim.nc4"),
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
