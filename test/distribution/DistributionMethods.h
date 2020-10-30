/*
 * (C) Copyright 2009-2016 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

#ifndef TEST_DISTRIBUTION_DISTRIBUTIONMETHODS_H_
#define TEST_DISTRIBUTION_DISTRIBUTIONMETHODS_H_

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
#include "oops/util/Logger.h"

#include "ioda/distribution/Distribution.h"
#include "ioda/distribution/DistributionFactory.h"

namespace ioda {
namespace test {

// -----------------------------------------------------------------------------

void testDistributionMethods() {
  const eckit::LocalConfiguration conf(::test::TestEnvironment::config());
  std::vector<eckit::LocalConfiguration> dist_types;
  const eckit::mpi::Comm & MpiComm = oops::mpi::world();

  std::string TestDistType;
  std::string DistName;
  std::unique_ptr<ioda::Distribution> TestDist;
  DistributionFactory * DistFactory = nullptr;
  std::size_t MyRank = MpiComm.rank();
  bool complete;  // true means distribution does not need to comunicate
                  // (each processors has every observation)
  conf.get("distribution types", dist_types);
  for (std::size_t i = 0; i < dist_types.size(); ++i) {
    conf.get("distribution", dist_types);
    oops::Log::debug() << "Distribution::DistributionTypes: conf: "
                       << dist_types[i] << std::endl;
    DistName = dist_types[i].getString("name");
    complete = dist_types[i].getBool("complete");
    TestDist.reset(DistFactory->createDistribution(MpiComm, DistName));
    oops::Log::debug() << "Distribution::DistType: " << TestDistType << std::endl;

    // set up a,b,c on each processor (assuming 4 processors)
    double a = MyRank;
    float b = MyRank;
    int c = MyRank;
    std::vector<double> va(5, MyRank);
    std::vector<size_t> vb(5, MyRank);

    // vector solutions for sum
    std::vector<double> vaRefComplete(5, MyRank);
    std::vector<size_t> vbRefComplete(5, MyRank);
    std::vector<double> vaRef(5, 6);
    std::vector<size_t> vbRef(5, 6);

    if (complete) {
        // sum
        TestDist->sum(a);
        EXPECT(a == MyRank);  // 0 + 1 + 2 + 3
        TestDist->sum(c);
        EXPECT(c == MyRank);
        TestDist->sum(va);
        EXPECT(va == vaRefComplete);
        TestDist->sum(vb);
        EXPECT(vb == vbRefComplete);


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
        TestDist->sum(a);
        EXPECT(a == 6);  // 0 + 1 + 2 + 3
        TestDist->sum(c);
        EXPECT(c == 6);
        TestDist->sum(va);
        EXPECT(va == vaRef);
        TestDist->sum(vb);
        EXPECT(vb == vbRef);

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
        EXPECT(a == 3);
        TestDist->max(b);
        EXPECT(b == 3);
        TestDist->max(c);
        EXPECT(c == 3);
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
