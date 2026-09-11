/*
 * (C) Copyright 2018-2025 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef TEST_IODA_OBSSPACEHAS_H_
#define TEST_IODA_OBSSPACEHAS_H_

#include <cmath>
#include <set>
#include <string>
#include <vector>

#define ECKIT_TESTING_SELF_REGISTER_CASES 0

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

#include "eckit/config/LocalConfiguration.h"
#include "eckit/testing/Test.h"

#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"

#include "ioda/IodaTrait.h"
#include "ioda/ObsSpace.h"
#include "IodaTestUtils.h"

namespace eckit {
  // Don't use the contracted output for these types: the current implementation works only
  // with integer types.
  template <> struct VectorPrintSelector<float> { typedef VectorPrintSimple selector; };
  template <> struct VectorPrintSelector<util::DateTime> { typedef VectorPrintSimple selector; };
}  // namespace eckit

namespace ioda {
namespace test {

// -----------------------------------------------------------------------------

class ObsSpaceTestFixture : private boost::noncopyable {
 public:
  static ioda::ObsSpace & obspace(const std::size_t ii) {
    return *getInstance().ospaces_.at(ii);
  }
  static const eckit::LocalConfiguration & config(const std::size_t ii) {
    return getInstance().configs_.at(ii);
  }
  static std::size_t size() {return getInstance().ospaces_.size();}
  static void cleanup() {
    auto &spaces = getInstance().ospaces_;
    for (auto &space : spaces) {
      space->save();
      space.reset();
    }
  }

 private:
  static ObsSpaceTestFixture & getInstance() {
    static ObsSpaceTestFixture theObsSpaceTestFixture;
    return theObsSpaceTestFixture;
  }

  ObsSpaceTestFixture(): ospaces_() {
    const util::TimeWindow timeWindow
      (::test::TestEnvironment::config().getSubConfiguration("time window"));

    ::test::TestEnvironment::config().get("observations", configs_);

    for (std::size_t jj = 0; jj < configs_.size(); ++jj) {
      eckit::LocalConfiguration obsconf(configs_[jj], "obs space");
      applyContainerDefault(::test::TestEnvironment::config(), obsconf);
      boost::shared_ptr<ioda::ObsSpace> tmp(new ioda::ObsSpace(obsconf, oops::mpi::world(),
                                                               timeWindow, oops::mpi::myself()));
      ospaces_.push_back(tmp);
    }
  }

  ~ObsSpaceTestFixture() {}

  std::vector<eckit::LocalConfiguration> configs_;
  std::vector<boost::shared_ptr<ioda::ObsSpace> > ospaces_;
};

// -----------------------------------------------------------------------------

void testHasGroupVar() {
  typedef ObsSpaceTestFixture Test_;

  std::vector<eckit::LocalConfiguration> conf;
  ::test::TestEnvironment::config().get("observations", conf);

  for (std::size_t jj = 0; jj < Test_::size(); ++jj) {
    // Grab the test data configuration
    eckit::LocalConfiguration testConfig;
    conf[jj].get("test data", testConfig);

    const ObsSpace &odb = Test_::obspace(jj);

    // Try out various "has" tests according to the test data spec in config
    std::vector<eckit::LocalConfiguration> hasTestConfigs =
      testConfig.getSubConfigurations("has group var checks");
    for (auto & hasTestConfig : hasTestConfigs) {
      const std::string grpName = hasTestConfig.getString("group");
      const std::string varName = hasTestConfig.getString("variable");
      const bool expectedResult = hasTestConfig.getBool("result");

      const bool result = odb.has(grpName, varName);
      EXPECT_EQUAL(result, expectedResult);
    }
  }
}

// -----------------------------------------------------------------------------

void testHasGroup() {
  typedef ObsSpaceTestFixture Test_;

  std::vector<eckit::LocalConfiguration> conf;
  ::test::TestEnvironment::config().get("observations", conf);

  for (std::size_t jj = 0; jj < Test_::size(); ++jj) {
    // Grab the test data configuration
    eckit::LocalConfiguration testConfig;
    conf[jj].get("test data", testConfig);

    const ObsSpace &odb = Test_::obspace(jj);

    // Try out various "has" tests according to the test data spec in config
    std::vector<eckit::LocalConfiguration> hasTestConfigs =
      testConfig.getSubConfigurations("has group checks");
    for (auto & hasTestConfig : hasTestConfigs) {
      const std::string grpName = hasTestConfig.getString("group");
      const bool expectedResult = hasTestConfig.getBool("result");

      const bool result = odb.has(grpName);
      EXPECT_EQUAL(result, expectedResult);
    }
  }
}

// -----------------------------------------------------------------------------

class ObsSpaceHas : public oops::Test {
 public:
  ObsSpaceHas() {}
  virtual ~ObsSpaceHas() {}

 private:
  std::string testid() const override {return "test::ObsSpaceHas<ioda::IodaTrait>";}

  void register_tests() const override {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    ts.emplace_back(CASE("ioda/ObsSpaceHas/testHasGroupVar")
      { testHasGroupVar(); });
    ts.emplace_back(CASE("ioda/ObsSpaceHas/testHasGroup")
      { testHasGroup(); });
  }

  void clear() const override {
    typedef ObsSpaceTestFixture Test_;
    Test_::cleanup();
  }
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda

#endif  // TEST_IODA_OBSSPACEHAS_H_
