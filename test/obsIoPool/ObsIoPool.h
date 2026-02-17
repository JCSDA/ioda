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

#include <boost/noncopyable.hpp>

#include "eckit/config/LocalConfiguration.h"
#include "eckit/exception/Exceptions.h"
#include "eckit/testing/Test.h"

#include "ioda/ioPool/IoPoolParameters.h"
#include "ioda/obsIoPool/ObsIoPool.hpp"

#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"

namespace ioda {
namespace test {

// -----------------------------------------------------------------------------
void testConstructor() {
  // Configuration contains a list of variables (columns) that
  // specify the data to be added to the data frame. Plus a
  // tolerance value for floating point comparisons.
  std::vector<eckit::LocalConfiguration> constructorTestsConfig;
  ::test::TestEnvironment::config().get("constructor tests", constructorTestsConfig);

  // Run the test cases
  const eckit::mpi::Comm & commAll = oops::mpi::world();
  for (const auto& config : constructorTestsConfig) {
    const std::string testName = config.getString("name");
    oops::Log::info() << "Testing: " << testName << std::endl;

    const eckit::LocalConfiguration ioPoolConfig = config.getSubConfiguration("io pool");
    ioda::IoPool::IoPoolParameters configParams;
    configParams.validateAndDeserialize(ioPoolConfig);
    std::unique_ptr<ioda::ObsIoPool::ObsIoPool> obsIoPool =
      std::make_unique<ioda::ObsIoPool::ObsIoPool>(configParams, commAll);

    // run checks
    const eckit::LocalConfiguration testConfig = config.getSubConfiguration("test data");
    const std::size_t expectedAllRank = commAll.rank();
    const std::size_t expectedAllSize = commAll.size();
    const std::string sizeRankKey = "mpi size" + std::to_string(expectedAllSize) +
                                    ".rank" + std::to_string(expectedAllRank);
    const std::size_t expectedPoolRank = testConfig.getUnsigned(sizeRankKey + ".comm pool rank");
    const std::size_t expectedPoolSize = testConfig.getUnsigned(sizeRankKey + ".comm pool size");
    const std::string inIoPoolKey = testConfig.getString(sizeRankKey + ".in io pool");
    const bool expectedInIoPool = (inIoPoolKey == "true");

    EXPECT(obsIoPool.get() != nullptr);
    EXPECT(obsIoPool->commAll().rank() == expectedAllRank);
    EXPECT(obsIoPool->commAll().size() == expectedAllSize);
    EXPECT(obsIoPool->commPool().rank() == expectedPoolRank);
    EXPECT(obsIoPool->commPool().size() == expectedPoolSize);
    EXPECT(obsIoPool->inIoPool() == expectedInIoPool);
  }
}

// -----------------------------------------------------------------------------
class ObsIoPool : public oops::Test {
 public:
  ObsIoPool() {}
  virtual ~ObsIoPool() {}

 private:
  std::string testid() const override {return "test::ObsIoPool";}

  void register_tests() const override {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    ts.emplace_back(CASE("ioda/ObsIoPool/testConstructor")
      { testConstructor(); });
  }

  void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda

