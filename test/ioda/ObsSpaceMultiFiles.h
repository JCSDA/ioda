/*
 * (C) Copyright 2024 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef TEST_IODA_OBSSPACEMULTIFILES_H_
#define TEST_IODA_OBSSPACEMULTIFILES_H_

#include <cmath>
#include <set>
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

#include "ioda/distribution/Accumulator.h"
#include "ioda/distribution/DistributionUtils.h"
#include "ioda/IodaTrait.h"
#include "ioda/ObsSpace.h"

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

void testConstructor() {
  typedef ObsSpaceTestFixture Test_;

  std::vector<eckit::LocalConfiguration> conf;
  ::test::TestEnvironment::config().get("observations", conf);

  for (std::size_t jj = 0; jj < Test_::size(); ++jj) {
    // Grab the test data configuration
    eckit::LocalConfiguration testConfig;
    conf[jj].get("test data", testConfig);

    std::size_t refNlocs = testConfig.getUnsigned("nlocs");

    const ObsSpace &odb = Test_::obspace(jj);

    std::size_t nlocs = odb.nlocs();
    EXPECT_EQUAL(nlocs, refNlocs);
  }
}

// -----------------------------------------------------------------------------

void testObsGroupAppend() {
  typedef ObsSpaceTestFixture Test_;

  eckit::LocalConfiguration timeWindowConfig;
  ::test::TestEnvironment::config().get("time window", timeWindowConfig);

  std::vector<eckit::LocalConfiguration> conf;
  ::test::TestEnvironment::config().get("observations", conf);

  for (std::size_t jj = 0; jj < Test_::size(); ++jj) {
    // Grab the test data configuration
    eckit::LocalConfiguration testConfig;
    conf[jj].get("test data", testConfig);
    std::size_t refNlocs = testConfig.getUnsigned("nlocs");

    const ObsSpace &odb = Test_::obspace(jj);

    std::size_t nlocs = odb.nlocs();
    EXPECT_EQUAL(nlocs, refNlocs);

    // Grab the obs space configuration, and build a new (clone) instance of this obs space
    eckit::LocalConfiguration obsSpaceConfig;
    conf[jj].get("obs space", obsSpaceConfig);

    const util::TimeWindow timeWindow(timeWindowConfig);
    boost::shared_ptr<ioda::ObsSpace> cloneObsSpace(
      new ioda::ObsSpace(obsSpaceConfig, oops::mpi::world(), timeWindow, oops::mpi::myself()));

    nlocs = cloneObsSpace->nlocs();
    EXPECT_EQUAL(nlocs, refNlocs);

    // Append original ObsGroup to the clone ObsGroup. The obs_group_ container will
    // get doubled, but at this point the ObsSpace::nlocs_ data member won't get
    // updated so we need to check to see if the clone's Location variable has
    // doubled in size.
    ObsGroup tempObsGroup = odb.getObsGroup();
    cloneObsSpace->getObsGroup().append(tempObsGroup);
    Variable cloneLocVar = cloneObsSpace->getObsGroup().vars.open("Location");
    nlocs = cloneLocVar.getDimensions().dimsCur[0];
    EXPECT_EQUAL(nlocs, 2 * refNlocs);
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

class ObsSpaceMultiFiles : public oops::Test {
 public:
  ObsSpaceMultiFiles() {}
  virtual ~ObsSpaceMultiFiles() {}

 private:
  std::string testid() const override {return "test::ObsSpaceMultiFiles<ioda::IodaTrait>";}

  void register_tests() const override {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    ts.emplace_back(CASE("ioda/ObsSpace/testConstructor")
      { testConstructor(); });
    ts.emplace_back(CASE("ioda/ObsSpace/testObsGroupAppend")
      { testObsGroupAppend(); });
    ts.emplace_back(CASE("ioda/ObsSpace/testCleanup")
      { testCleanup(); });
  }

  void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda

#endif  // TEST_IODA_OBSSPACEMULTIFILES_H_
