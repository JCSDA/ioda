/*
 * (C) Copyright 2017 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#include "Locations.h"
#include "Fortran.h"

namespace ioda {

// -----------------------------------------------------------------------------

Locations::Locations(const eckit::Configuration & conf) {
  std::vector<double> lats = conf.getDoubleVector("lats");
  std::vector<double> lons = conf.getDoubleVector("lons");
  ASSERT(lats.size() == lons.size());
  const int nloc = lats.size();
  ioda_locs_create_f90(keyLoc_, nloc, &lats[0], &lons[0]);
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
  os << "Locations: " << nobs << " locations";
}

// -----------------------------------------------------------------------------

}  // namespace UFO

