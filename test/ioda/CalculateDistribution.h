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

#include "ioda/distribution/DistributionFactory.h"
#include "ioda/ioPool/ReaderPoolUtils.h"

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

  const std::vector<eckit::LocalConfiguration> confs =
      topLevelConf.getSubConfigurations("test cases");
  std::unique_ptr<ioda::Distribution> distribution;
  std::vector<float> latValues, lonValues;
  std::vector<std::size_t> sourceRecNums;
  for (const eckit::LocalConfiguration & conf : confs) {
    const eckit::LocalConfiguration configInputs = conf.getSubConfiguration("function inputs");
    lonValues = configInputs.getFloatVector("longitude");
    latValues = configInputs.getFloatVector("latitude");
    sourceRecNums = configInputs.getUnsignedVector("records vector");
    size_t globalNlocs = latValues.size();
    ASSERT(globalNlocs == lonValues.size());
    ASSERT(globalNlocs == sourceRecNums.size());
    std::vector<std::size_t> sourceLocIndices(globalNlocs);
    // This test is for the Load/Filter/Distribute paradigm, so assuming pre-filtered data,
    // so sourceLocIndices is just 0 to N-1
    std::iota(sourceLocIndices.begin(), sourceLocIndices.end(), 0);

    ioda::EmptyDistributionParameters distParams;
    distParams.deserialize(configInputs.getSubConfiguration("distribution"));
    std::shared_ptr<Distribution> distribution =
        DistributionFactory::create(oops::mpi::world(), distParams);

    std::vector<std::size_t> testLocalLocIndices, testLocalRecNums;
    std::size_t testLocalNlocs, testLocalNrecs;
    ioda::IoPool::applyMpiDistribution(distribution, false, lonValues, latValues,
                          sourceLocIndices, sourceRecNums,
                          testLocalLocIndices, testLocalRecNums,
                          testLocalNlocs, testLocalNrecs);

    std::string myResultsKey = "expected results.mpisize" + std::to_string(myMpiSize) +
                        ".rank" + std::to_string(myMpiRank);
    std::vector<std::size_t> expectedLocalLocIndices, expectedLocalRecNums;
    std::size_t expectedLocalNlocs, expectedLocalNrecs;
    const eckit::LocalConfiguration expectedConf =
        conf.getSubConfiguration(myResultsKey);
    expectedLocalLocIndices = expectedConf.getUnsignedVector("local loc indices");
    expectedLocalRecNums = expectedConf.getUnsignedVector("local record numbers");
    expectedLocalNlocs = expectedConf.getUnsigned("local num locations");
    expectedLocalNrecs = expectedConf.getUnsigned("local num records");

    EXPECT(testLocalLocIndices == expectedLocalLocIndices);
    EXPECT(testLocalRecNums == expectedLocalRecNums);
    EXPECT(testLocalNlocs == expectedLocalNlocs);
    EXPECT(testLocalNrecs == expectedLocalNrecs);
  }
}


// -----------------------------------------------------------------------------
class CalculateDistribution : public oops::Test {
 public:
  CalculateDistribution() {}
  virtual ~CalculateDistribution() {}

 private:
  std::string testid() const override {return "test::CalculateDistribution";}

  void register_tests() const override {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    ts.emplace_back(CASE("ioda/CalculateDistribution/testOsdfAssignRecords")
      { testCalculateDistribution(); });
  }

  void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda

