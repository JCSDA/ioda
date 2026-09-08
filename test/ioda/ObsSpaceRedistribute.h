/*
 * (C) Copyright 2026- UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#pragma once

#include <memory>
#include <string>
#include <utility>
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
#include "ioda/ObsSpaceParameters.h"

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
    const util::TimeWindow timeWindow
      (::test::TestEnvironment::config().getSubConfiguration("time window"));
    std::vector<eckit::LocalConfiguration> conf;
    ::test::TestEnvironment::config().get("observations", conf);

    for (std::size_t jj = 0; jj < conf.size(); ++jj) {
      eckit::LocalConfiguration obsconf(conf[jj], "obs space");
      ioda::ObsTopLevelParameters obsparams;
      obsparams.validateAndDeserialize(obsconf);
      boost::shared_ptr<ioda::ObsSpace> tmp(new ioda::ObsSpace(obsconf, oops::mpi::world(),
                                                               timeWindow, oops::mpi::myself()));
      ospaces_.push_back(tmp);
    }
  }

  ~ObsSpaceTestFixture() {}

  std::vector<boost::shared_ptr<ioda::ObsSpace> > ospaces_;
};

// -----------------------------------------------------------------------------

void testRedistribute() {
  typedef ObsSpaceTestFixture Test_;

  std::vector<eckit::LocalConfiguration> conf;
  ::test::TestEnvironment::config().get("observations", conf);

  for (std::size_t jj = 0; jj < Test_::size(); ++jj) {
    eckit::LocalConfiguration config(conf[jj], "redistribution");
    const bool expectException = conf[jj].getBool("expectException", false);
    ioda::ObsSpace & odb = Test_::obspace(jj);
    oops::Log::info() << "Before redistribution: " << odb.distribution()->name() << std::endl;
    std::unique_ptr<ioda::ObsVector> obsvec1 = std::make_unique<ioda::ObsVector>(odb, "ObsValue");
    oops::Log::info() << "ObsVector before redistribution: " << *obsvec1 << std::endl;
    // Save reference rms and global number of obs before redistribution
    const double ref_rms = obsvec1->rms();
    const size_t ref_nobs = obsvec1->nobs();
    // Check that redistribution throws an exception if the redistribution can be done,
    // but there are existing ObsVectors attached to the ObsSpace.
    if (!expectException) {
      EXPECT_THROWS_AS(odb.redistribute(config), eckit::UnexpectedState);
    }
    // Reset the pointer to the vector initialized before redistribution.
    obsvec1.reset();
    // Check if exception is expected for this configuration.
    if (expectException) {
      EXPECT_THROWS_AS(odb.redistribute(config), eckit::NotImplemented);
    } else {
      // Redistribute the obs space and check the distribution and data.
      odb.redistribute(config);
      oops::Log::info() << "After redistribution: " << odb.distribution()->name() << std::endl;
      ObsVector obsvec2(odb, "ObsValue");
      oops::Log::info() << "ObsVector after redistribution: " << obsvec2 << std::endl;
      EXPECT_EQUAL(obsvec2.rms(), ref_rms);
      EXPECT_EQUAL(obsvec2.nobs(), ref_nobs);
    }
  }
}

// -----------------------------------------------------------------------------

class ObsSpaceRedistribute : public oops::Test {
 public:
  ObsSpaceRedistribute() {}
  virtual ~ObsSpaceRedistribute() {}
 private:
  std::string testid() const override {return "test::ObsSpaceRedistribute<ioda::IodaTrait>";}

  void register_tests() const override {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    ts.emplace_back(CASE("ioda/ObsSpaceRedistribute/testRedistribute")
      { testRedistribute(); });
  }

  void clear() const override {
    typedef ObsSpaceTestFixture Test_;
    Test_::cleanup();
  }
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda
