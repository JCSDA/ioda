/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef TEST_DISTRIBUTION_DISTRIBUTIONMETHODS_H_
#define TEST_DISTRIBUTION_DISTRIBUTIONMETHODS_H_

#include <algorithm>
#include <limits>
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

#include "ioda/distribution/Accumulator.h"
#include "ioda/distribution/Distribution.h"
#include "ioda/distribution/DistributionFactory.h"

namespace ioda {
namespace test {

// -----------------------------------------------------------------------------

template <typename T>
void testAccumulateScalar(const Distribution &TestDist, const std::vector<size_t> &myRecords,
                          size_t expectedSum) {
  auto accumulator = TestDist.createAccumulator<T>();
  for (size_t loc = 0; loc < myRecords.size(); ++loc)
    accumulator->addTerm(loc, myRecords[loc]);
  const T sum = accumulator->computeResult();
  EXPECT_EQUAL(sum, static_cast<T>(expectedSum));
}

template <typename T>
void testAccumulateVector(const Distribution &TestDist, const std::vector<size_t> &myRecords,
                          size_t expectedSum) {
  const size_t numSums = 3;
  std::vector<T> expectedSums(numSums);
  for (size_t i = 0; i < numSums; ++i)
    expectedSums[i] = (i + 1) * expectedSum;

  // Part 1: two-argument addTerm overload
  {
    auto accumulator = TestDist.createAccumulator<T>(numSums);
    std::vector<T> terms(numSums);
    for (size_t loc = 0; loc < myRecords.size(); ++loc) {
      for (size_t i = 0; i < numSums; ++i)
        terms[i] = (i + 1) * myRecords[loc];
      accumulator->addTerm(loc, terms);
    }
    std::vector<T> sums = accumulator->computeResult();

    EXPECT_EQUAL(sums, expectedSums);
  }

  // Part 2: three-argument addTerm overload
  {
    auto accumulator = TestDist.createAccumulator<T>(numSums);
    std::vector<T> terms(numSums);
    for (size_t loc = 0; loc < myRecords.size(); ++loc)
      for (size_t i = 0; i < numSums; ++i)
        accumulator->addTerm(loc, i, (i + 1) * myRecords[loc]);
    std::vector<T> sums = accumulator->computeResult();

    EXPECT_EQUAL(sums, expectedSums);
  }
}

template <typename T>
void testMaxScalar(const Distribution &TestDist, const std::vector<size_t> &myRecords,
                   size_t expectedMax) {
  // Perform a local reduction
  T max = std::numeric_limits<T>::lowest();
  for (size_t loc = 0; loc < myRecords.size(); ++loc)
    max = std::max<T>(max, myRecords[loc]);

  // Perform a global reduction
  TestDist.max(max);

  EXPECT_EQUAL(max, expectedMax);
}

template <typename T>
void testMaxVector(const Distribution &TestDist, const std::vector<size_t> &myRecords,
                   size_t expectedMax) {
  // Perform a local reduction
  T max = std::numeric_limits<T>::lowest();
  for (size_t loc = 0; loc < myRecords.size(); ++loc)
    max = std::max<T>(max, myRecords[loc]);

  // Perform a global reduction
  const T shift = 10;
  std::vector<T> maxes{max, max + shift};
  TestDist.max(maxes);
  const std::vector<T> expectedMaxes{static_cast<T>(expectedMax),
                                     static_cast<T>(expectedMax + shift)};

  EXPECT_EQUAL(maxes, expectedMaxes);
}

template <typename T>
void testMinScalar(const Distribution &TestDist, const std::vector<size_t> &myRecords,
                   size_t expectedMin) {
  // Perform a local reduction
  T min = std::numeric_limits<T>::max();
  for (size_t loc = 0; loc < myRecords.size(); ++loc)
    min = std::min<T>(min, myRecords[loc]);

  // Perform a global reduction
  TestDist.min(min);

  EXPECT_EQUAL(min, expectedMin);
}

template <typename T>
void testMinVector(const Distribution &TestDist, const std::vector<size_t> &myRecords,
                   size_t expectedMin) {
  // Perform a local reduction
  T min = std::numeric_limits<T>::max();
  for (size_t loc = 0; loc < myRecords.size(); ++loc)
    min = std::min<T>(min, myRecords[loc]);

  // Perform a global reduction
  const T shift = 10;
  std::vector<T> mins{min, min + shift};
  TestDist.min(mins);
  const std::vector<T> expectedMins{static_cast<T>(expectedMin),
                                    static_cast<T>(expectedMin + shift)};

  EXPECT_EQUAL(mins, expectedMins);
}

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
    std::vector<size_t> myRecords;
    for (std::size_t j = 0; j < Gnlocs; ++j) {
      glons[j] = j*360.0/Gnlocs;
      eckit::geometry::Point2 point(glons[j], glats[j]);
      TestDist->assignRecord(j, j, point);
      if (TestDist->isMyRecord(j))
        myRecords.push_back(j);
    }
    TestDist->computePatchLocs(Gnlocs);

    // Expected results
    // Accumulate: sum (0 + 1 + ... + nprocs - 1)
    size_t expectedSum = 0;
    for (std::size_t i = 0; i < nprocs; i++) {
       expectedSum += i;
    }
    // Max: max (0, 1, ..., nprocs - 1)
    const size_t expectedMax = nprocs - 1;
    // Min: min (0, 1, ..., nprocs - 1)
    const size_t expectedMin = 0;

    testAccumulateScalar<double>(*TestDist, myRecords, expectedSum);
    testAccumulateScalar<float>(*TestDist, myRecords, expectedSum);
    testAccumulateScalar<int>(*TestDist, myRecords, expectedSum);
    testAccumulateScalar<size_t>(*TestDist, myRecords, expectedSum);
    testAccumulateVector<double>(*TestDist, myRecords, expectedSum);
    testAccumulateVector<float>(*TestDist, myRecords, expectedSum);
    testAccumulateVector<int>(*TestDist, myRecords, expectedSum);
    testAccumulateVector<size_t>(*TestDist, myRecords, expectedSum);

    testMaxScalar<double>(*TestDist, myRecords, expectedMax);
    testMaxScalar<float>(*TestDist, myRecords, expectedMax);
    testMaxScalar<int>(*TestDist, myRecords, expectedMax);
    testMaxScalar<size_t>(*TestDist, myRecords, expectedMax);
    testMaxVector<double>(*TestDist, myRecords, expectedMax);
    testMaxVector<float>(*TestDist, myRecords, expectedMax);
    testMaxVector<int>(*TestDist, myRecords, expectedMax);
    testMaxVector<size_t>(*TestDist, myRecords, expectedMax);

    testMinScalar<double>(*TestDist, myRecords, expectedMin);
    testMinScalar<float>(*TestDist, myRecords, expectedMin);
    testMinScalar<int>(*TestDist, myRecords, expectedMin);
    testMinScalar<size_t>(*TestDist, myRecords, expectedMin);
    testMinVector<double>(*TestDist, myRecords, expectedMin);
    testMinVector<float>(*TestDist, myRecords, expectedMin);
    testMinVector<int>(*TestDist, myRecords, expectedMin);
    testMinVector<size_t>(*TestDist, myRecords, expectedMin);
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
