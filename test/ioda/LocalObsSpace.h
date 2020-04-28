/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef TEST_INTERFACE_LOCALOBSSPACE_H_
#define TEST_INTERFACE_LOCALOBSSPACE_H_

#include <string>

#define ECKIT_TESTING_SELF_REGISTER_CASES 0

#include <boost/shared_ptr.hpp>

#include "../test/ioda/ObsSpace.h"
#include "eckit/config/LocalConfiguration.h"
#include "eckit/testing/Test.h"
#include "ioda/IodaTrait.h"
#include "ioda/ObsSpace.h"
#include "oops/parallel/mpi/mpi.h"
#include "oops/runs/Test.h"
#include "oops/../test/TestEnvironment.h"

namespace ioda {
namespace test {

// -----------------------------------------------------------------------------

void testConstructor_local() {
  typedef ObsSpaceTestFixture Test_;

  const eckit::LocalConfiguration obsconf(::test::TestEnvironment::config(), "Observations");
  std::vector<eckit::LocalConfiguration> conf;
  obsconf.get("ObsTypes", conf);

  for (std::size_t jj = 0; jj < Test_::size(); ++jj) {
    double lonpt = conf[jj].getDouble("ObsSpace.Localization.lonRefPoint");
    double latpt = conf[jj].getDouble("ObsSpace.Localization.latRefPoint");
    eckit::geometry::Point2 refPoint(lonpt, latpt);
    // Create local obsspace object
    const eckit::LocalConfiguration locconf(conf[jj], "ObsSpace.Localization");
    ioda::ObsSpace obsspace_local(Test_::obspace(jj), refPoint, locconf);
    // Get the numbers of locations (nlocs) from the local obspace object
    std::size_t Nlocs = obsspace_local.nlocs();
    oops::Log::debug() << "Nlocs_local = " << Nlocs << std::endl;
    obsspace_local.comm().allReduceInPlace(Nlocs, eckit::mpi::sum());
    // Get the expected nlocs from the obspace object's configuration
    std::size_t ExpectedNlocs = conf[jj].getUnsigned("ObsSpace.TestData.nlocs_local");
    oops::Log::debug() << "Expected Nlocs_local = " << ExpectedNlocs << std::endl;
    EXPECT(Nlocs == ExpectedNlocs);
    //test localization distances
    std::vector<double> obsdist = obsspace_local.obsdist();
    double distance = conf[jj].getDouble("ObsSpace.Localization.lengthscale");
    oops::Log::debug() << "loc_ dist" << distance << std::endl;
    if (!obsdist.empty()) {
      oops::Log::debug() << "loc_obs_dist(min,max) = " << 
                          *std::min_element(obsdist.begin(), obsdist.end()) << " "<<
                          *std::max_element(obsdist.begin(), obsdist.end()) << std::endl;
    EXPECT(*std::max_element(obsdist.begin(), obsdist.end()) <= distance); 
    EXPECT(*std::min_element(obsdist.begin(), obsdist.end()) > 0.0);
    }
  }
}

// -----------------------------------------------------------------------------

class LocalObsSpace : public oops::Test {
 public:
  LocalObsSpace() {}
  virtual ~LocalObsSpace() {}
 private:
  std::string testid() const {return "test::LocalObsSpace<ioda::IodaTrait>";}

  void register_tests() const {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    ts.emplace_back(CASE("ioda/LocalObsSpace/testConstructor_local")
      { testConstructor_local(); });
  }
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda

#endif  // TEST_INTERFACE_LOCALOBSSPACE_H_
