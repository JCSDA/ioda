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
#include "ioda/distribution/IdentityDistribution.h"
#include "ioda/reader/distribute/distributeObs.hpp"
#include "ioda/test/containers/OsdfTestUtils.h"

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
  std::unique_ptr<osdf::IFrame> inOsdf = std::make_unique<osdf::FrameRows>();
  std::unique_ptr<osdf::IFrame> rememberOsdf = std::make_unique<osdf::FrameRows>();
  std::unique_ptr<osdf::IFrame> outOsdf = std::make_unique<osdf::FrameRows>();
  std::vector<std::string> inputColumnNames, rememberColumnNames;
  std::vector<std::string> inputColumnTypes, rememberColumnTypes;
  populateFrame(configInputFrameData, inOsdf, inputColumnNames, inputColumnTypes);
  populateFrame(configInputFrameData, rememberOsdf, rememberColumnNames, rememberColumnTypes);

  // Compare the two frames we just created to validate they're the same
  compareFrames(inOsdf, inputColumnNames, inputColumnTypes, rememberOsdf, rememberColumnNames,
                rememberColumnTypes, tolerance, false);

  // Create the other distributeObs function inputs
  // obsGroupVarList
  const std::vector<std::string> obsGroupVarList =
      configInputData.getStringVector("grouping variables");

  // osdfMetadata
  osdf::FrameMetadata osdfMetadata;

  // ObsSourceStats
  ioda::ObsSourceStats inObsSourceStats, rememberObsSourceStats, outObsSourceStats;
  inObsSourceStats.sourceNlocs = configInputData.getUnsigned("number of locations");
  rememberObsSourceStats.sourceNlocs = configInputData.getUnsigned("number of locations");

  // Distribution Parameters
  auto distParamsConfig = configInputData.getSubConfiguration("distribution");
  std::unique_ptr<DistributionParametersBase> distParams =
        DistributionFactory::createParameters(distParamsConfig.getString("name"));
  distParams->deserialize(distParamsConfig);

  std::shared_ptr<Distribution> inDist;  // gets created as nullptr
  std::shared_ptr<Distribution> outDist;
  std::unique_ptr<DistributionParametersBase> identityDistParams =
    createBaseDistributionParams("Identity");
  // Pass the Identity distribution parameters and expect it to throw an exception
  EXPECT_THROWS_AS(ioda::reader::distributeObs(*identityDistParams, oops::mpi::world(),
                                               obsGroupVarList,
                                               inObsSourceStats,
                                               inDist,
                                               inOsdf,
                                               outObsSourceStats,
                                               outDist,
                                               outOsdf),
                   eckit::Exception);

  // Pass the same ObsSourceStats and expect it to throw an exception
  EXPECT_THROWS_AS(ioda::reader::distributeObs(*distParams, oops::mpi::world(),
                                               obsGroupVarList,
                                               inObsSourceStats,
                                               inDist,
                                               inOsdf,
                                               inObsSourceStats,
                                               outDist,
                                               outOsdf),
                   eckit::Exception);

  // Pass the same distribution pointer and expect it to throw an exception
  EXPECT_THROWS_AS(ioda::reader::distributeObs(*distParams, oops::mpi::world(),
                                               obsGroupVarList,
                                               inObsSourceStats,
                                               inDist,
                                               inOsdf,
                                               outObsSourceStats,
                                               inDist,
                                               outOsdf),
                   eckit::Exception);

  // Pass the same OSDF container and expect it to throw an exception
  EXPECT_THROWS_AS(ioda::reader::distributeObs(*distParams, oops::mpi::world(),
                                               obsGroupVarList,
                                               inObsSourceStats,
                                               inDist,
                                               inOsdf,
                                               outObsSourceStats,
                                               outDist,
                                               inOsdf),
                   eckit::Exception);

  // Call the distributeObs function, passing a null inDist, and expect it to throw an exception
  EXPECT_THROWS_AS(ioda::reader::distributeObs(*distParams, oops::mpi::world(),
                                               obsGroupVarList,
                                               inObsSourceStats,
                                               inDist,
                                               inOsdf,
                                               outObsSourceStats,
                                               outDist,
                                               outOsdf),
                   eckit::Exception);

  // Now create inDist as IdentityDistribution and call the function again
  inDist = std::make_unique<IdentityDistribution>(oops::mpi::world(),
                                                  IdentityDistribution::Parameters_{});
  inDist->setNumberLocations(inOsdf->numRows());

  // Call the function correctly, using the overload that preserves the inputs
  ioda::reader::distributeObs(*distParams, oops::mpi::world(),
                             obsGroupVarList,
                             inObsSourceStats,
                             inDist,
                             inOsdf,
                             outObsSourceStats,
                             outDist,
                             outOsdf);

  // Make sure the input distribution, osdf, and ObsSourceStats were not modified
  EXPECT(inDist->name() == "Identity");
  compareFrames(inOsdf, inputColumnNames, inputColumnTypes, rememberOsdf, rememberColumnNames,
                rememberColumnTypes, tolerance, false);
  EXPECT(inObsSourceStats.sourceNlocs == rememberObsSourceStats.sourceNlocs);
  EXPECT(inObsSourceStats.nlocs == rememberObsSourceStats.nlocs);
  EXPECT(inObsSourceStats.locIndices == rememberObsSourceStats.locIndices);
  EXPECT(inObsSourceStats.nrecs == rememberObsSourceStats.nrecs);
  EXPECT(inObsSourceStats.recNums == rememberObsSourceStats.recNums);

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
  compareFrames(outOsdf, inputColumnNames, inputColumnTypes, refOutOsdf, refColumnNames,
                refColumnTypes, tolerance, false);

  std::vector<std::size_t> expectedLocalLocIndices, expectedLocalRecNums;
  std::size_t expectedLocalNlocs, expectedLocalNrecs;
  expectedLocalLocIndices = configExpectedResults.getUnsignedVector("local loc indices");
  expectedLocalRecNums = configExpectedResults.getUnsignedVector("local record numbers");
  expectedLocalNlocs = configExpectedResults.getUnsigned("local num locations");
  expectedLocalNrecs = configExpectedResults.getUnsigned("local num records");

  EXPECT(outObsSourceStats.sourceNlocs == rememberObsSourceStats.sourceNlocs);
  EXPECT(outObsSourceStats.locIndices == expectedLocalLocIndices);
  EXPECT(outObsSourceStats.recNums == expectedLocalRecNums);
  EXPECT(outObsSourceStats.nlocs == expectedLocalNlocs);
  EXPECT(outObsSourceStats.nrecs == expectedLocalNrecs);
  EXPECT(outDist->name() == distParams->name.value());

  // Now call the function using the overload that overwrites the inputs
  ioda::reader::distributeObs(*distParams, oops::mpi::world(),
                             obsGroupVarList,
                             inObsSourceStats,
                             inDist,
                             inOsdf);

  // Make sure the input distribution, osdf, and ObsSourceStats were modified
  compareFrames(inOsdf, inputColumnNames, inputColumnTypes, refOutOsdf, refColumnNames,
                refColumnTypes, tolerance, false);
  EXPECT(inObsSourceStats.sourceNlocs == rememberObsSourceStats.sourceNlocs);
  EXPECT(inObsSourceStats.nlocs == expectedLocalNlocs);
  EXPECT(inObsSourceStats.locIndices == expectedLocalLocIndices);
  EXPECT(inObsSourceStats.nrecs == expectedLocalNrecs);
  EXPECT(inObsSourceStats.recNums == expectedLocalRecNums);
  EXPECT(inDist->name() == distParams->name.value());
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

