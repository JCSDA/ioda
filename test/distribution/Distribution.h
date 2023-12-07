/*
 * (C) Copyright 2009-2016 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

#ifndef TEST_DISTRIBUTION_DISTRIBUTION_H_
#define TEST_DISTRIBUTION_DISTRIBUTION_H_

#include <algorithm>
#include <cmath>
#include <memory>
#include <numeric>
#include <set>
#include <string>
#include <vector>

#define ECKIT_TESTING_SELF_REGISTER_CASES 0

#include <boost/noncopyable.hpp>

#include "eckit/config/LocalConfiguration.h"
#include "eckit/geometry/Point2.h"
#include "eckit/mpi/Comm.h"
#include "eckit/testing/Test.h"

#include "oops/mpi/mpi.h"
#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"
#include "oops/util/DateTime.h"
#include "oops/util/Logger.h"

#include "ioda/distribution/Distribution.h"
#include "ioda/distribution/DistributionFactory.h"
#include "ioda/ObsSpace.h"

namespace eckit
{
  // Don't use the contracted output for these types: the current implementation works only
  // with integer types.
  // TODO(wsmigaj) Report this (especially for floats) as a bug in eckit?
  template <> struct VectorPrintSelector<float> { typedef VectorPrintSimple selector; };
  template <> struct VectorPrintSelector<util::DateTime> { typedef VectorPrintSimple selector; };
  template <> struct VectorPrintSelector<util::Duration> { typedef VectorPrintSimple selector; };
}  // namespace eckit

namespace ioda {
namespace test {

// -----------------------------------------------------------------------------

void testConstructor() {
  const eckit::LocalConfiguration conf(::test::TestEnvironment::config());

  const eckit::mpi::Comm & MpiComm = oops::mpi::world();
  const std::size_t MyRank = MpiComm.rank();

  // Walk through the different distribution types and try constructing.
  std::vector<eckit::LocalConfiguration> dist_types;
  conf.get("distribution types", dist_types);
  for (std::size_t i = 0; i < dist_types.size(); ++i) {
    oops::Log::debug() << "Distribution::DistributionTypes: conf: " << dist_types[i] << std::endl;

    const std::string MyRankCfgName = "specs.rank" + std::to_string(MyRank) +
                                      ".config.distribution";
    eckit::LocalConfiguration DistConfig(dist_types[i], MyRankCfgName);
    const std::string TestDistType = DistConfig.getString("name");
    oops::Log::debug() << "Distribution::DistType: " << TestDistType << std::endl;

    DistributionParametersWrapper params;
    params.validateAndDeserialize(DistConfig);
    std::unique_ptr<ioda::Distribution> TestDist =
                    DistributionFactory::create(MpiComm, params.params);
    EXPECT(TestDist.get());
  }
}

// -----------------------------------------------------------------------------

void testDistribution(const eckit::Configuration &config,
                      const eckit::Configuration &MyRankConfig,
                      const ioda::Distribution *TestDist,
                      const std::vector<std::size_t> &Index,
                      const std::vector<std::size_t> &Recnums) {
  // expected answers
  std::size_t ExpectedNlocs = MyRankConfig.getUnsigned("nlocs");
  std::size_t ExpectedNrecs = MyRankConfig.getUnsigned("nrecs");
  std::size_t ExpectedNPatchLocs = MyRankConfig.getUnsigned("nPatchLocs");
  std::vector<std::size_t> ExpectedIndex =
                               MyRankConfig.getUnsignedVector("index");
  std::vector<std::size_t> ExpectedRecnums =
                               MyRankConfig.getUnsignedVector("recnums");
  std::vector<std::size_t> ExpectedPatchIndex =
                               MyRankConfig.getUnsignedVector("patchIndex");
  const std::vector<std::size_t> ExpectedAllGatherv =
      config.getUnsignedVector("specs.allgatherv");

  std::vector<bool> patchBool(Index.size());
  std::vector<std::size_t> PatchLocsThisPE;
  TestDist->patchObs(patchBool);
  for (std::size_t j = 0; j < Index.size(); ++j) {
    if (patchBool[j]) {PatchLocsThisPE.push_back(Index[j]);}
  }

  // Check the location and record counts
  std::size_t Nlocs = Index.size();
  std::size_t Nrecs = std::set<std::size_t>(Recnums.begin(), Recnums.end()).size();
  std::size_t NPatchLocs = PatchLocsThisPE.size();

  oops::Log::debug() << "Location Index: " << Index << std::endl;
  oops::Log::debug() << "PatchLocsThisPE: " << PatchLocsThisPE << std::endl;
  oops::Log::debug() << "Nlocs: " << Nlocs << " Nrecs: " << Nrecs
                     << " NPatchLocs" << NPatchLocs << std::endl;

  EXPECT_EQUAL(Nlocs, ExpectedNlocs);
  EXPECT_EQUAL(Nrecs, ExpectedNrecs);
  EXPECT_EQUAL(NPatchLocs, ExpectedNPatchLocs);

  // Check the resulting index and recnum vectors
  EXPECT_EQUAL(Index, ExpectedIndex);
  EXPECT_EQUAL(Recnums, ExpectedRecnums);
  EXPECT_EQUAL(PatchLocsThisPE, ExpectedPatchIndex);

  // Test overloads of the allGatherv() method. We will pass to it vectors derived from the
  // Index vector and compare the results against vectors derived from ExpectedAllGatherv.

  // Overload taking an std::vector<size_t>
  {
    const std::vector<std::size_t> ExpectedAllGathervSizeT = ExpectedAllGatherv;
    std::vector<size_t> AllGathervSizeT = Index;
    TestDist->allGatherv(AllGathervSizeT);
    EXPECT_EQUAL(AllGathervSizeT, ExpectedAllGathervSizeT);

    // Take advantage of the output produced by allGatherv() to test
    // globalUniqueConsecutiveLocationIndex(). This function is expected to map the index of each
    // location held on the calling process to the index of the corresponding element of the vector
    // produced by allGatherv().

    std::vector<size_t> ExpectedGlobalUniqueConsecutiveLocationIndices(Nlocs);
    std::vector<size_t> GlobalUniqueConsecutiveLocationIndices(Nlocs);
    for (size_t loc = 0; loc < Nlocs; ++loc) {
      const std::vector<size_t>::iterator it = std::find(
            AllGathervSizeT.begin(), AllGathervSizeT.end(), Index[loc]);
      ASSERT(it != AllGathervSizeT.end());
      ExpectedGlobalUniqueConsecutiveLocationIndices[loc] = it - AllGathervSizeT.begin();
      GlobalUniqueConsecutiveLocationIndices[loc] =
          TestDist->globalUniqueConsecutiveLocationIndex(loc);
    }

    EXPECT_EQUAL(GlobalUniqueConsecutiveLocationIndices,
                 ExpectedGlobalUniqueConsecutiveLocationIndices);
  }

  // Overload taking an std::vector<int>
  {
    std::vector<int> ExpectedAllGathervInt(ExpectedAllGatherv.begin(),
                                           ExpectedAllGatherv.end());
    std::vector<int> AllGathervInt(Index.begin(), Index.end());
    TestDist->allGatherv(AllGathervInt);
    EXPECT_EQUAL(AllGathervInt, ExpectedAllGathervInt);
  }

  // Overload taking an std::vector<float>
  {
    std::vector<float> ExpectedAllGathervFloat(ExpectedAllGatherv.begin(),
                                               ExpectedAllGatherv.end());
    std::vector<float> AllGathervFloat(Index.begin(), Index.end());
    TestDist->allGatherv(AllGathervFloat);
    EXPECT_EQUAL(AllGathervFloat, ExpectedAllGathervFloat);
  }

  // Overload taking an std::vector<double>
  {
    std::vector<double> ExpectedAllGathervDouble(ExpectedAllGatherv.begin(),
                                                 ExpectedAllGatherv.end());
    std::vector<double> AllGathervDouble(Index.begin(), Index.end());
    TestDist->allGatherv(AllGathervDouble);
    EXPECT_EQUAL(AllGathervDouble, ExpectedAllGathervDouble);
  }

  // Overload taking an std::vector<std::string>
  {
    auto numberToString = [](std::size_t x) { return std::to_string(x); };
    std::vector<std::string> ExpectedAllGathervString;
    std::transform(ExpectedAllGatherv.begin(), ExpectedAllGatherv.end(),
                   std::back_inserter(ExpectedAllGathervString), numberToString);
    std::vector<std::string> AllGathervString;
    std::transform(Index.begin(), Index.end(),
                   std::back_inserter(AllGathervString), numberToString);
    TestDist->allGatherv(AllGathervString);
    EXPECT_EQUAL(AllGathervString, ExpectedAllGathervString);
  }

  // Overload taking an std::vector<util::DateTime>
  {
    auto numberToDateTime = [](std::size_t x) { return util::DateTime(2000, 1, 1, 0, 0, x); };
    std::vector<util::DateTime> ExpectedAllGathervDateTime;
    std::transform(ExpectedAllGatherv.begin(), ExpectedAllGatherv.end(),
                   std::back_inserter(ExpectedAllGathervDateTime), numberToDateTime);
    std::vector<util::DateTime> AllGathervDateTime;
    std::transform(Index.begin(), Index.end(),
                   std::back_inserter(AllGathervDateTime), numberToDateTime);
    TestDist->allGatherv(AllGathervDateTime);
    EXPECT_EQUAL(AllGathervDateTime, ExpectedAllGathervDateTime);
  }
}

// -----------------------------------------------------------------------------

void testDistributionConstructedManually() {
  const eckit::LocalConfiguration conf(::test::TestEnvironment::config());

  const eckit::mpi::Comm & MpiComm = oops::mpi::world();
  const std::size_t MyRank = MpiComm.rank();

  std::string TestDistType;
  std::string DistName;
  std::unique_ptr<ioda::Distribution> TestDist;

  // Walk through the different distribution types and try constructing.
  std::vector<eckit::LocalConfiguration> dist_types;
  conf.get("distribution types", dist_types);
  for (std::size_t i = 0; i < dist_types.size(); ++i) {
    oops::Log::debug() << "Distribution::DistributionTypes: conf: "
                       << dist_types[i] << std::endl;

    // Expected results are listed in "specs.rank*", where * stands for the MPI rank number
    const std::string MyRankCfgName = "specs.rank" + std::to_string(MyRank);
    const eckit::LocalConfiguration MyRankConfig = dist_types[i].getSubConfiguration(MyRankCfgName);
    oops::Log::debug() << "Distribution::DistributionTypes: "
                       << MyRankCfgName << ": " << MyRankConfig << std::endl;

    const eckit::LocalConfiguration DistConfig(MyRankConfig, "config.distribution");
    const std::string DistName = DistConfig.getString("name");
    oops::Log::debug() << "Distribution::DistType: " << DistName << std::endl;

    DistributionParametersWrapper params;
    params.validateAndDeserialize(DistConfig);
    std::unique_ptr<ioda::Distribution> TestDist =
                    DistributionFactory::create(MpiComm, params.params);
    EXPECT(TestDist.get());

    // read lat/lon
    std::size_t Gnlocs = dist_types[i].getInt("specs.gnlocs");
    std::vector<double> glats(Gnlocs, 0);
    std::vector<double> glons(Gnlocs, 0);

    dist_types[i].get("specs.latitude", glats);
    dist_types[i].get("specs.longitude", glons);

    // If obsgrouping is specified then read the record grouping directly from
    // the config file. Otherwise, assign 0 to Gnlocs-1 into the record grouping
    // vector.
    std::vector<std::size_t> Groups(Gnlocs, 0);
    if (dist_types[i].has("specs.obsgrouping")) {
      Groups = dist_types[i].getUnsignedVector("specs.obsgrouping");
    } else {
      std::iota(Groups.begin(), Groups.end(), 0);
    }

    // Loop on gnlocs, and keep the indecies according to the distribution type.
    std::vector<std::size_t> Index;
    std::vector<std::size_t> Recnums;
    for (std::size_t j = 0; j < Gnlocs; ++j) {
      std::size_t RecNum = Groups[j];
      eckit::geometry::Point2 point(glons[j], glats[j]);
      TestDist->assignRecord(RecNum, j, point);
      if (TestDist->isMyRecord(RecNum)) {
        Index.push_back(j);
        Recnums.push_back(RecNum);
      }
    }
    TestDist->computePatchLocs();

    testDistribution(dist_types[i], MyRankConfig, TestDist.get(), Index, Recnums);
  }    // loop distributions
}      // testDistributionConstructedManually

// -----------------------------------------------------------------------------

// This test can be used to test distributions that cannot be constructed by the
// DistributionFactory, but need to be constructed by an ObsSpace. For example, the
// MasterAndReplicaDistribution.
void testDistributionConstructedByObsSpace() {
  const eckit::LocalConfiguration topLevelConf(::test::TestEnvironment::config());

  const util::TimeWindow timeWindow(topLevelConf.getSubConfiguration("time window"));

  const eckit::mpi::Comm & MpiComm = oops::mpi::world();
  const std::size_t MyRank = MpiComm.rank();

  const eckit::LocalConfiguration & obsConf = topLevelConf.getSubConfiguration("observations");

  for (const eckit::LocalConfiguration & conf : obsConf.getSubConfigurations()) {
    eckit::LocalConfiguration obsspaceConf(conf, "obs space");
    ioda::ObsTopLevelParameters obsParams;
    obsParams.validateAndDeserialize(obsspaceConf);
    ioda::ObsSpace obsspace(obsParams, MpiComm, timeWindow, oops::mpi::myself());

    // Expected results are listed in "specs.index" with the MPI rank number
    // appended on the end.
    std::string MyRankConfName = "specs.rank" + std::to_string(MyRank);
    eckit::LocalConfiguration MyRankConf(conf, MyRankConfName);
    oops::Log::debug() << "MyRankConf: "
                       << MyRankConfName << ": " << MyRankConf << std::endl;

    testDistribution(conf, MyRankConf, obsspace.distribution().get(),
                     obsspace.index(), obsspace.recnum());
  }
}

// -----------------------------------------------------------------------------

class Distribution : public oops::Test {
 public:
  Distribution() {}
  virtual ~Distribution() {}
 private:
  std::string testid() const override {return "test::Distribution";}

  void register_tests() const override {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    ts.emplace_back(CASE("distribution/Distribution/testConstructor")
      { testConstructor(); });
    ts.emplace_back(CASE("distribution/Distribution/testDistributionConstructedManually")
      { testDistributionConstructedManually(); });
    ts.emplace_back(CASE("distribution/Distribution/testDistributionConstructedByObsSpace")
      { testDistributionConstructedByObsSpace(); });
  }

  void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda

#endif  // TEST_DISTRIBUTION_DISTRIBUTION_H_
