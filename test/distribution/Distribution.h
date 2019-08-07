/*
 * (C) Copyright 2009-2016 ECMWF.
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 * In applying this licence, ECMWF does not waive the privileges and immunities 
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

#ifndef TEST_INTERFACE_OBSSPACE_H_
#define TEST_INTERFACE_OBSSPACE_H_

#include <string>
#include <cmath>

#define ECKIT_TESTING_SELF_REGISTER_CASES 0

#include <boost/noncopyable.hpp>

#include "eckit/config/LocalConfiguration.h"
#include "eckit/mpi/Comm.h"
#include "eckit/testing/Test.h"
#include "oops/parallel/mpi/mpi.h"
#include "oops/runs/Test.h"
#include "oops/util/Logger.h"
#include "test/TestEnvironment.h"

#include "distribution/Distribution.h"
#include "distribution/DistributionFactory.h"

namespace ioda {
namespace test {

// -----------------------------------------------------------------------------

void testConstructor() {
  const eckit::LocalConfiguration conf(::test::TestEnvironment::config());
  std::vector<eckit::LocalConfiguration> dist_types;
  eckit::mpi::Comm & MpiComm = eckit::mpi::comm();

  std::string TestDistType;
  std::string DistName;
  std::unique_ptr<ioda::Distribution> TestDist;
  DistributionFactory * DistFactory;

  // Walk through the different distribution types and try constructing.
  conf.get("DistributionTypes", dist_types);
  for (std::size_t i = 0; i < dist_types.size(); ++i) {
    oops::Log::debug() << "Distribution::DistributionTypes: conf: " << dist_types[i] << std::endl;

    TestDistType = dist_types[i].getString("DistType");
    oops::Log::debug() << "Distribution::DistType: " << TestDistType << std::endl;

    DistName = dist_types[i].getString("Specs.dist_name");
    if (dist_types[i].has("Specs.obs_grouping")) {
      std::vector<std::size_t> Groups = dist_types[i].getUnsignedVector("Specs.obs_grouping");
      TestDist.reset(DistFactory->createDistribution(MpiComm, 0, DistName, Groups));
    } else {
      TestDist.reset(DistFactory->createDistribution(MpiComm, 0, DistName));
    }
    EXPECT(TestDist.get());
    }
  }

// -----------------------------------------------------------------------------

void testDistribution() {
  const eckit::LocalConfiguration conf(::test::TestEnvironment::config());
  std::vector<eckit::LocalConfiguration> dist_types;
  eckit::mpi::Comm & MpiComm = eckit::mpi::comm();

  std::string TestDistType;
  std::string DistName;
  std::unique_ptr<ioda::Distribution> TestDist;
  DistributionFactory * DistFactory;

  std::size_t MyRank = MpiComm.rank();

  // Walk through the different distribution types and try constructing.
  conf.get("DistributionTypes", dist_types);
  for (std::size_t i = 0; i < dist_types.size(); ++i) {
    oops::Log::debug() << "Distribution::DistributionTypes: conf" << dist_types[i] << std::endl;

    TestDistType = dist_types[i].getString("DistType");
    oops::Log::debug() << "Distribution::DistType: " << TestDistType << std::endl;

    std::size_t Nlocs = dist_types[i].getInt("Specs.nlocs");
    DistName = dist_types[i].getString("Specs.dist_name");
    if (dist_types[i].has("Specs.obs_grouping")) {
      std::vector<std::size_t> Groups = dist_types[i].getUnsignedVector("Specs.obs_grouping");
      TestDist.reset(DistFactory->createDistribution(MpiComm, Nlocs, DistName, Groups));
    } else {
      TestDist.reset(DistFactory->createDistribution(MpiComm, Nlocs, DistName));
    }
    EXPECT(TestDist.get());

    // Expected results are listed in "Specs.index" with the MPI rank number
    // appended on the end.
    std::string MyIndexName = "Specs.index" + std::to_string(MyRank);
    std::vector<int> ExpectedIndex = dist_types[i].getIntVector(MyIndexName);

    // Form the distribution
    TestDist->distribution();
    std::vector<int> Index(TestDist->size());
    for (std::size_t i = 0; i < TestDist->size(); i++) {
      Index[i] = TestDist->index()[i];
    }
    EXPECT(Index == ExpectedIndex);
  }
}

// -----------------------------------------------------------------------------

class Distribution : public oops::Test {
 public:
  Distribution() {}
  virtual ~Distribution() {}
 private:
  std::string testid() const {return "test::Distribution";}

  void register_tests() const {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    ts.emplace_back(CASE("distribution/Distribution/testConstructor")
      { testConstructor(); });
    ts.emplace_back(CASE("distribution/Distribution/testDistribution")
      { testDistribution(); });
  }
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda

#endif  // TEST_INTERFACE_OBSSPACE_H_
