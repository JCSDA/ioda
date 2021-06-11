/*
 * (C) Copyright 2018-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef TEST_IODA_OBSSPACEINVALIDNUMERIC_H_
#define TEST_IODA_OBSSPACEINVALIDNUMERIC_H_

#include <cmath>
#include <string>
#include <vector>

#define ECKIT_TESTING_SELF_REGISTER_CASES 0

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

#include "eckit/config/LocalConfiguration.h"
#include "eckit/testing/Test.h"

#include "oops/mpi/mpi.h"
#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"

#include "ioda/IodaTrait.h"
#include "ioda/ObsSpace.h"

namespace ioda {
namespace test {

// -----------------------------------------------------------------------------

class ObsSpaceTestFixture : private boost::noncopyable {
 public:
  static ioda::ObsSpace & obspace(const std::size_t ii) {
    return *getInstance().ospaces_.at(ii);
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
    util::DateTime bgn(::test::TestEnvironment::config().getString("window begin"));
    util::DateTime end(::test::TestEnvironment::config().getString("window end"));

    std::vector<eckit::LocalConfiguration> conf;
    ::test::TestEnvironment::config().get("observations", conf);

    for (std::size_t jj = 0; jj < conf.size(); ++jj) {
      eckit::LocalConfiguration obsconf(conf[jj], "obs space");
      std::string distname = obsconf.getString("distribution", "RoundRobin");
      boost::shared_ptr<ioda::ObsSpace> tmp(new ioda::ObsSpace(obsconf, oops::mpi::world(),
                                                               bgn, end, oops::mpi::myself()));
      ospaces_.push_back(tmp);
    }
  }

  ~ObsSpaceTestFixture() {}

  std::vector<boost::shared_ptr<ioda::ObsSpace> > ospaces_;
};

// -----------------------------------------------------------------------------

void testInvalidNumeric() {
  typedef ObsSpaceTestFixture Test_;

  std::vector<eckit::LocalConfiguration> conf;
  ::test::TestEnvironment::config().get("observations", conf);

  for (std::size_t jj = 0; jj < Test_::size(); ++jj) {
    // Grab the obs space and test data configurations
    eckit::LocalConfiguration obsConfig;
    eckit::LocalConfiguration testConfig;
    conf[jj].get("obs space", obsConfig);
    conf[jj].get("test data", testConfig);

    const ObsSpace &odb = Test_::obspace(jj);

    // Get the list of variables to check from the test configuration
    std::vector<eckit::LocalConfiguration> testConfigVars;
    testConfig.get("variables", testConfigVars);
    float testTol = testConfig.getFloat("tolerance");

    // Check that that invalid numeric values got handled properly
    for (std::size_t i = 0; i < testConfigVars.size(); ++i) {
      std::string varName = testConfigVars[i].getString("name");
      std::string groupName = testConfigVars[i].getString("group");
      std::string varType = testConfigVars[i].getString("type");

      if (varType == "int") {
        std::vector<int> expectedTestVals = testConfigVars[i].getIntVector("values");
        std::vector<int> testVals;
        odb.get_db(groupName, varName, testVals);
        EXPECT_EQUAL(testVals, expectedTestVals);
      } else if (varType == "float") {
        std::vector<float> expectedTestVals = testConfigVars[i].getFloatVector("values");
        std::vector<float> testVals;
        odb.get_db(groupName, varName, testVals);
        for (std::size_t j = 0; j < expectedTestVals.size(); ++j) {
          EXPECT(oops::is_close(testVals[j], expectedTestVals[j], testTol));
        }
      } else if (varType == "string") {
        std::vector<std::string> expectedTestVals =
                       testConfigVars[i].getStringVector("values");
        std::vector<std::string> testVals;
        odb.get_db(groupName, varName, testVals);
        EXPECT_EQUAL(testVals, expectedTestVals);
      }
    }
  }
}

// -----------------------------------------------------------------------------

void testCleanup() {
  // This test removes the obsspaces and ensures that they evict their contents
  // to disk successfully.
  typedef ObsSpaceTestFixture Test_;

  Test_::cleanup();
}

// -----------------------------------------------------------------------------

class ObsSpaceInvalidNumeric : public oops::Test {
 public:
  ObsSpaceInvalidNumeric() {}
  virtual ~ObsSpaceInvalidNumeric() {}

 private:
  std::string testid() const override {return "test::ObsSpaceInvalidNumeric<ioda::IodaTrait>";}

  void register_tests() const override {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    ts.emplace_back(CASE("ioda/ObsSpace/testInvalidNumeric")
      { testInvalidNumeric(); });
    ts.emplace_back(CASE("ioda/ObsSpace/testCleanup")
      { testCleanup(); });
  }

  void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda

#endif  // TEST_IODA_OBSSPACEINVALIDNUMERIC_H_
