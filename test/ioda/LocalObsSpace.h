/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef TEST_IODA_LOCALOBSSPACE_H_
#define TEST_IODA_LOCALOBSSPACE_H_

#include <algorithm>
#include <string>
#include <vector>

#define ECKIT_TESTING_SELF_REGISTER_CASES 0

#include <boost/shared_ptr.hpp>

#include "eckit/config/LocalConfiguration.h"
#include "eckit/testing/Test.h"

#include "oops/mpi/mpi.h"
#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"

#include "ioda/IodaTrait.h"
#include "ioda/ObsSpace.h"
#include "ioda/test/ioda/ObsSpace.h"

namespace ioda {
namespace test {

// -----------------------------------------------------------------------------

void testConstructor_local() {
  typedef ObsSpaceTestFixture Test_;

  std::vector<eckit::LocalConfiguration> conf;
  ::test::TestEnvironment::config().get("observations", conf);


    for (std::size_t jj = 0; jj < Test_::size(); ++jj) {
      double lonpt = conf[jj].getDouble("obs space.localization.lon ref point");
      double latpt = conf[jj].getDouble("obs space.localization.lat ref point");
      eckit::geometry::Point2 refPoint(lonpt, latpt);

      // Create local obsspace object
      eckit::LocalConfiguration locconf(conf[jj], "obs space.localization");
      std::string searchMethods[2] = {"brute_force", "kd_tree"};
      // Loop through search methods
      for (const std::string &searchMethod : searchMethods) {
        locconf.set("search method", searchMethod);
        oops::Log::debug() << "Using " << searchMethod << " for search method" << std::endl;
        ioda::ObsSpace obsspace_local(Test_::obspace(jj), refPoint, locconf);
        // Get the numbers of locations (nlocs) from the local obspace object
        std::size_t Nlocs = obsspace_local.nlocs();
        oops::Log::debug() << "Nlocs_local = " << Nlocs << std::endl;
        obsspace_local.comm().allReduceInPlace(Nlocs, eckit::mpi::sum());
        // Get the expected nlocs from the obspace object's configuration
        std::size_t ExpectedNlocs =
          conf[jj].getUnsigned("obs space.test data.expected local nlocs");
        oops::Log::debug() << "Expected Nlocs_local = " << ExpectedNlocs << std::endl;
        EXPECT(Nlocs == ExpectedNlocs);
        // test localization distances
        std::vector<double> obsdist = obsspace_local.obsdist();
        double distance = conf[jj].getDouble("obs space.localization.lengthscale");
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

#endif  // TEST_IODA_LOCALOBSSPACE_H_
