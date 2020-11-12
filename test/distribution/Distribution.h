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
#include "eckit/mpi/Comm.h"
#include "eckit/testing/Test.h"

#include "oops/mpi/mpi.h"
#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"
#include "oops/util/DateTime.h"
#include "oops/util/Logger.h"

#include "ioda/distribution/Distribution.h"
#include "ioda/distribution/DistributionFactory.h"

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
  std::vector<eckit::LocalConfiguration> dist_types;
  const eckit::mpi::Comm & MpiComm = oops::mpi::world();

  std::string TestDistType;
  std::string DistName;
  std::unique_ptr<ioda::Distribution> TestDist;
  DistributionFactory * DistFactory = nullptr;

  // Walk through the different distribution types and try constructing.
  conf.get("distribution types", dist_types);
  for (std::size_t i = 0; i < dist_types.size(); ++i) {
    oops::Log::debug() << "Distribution::DistributionTypes: conf: " << dist_types[i] << std::endl;

    TestDistType = dist_types[i].getString("distribution");
    oops::Log::debug() << "Distribution::DistType: " << TestDistType << std::endl;

    DistName = dist_types[i].getString("specs.name");
    TestDist.reset(DistFactory->createDistribution(MpiComm, DistName));
    EXPECT(TestDist.get());
    }
  }

// -----------------------------------------------------------------------------

void testDistribution() {
  const eckit::LocalConfiguration conf(::test::TestEnvironment::config());
  std::vector<eckit::LocalConfiguration> dist_types;
  const eckit::mpi::Comm & MpiComm = oops::mpi::world();

  std::string TestDistType;
  std::string DistName;
  std::unique_ptr<ioda::Distribution> TestDist;
  DistributionFactory * DistFactory = nullptr;

  std::size_t MyRank = MpiComm.rank();

  // Walk through the different distribution types and try constructing.
  conf.get("distribution types", dist_types);
  for (std::size_t i = 0; i < dist_types.size(); ++i) {
    oops::Log::debug() << "Distribution::DistributionTypes: conf: "
                       << dist_types[i] << std::endl;

    TestDistType = dist_types[i].getString("distribution");
    oops::Log::debug() << "Distribution::DistType: " << TestDistType << std::endl;

    DistName = dist_types[i].getString("specs.name");
    TestDist.reset(DistFactory->createDistribution(MpiComm, DistName));
    EXPECT(TestDist.get());

    // Expected results are listed in "specs.index" with the MPI rank number
    // appended on the end.
    std::string MyRankCfgName = "specs.rank" + std::to_string(MyRank);
    eckit::LocalConfiguration MyRankConfig = dist_types[i].getSubConfiguration(MyRankCfgName);
    oops::Log::debug() << "Distribution::DistributionTypes: "
                       << MyRankCfgName << ": " << MyRankConfig << std::endl;

    std::size_t ExpectedNlocs = MyRankConfig.getUnsigned("nlocs");
    std::size_t ExpectedNrecs = MyRankConfig.getUnsigned("nrecs");
    std::vector<std::size_t> ExpectedIndex =
                                 MyRankConfig.getUnsignedVector("index");
    std::vector<std::size_t> ExpectedRecnums =
                                 MyRankConfig.getUnsignedVector("recnums");
    const std::vector<std::size_t> ExpectedAllGatherv =
        dist_types[i].getUnsignedVector("specs.allgatherv");
    const std::size_t ExpectedExclusiveScan = MyRankConfig.getUnsigned("exclusivescan");

    // If obsgrouping is specified then read the record grouping directly from
    // the config file. Otherwise, assign 0 to Gnlocs-1 into the record grouping
    // vector.
    std::size_t Gnlocs = dist_types[i].getInt("specs.gnlocs");
    std::vector<std::size_t> Groups(Gnlocs, 0);
    if (dist_types[i].has("specs.obsgrouping")) {
      Groups = dist_types[i].getUnsignedVector("specs.obsgrouping");
    } else {
      std::iota(Groups.begin(), Groups.end(), 0);
    }

    // Loop on gnlocs, and keep the indecies according to the distribution type.
    std::vector<std::size_t> Index;
    std::vector<std::size_t> Recnums;
    std::set<std::size_t> UniqueRecnums;
    for (std::size_t j = 0; j < Gnlocs; ++j) {
      std::size_t RecNum = Groups[j];
      if (TestDist->isMyRecord(RecNum)) {
        Index.push_back(j);
        Recnums.push_back(RecNum);
        UniqueRecnums.insert(RecNum);
      }
    }

    // Check the location and record counts
    std::size_t Nlocs = Index.size();
    std::size_t Nrecs = UniqueRecnums.size();
    EXPECT(Nlocs == ExpectedNlocs);
    EXPECT(Nrecs == ExpectedNrecs);

    // Check the resulting index and recnum vectors
    for (std::size_t j = 0; j < ExpectedIndex.size(); ++j) {
      EXPECT(Index[j] == ExpectedIndex[j]);
    }

    for (std::size_t j = 0; j < ExpectedRecnums.size(); ++j) {
      EXPECT(Recnums[j] == ExpectedRecnums[j]);
    }

    // Test overloads of the allGatherv() method. We will pass to it vectors derived from the Index
    // vector and compare the results against vectors derived from ExpectedAllGatherv.

    // Overload taking an std::vector<size_t>
    {
      const std::vector<std::size_t> ExpectedAllGathervSizeT = ExpectedAllGatherv;
      std::vector<size_t> AllGathervSizeT = Index;
      TestDist->allGatherv(AllGathervSizeT);
      EXPECT_EQUAL(AllGathervSizeT, ExpectedAllGathervSizeT);
    }

    // Overload taking an std::vector<int>
    {
      std::vector<int> ExpectedAllGathervInt(ExpectedAllGatherv.begin(), ExpectedAllGatherv.end());
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

    // Test the exclusiveScan() method
    {
      std::size_t ExclusiveScan = Nlocs;
      TestDist->exclusiveScan(ExclusiveScan);
      EXPECT_EQUAL(ExclusiveScan, ExpectedExclusiveScan);
    }
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
    ts.emplace_back(CASE("distribution/Distribution/testDistribution")
      { testDistribution(); });
  }

  void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda

#endif  // TEST_DISTRIBUTION_DISTRIBUTION_H_
