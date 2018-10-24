/*
 * (C) Copyright 2017 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#include "ioda/Locations.h"

#include <memory>
#include <random>
#include <vector>

#include "oops/util/Logger.h"

#include "ioda/Fortran.h"

namespace ioda {

// -----------------------------------------------------------------------------
/*! UFO Locations Constructor with Configuration
 * 
 * \details This constructor can be used to generate user-specified
 * and/or random locations for use with interpolation or other tests
 *
 * To generate random locations, the relevant parameters specified in 
 * **StateTest.Locations** section of the config file are:
 * 
 * * **lats** user-specified latitudes (degrees)
 * * **lons** user-specified longitudes (degrees)
 * * **Nrandom** number of random locations desired
 * * **random_seed** (optional) random seed for reproducibility of results
 * 
 * \date May, 2018 Created (M. Miesch, JCSDA)
 *
 * \sa ioda::ioda_locs_create() ioda::ioda_loc_test() test::testStateInterpolation() 
 *
 */

Locations::Locations(const eckit::Configuration & conf) {
  std::vector<double> lats = conf.getDoubleVector("lats");
  std::vector<double> lons = conf.getDoubleVector("lons");

  ASSERT(lats.size() == lons.size());
  int nloc = lats.size();

  int rdist = 0;

  if (conf.has("Nrandom")) {
    int Nrandom = conf.getInt("Nrandom");

    std::unique_ptr<std::mt19937> generator;

    if (conf.has("random_seed")) {
      int rseed = conf.getInt("random_seed");
      generator.reset(new std::mt19937(rseed));
    } else {
      generator.reset(new std::mt19937(time(0)));
    }

    static std::uniform_real_distribution<double> distribution(-90, 90);

    // random latitudes range from -90 to 90 degrees
    // random longitudes range from 0 to 360 degrees
    std::vector<double> xx(Nrandom, 0.0);
    for (size_t jj=0; jj < Nrandom; ++jj) xx[jj] = distribution(*generator);
    lats.insert(lats.end(), xx.begin(), xx.end());
    for (size_t jj=0; jj < Nrandom; ++jj) xx[jj] = 2.0*distribution(*generator) + 180.0;
    lons.insert(lons.end(), xx.begin(), xx.end());

    nloc += Nrandom;

    if (conf.has("Rdist")) {
      rdist = conf.getInt("Rdist");
    }
  }

  ioda_locs_create_f90(keyLoc_, nloc, &lats[0], &lons[0], rdist);
}

// -----------------------------------------------------------------------------

Locations::~Locations() {
  ioda_locs_delete_f90(keyLoc_);
}

// -----------------------------------------------------------------------------

int Locations::nobs() const {
  int nobs;
  ioda_locs_nobs_f90(keyLoc_, nobs);
  return nobs;
}

// -----------------------------------------------------------------------------

void Locations::print(std::ostream & os) const {
  int nobs;
  ioda_locs_nobs_f90(keyLoc_, nobs);
  os << "Locations: " << nobs << " locations: ";

  // Write lat and lon to debug stream
  double lat, lon;

  for (int i=0; i < nobs; ++i) {
    ioda_locs_coords_f90(keyLoc_, i, lat, lon);
    oops::Log::debug() << "obs " << i << ": " << std::setprecision(2) << std::fixed
                       << "lat = " << lat << ", lon = " << lon << std::endl;
  }
}

// -----------------------------------------------------------------------------

}  // namespace ioda

