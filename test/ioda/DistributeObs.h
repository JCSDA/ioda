/*
 * (C) Copyright 2025 UCAR
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
#include "eckit/testing/Test.h"

#include "ioda/containers/FrameMetadata.h"
#include "ioda/containers/FrameRows.h"
#include "ioda/core/ObsSourceStats.h"
#include "ioda/distribution/DistributionFactory.h"
#include "ioda/reader/distribute/distributeObs.hpp"
#include "ioda/test/ioda/OsdfTestUtils.h"

#include "oops/mpi/mpi.h"
#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"
#include "oops/util/Logger.h"

namespace ioda {
namespace test {

void testCalculateDistribution() {
  int myMpiSize = oops::mpi::world().size();
  int myMpiRank = oops::mpi::world().rank();

  const auto &topLevelConf = ::test::TestEnvironment::config();

  std::string MyPath = "mpisize" + std::to_string(myMpiSize) +
                          ".rank" + std::to_string(myMpiRank);
  const eckit::LocalConfiguration configInputData =
    topLevelConf.getSubConfiguration("function inputs");
  const double tolerance = topLevelConf.getDouble("tolerance");

  // Create the input osdf frame to pass to distributeObs for this rank
  const std::vector<eckit::LocalConfiguration> configInputFrameData =
    configInputData.getSubConfigurations("column data." + MyPath);
  std::unique_ptr<osdf::IFrame> inoutOsdf = std::make_unique<osdf::FrameRows>();
  std::vector<std::string> inputColumnNames;
  std::vector<std::string> inputColumnTypes;
  populateFrame(configInputFrameData, inoutOsdf, inputColumnNames, inputColumnTypes);

  // Create the other distributeObs function inputs
  // Distribution
  ioda::EmptyDistributionParameters distParams;
  distParams.deserialize(configInputData.getSubConfiguration("distribution"));
  std::shared_ptr<Distribution> distribution;

  // obsGroupVarList
  const std::vector<std::string> obsGroupVarList =
      configInputData.getStringVector("grouping variables");

  // osdfMetadata
  osdf::FrameMetadata osdfMetadata;
  osdfMetadata.setFrameType("FrameRows");

  // ObsSourceStats
  ioda::ObsSourceStats obsSourceStats;
  obsSourceStats.sourceNlocs = configInputData.getUnsigned("number of locations");

  // Call the function being tested
  ioda::reader::distributeObs(distParams, oops::mpi::world(),
                             obsGroupVarList,
                             osdfMetadata,
                             obsSourceStats,
                             distribution,
                             inoutOsdf);

  // Create the reference osdf frame for this rank to compare results
  const eckit::LocalConfiguration configExpectedResults =
    topLevelConf.getSubConfiguration("expected results." + MyPath);
  const std::vector<eckit::LocalConfiguration> configRefFrameData =
    configExpectedResults.getSubConfigurations("expected local data");
  std::unique_ptr<osdf::IFrame> refOutOsdf = std::make_unique<osdf::FrameRows>();
  std::vector<std::string> refColumnNames;
  std::vector<std::string> refColumnTypes;
  populateFrame(configRefFrameData, refOutOsdf, refColumnNames, refColumnTypes);

  // Compare the output osdf frame to the reference osdf frame
  compareFrames(inoutOsdf, inputColumnNames, inputColumnTypes, refOutOsdf, refColumnNames,
                refColumnTypes, tolerance);

  std::vector<std::size_t> expectedLocalLocIndices, expectedLocalRecNums;
  std::size_t expectedLocalNlocs, expectedLocalNrecs;
  expectedLocalLocIndices = configExpectedResults.getUnsignedVector("local loc indices");
  expectedLocalRecNums = configExpectedResults.getUnsignedVector("local record numbers");
  expectedLocalNlocs = configExpectedResults.getUnsigned("local num locations");
  expectedLocalNrecs = configExpectedResults.getUnsigned("local num records");

  EXPECT(obsSourceStats.locIndices == expectedLocalLocIndices);
  EXPECT(obsSourceStats.recNums == expectedLocalRecNums);
  EXPECT(obsSourceStats.nlocs == expectedLocalNlocs);
  EXPECT(obsSourceStats.nrecs == expectedLocalNrecs);
}

// -----------------------------------------------------------------------------
class DistributeObs : public oops::Test {
 public:
  DistributeObs() {}
  virtual ~DistributeObs() {}

 private:
  std::string testid() const override {return "test::DistributeObs";}

  void register_tests() const override {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    ts.emplace_back(CASE("ioda/DistributeObs/testCalculateDistribution")
      { testCalculateDistribution(); });
  }

  void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda

