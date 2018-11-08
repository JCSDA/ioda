/*
 * (C) Copyright 2017 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/ObsSpace.h"

#include <map>
#include <string>
#include <vector>

#include "eckit/config/Configuration.h"

#include "oops/parallel/mpi/mpi.h"
#include "oops/util/abor1_cpp.h"
#include "oops/util/Logger.h"

#include "Locations.h"

namespace ioda {
// -----------------------------------------------------------------------------
const double ObsSpace::missingvalue_ = -9.9999e+299;
// -----------------------------------------------------------------------------

ObsSpace::ObsSpace(const eckit::Configuration & config,
                   const util::DateTime & bgn, const util::DateTime & end)
  : oops::ObsSpaceBase(config, bgn, end), winbgn_(bgn), winend_(end), commMPI_(oops::mpi::comm())
{
  oops::Log::trace() << "ioda::ObsSpace config  = " << config << std::endl;

  const eckit::Configuration * configc = &config;
  obsname_ = config.getString("ObsType");
  ioda_obsdb_setup_f90(keyOspace_, &configc);

  oops::Log::trace() << "ioda::ObsSpace contructed name = " << obsname_ << std::endl;
}

// -----------------------------------------------------------------------------

ObsSpace::~ObsSpace() {
  ioda_obsdb_delete_f90(keyOspace_);
}

// -----------------------------------------------------------------------------

void ObsSpace::getObsVector(const std::string & name, std::vector<double> & vec) const {
  ioda_obsdb_getd_f90(keyOspace_, name.size(), name.c_str(), vec.size(), vec.data());
}

// -----------------------------------------------------------------------------

void ObsSpace::putObsVector(const std::string & name, const std::vector<double> & vec) const {
  ioda_obsdb_putd_f90(keyOspace_, name.size(), name.c_str(), vec.size(), vec.data());
}

// -----------------------------------------------------------------------------
void ObsSpace::get_db(const std::string & group, const std::string & name,
                      const std::size_t & vsize, int vdata[]) const {
  ioda_obsdb_geti_f90(keyOspace_, name.size(), name.c_str(), static_cast<int>(vsize), vdata);
}
// -----------------------------------------------------------------------------
void ObsSpace::get_db(const std::string & group, const std::string & name,
                      const std::size_t & vsize, double vdata[]) const {
  ioda_obsdb_getd_f90(keyOspace_, name.size(), name.c_str(), static_cast<int>(vsize), vdata);
}
// -----------------------------------------------------------------------------
void ObsSpace::put_db(const std::string & group, const std::string & name,
                      const std::size_t & vsize, const int vdata[]) const {
  ioda_obsdb_puti_f90(keyOspace_, name.size(), name.c_str(), static_cast<int>(vsize), vdata);
}
// -----------------------------------------------------------------------------
void ObsSpace::put_db(const std::string & group, const std::string & name,
                      const std::size_t & vsize, const double vdata[]) const {
  ioda_obsdb_putd_f90(keyOspace_, name.size(), name.c_str(), static_cast<int>(vsize), vdata);
}
// -----------------------------------------------------------------------------

Locations * ObsSpace::getLocations(const util::DateTime & t1, const util::DateTime & t2) const {
  const util::DateTime * p1 = &t1;
  const util::DateTime * p2 = &t2;
  int keylocs;
  ioda_obsdb_getlocations_f90(keyOspace_, &p1, &p2, keylocs);

  return new Locations(keylocs);
}

// -----------------------------------------------------------------------------

int ObsSpace::nobs() const {
  int n;
  ioda_obsdb_nobs_f90(keyOspace_, n);

  return n;
}

// -----------------------------------------------------------------------------

int ObsSpace::nlocs() const {
  int n;
  ioda_obsdb_nlocs_f90(keyOspace_, n);
  return n;
}

// -----------------------------------------------------------------------------

void ObsSpace::generateDistribution(const eckit::Configuration & conf) {
  const eckit::Configuration * configc = &conf;

  const util::DateTime * p1 = &winbgn_;
  const util::DateTime * p2 = &winend_;
  ioda_obsdb_generate_f90(keyOspace_, &configc, &p1, &p2);
}

// -----------------------------------------------------------------------------

void ObsSpace::print(std::ostream & os) const {
  os << "ObsSpace::print not implemented";
}

// -----------------------------------------------------------------------------

void ObsSpace::printJo(const ObsVector & dy, const ObsVector & grad) {
  oops::Log::info() << "ObsSpace::printJo not implemented" << std::endl;
}

// -----------------------------------------------------------------------------

}  // namespace ioda
