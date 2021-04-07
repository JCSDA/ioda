/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef TEST_DISTRIBUTION_DISTRIBUTIONMETHODS_H_
#define TEST_DISTRIBUTION_DISTRIBUTIONMETHODS_H_

#include <memory>
#include <string>
#include <vector>

#define ECKIT_TESTING_SELF_REGISTER_CASES 0

#include "eckit/config/LocalConfiguration.h"
#include "eckit/mpi/Comm.h"
#include "eckit/testing/Test.h"

#include "oops/mpi/mpi.h"
#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"
#include "oops/util/Logger.h"

#include "ioda/distribution/Distribution.h"
#include "ioda/distribution/DistributionFactory.h"

namespace ioda {
namespace test {

// -----------------------------------------------------------------------------

void testDistributionMethods() {
  eckit::LocalConfiguration conf(::test::TestEnvironment::config());
  std::vector<eckit::LocalConfiguration> dist_types;
  const eckit::mpi::Comm & MpiComm = oops::mpi::world();

  std::string DistName;
  std::unique_ptr<ioda::Distribution> TestDist;
  std::size_t MyRank = MpiComm.rank();
  std::size_t nprocs = MpiComm.size();
  conf.get("distribution types", dist_types);
  for (std::size_t i = 0; i < dist_types.size(); ++i) {
    conf.get("distribution", dist_types);
    oops::Log::debug() << "Distribution::DistributionTypes: conf: "
                       << dist_types[i] << std::endl;
    DistName = dist_types[i].getString("name");

    eckit::LocalConfiguration DistConfig;
    DistConfig.set("distribution", DistName);

    TestDist = DistributionFactory::create(MpiComm, DistConfig);

    // initialize distributions
    size_t Gnlocs = nprocs;
    std::vector<double> glats(Gnlocs, 0.0);
    std::vector<double> glons(Gnlocs, 0.0);
    for (std::size_t j = 0; j < Gnlocs; ++j) {
      glons[j] = j*360.0/Gnlocs;
      eckit::geometry::Point2 point(glons[j], glats[j]);
      TestDist->assignRecord(j, j, point);
    }
    TestDist->computePatchLocs(Gnlocs);

    // Inputs for the tests: double, float, int, vector double, vector size_t
    // set up a,b,c on each processor
    double a = MyRank;
    float b = MyRank;
    int c = MyRank;
    std::vector<double> va(Gnlocs, MyRank);
    std::vector<size_t> vb(Gnlocs, MyRank);

    // Test result: sum (0 + 1 + ..  nprocs -1)
    double result = 0;
    for (std::size_t i = 0; i < nprocs; i++) {
       result = result + i;
    }

    // vector solutions for sum
    std::vector<double> vaRefInefficient(Gnlocs, MyRank);
    std::vector<size_t> vbRefInefficient(Gnlocs, MyRank);
    std::vector<double> vaRef(Gnlocs, result);
    std::vector<size_t> vbRef(Gnlocs, result);

    if (DistName == "InefficientDistribution") {
        // sum
        TestDist->sum(a);
        EXPECT(a == MyRank);  // MyRank (sum should do nothing for Inefficient)
        TestDist->sum(c);
        EXPECT(c == MyRank);
        TestDist->sum(va);
        EXPECT(va == vaRefInefficient);
        TestDist->sum(vb);
        EXPECT(vb == vbRefInefficient);

        // min
        a = MyRank;
        b = MyRank;
        c = MyRank;
        TestDist->min(a);
        EXPECT(a == MyRank);
        TestDist->min(b);
        EXPECT(b == MyRank);
        TestDist->min(c);
        EXPECT(c == MyRank);

        // max
        a = MyRank;
        b = MyRank;
        c = MyRank;
        TestDist->max(a);
        EXPECT(a == MyRank);
        TestDist->max(b);
        EXPECT(b == MyRank);
        TestDist->max(c);
        EXPECT(c == MyRank);

    } else {
        // sum
        if (DistName != "Halo") {
          TestDist->sum(a);
          EXPECT(a == result);  // 0 + 1 + .. nprocs-1 (sum across tasks)
          TestDist->sum(c);
          EXPECT(c == result);
          TestDist->sum(va);
          TestDist->sum(vb);
          oops::Log::debug() << "va=" << va << " vaRef=" << vaRef << std::endl;
          EXPECT(va == vaRef);
          EXPECT(vb == vbRef);
        }

        // min
        a = MyRank;
        b = MyRank;
        c = MyRank;
        TestDist->min(a);
        EXPECT(a == 0);
        TestDist->min(b);
        EXPECT(b == 0);
        TestDist->min(c);
        EXPECT(c == 0);

        // max
        a = MyRank;
        b = MyRank;
        c = MyRank;
        TestDist->max(a);
        EXPECT(a == nprocs -1);
        TestDist->max(b);
        EXPECT(b == nprocs -1);
        TestDist->max(c);
        EXPECT(c == nprocs -1);
    }
  }
}


// -----------------------------------------------------------------------------

class DistributionMethods : public oops::Test {
 public:
  DistributionMethods() {}
  virtual ~DistributionMethods() {}
 private:
  std::string testid() const override {return "test::DistributionMethods";}

  void register_tests() const override {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    ts.emplace_back(CASE("distribution/Distribution/testDistributionMethods")
      { testDistributionMethods(); });
  }

  void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda

#endif  // TEST_DISTRIBUTION_DISTRIBUTIONMETHODS_H_
