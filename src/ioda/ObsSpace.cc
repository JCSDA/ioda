/*
 * (C) Copyright 2017 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ObsSpace.h"

#include <map>
#include <string>

#include "eckit/config/Configuration.h"

#include "oops/parallel/mpi/mpi.h"
#include "oops/util/abor1_cpp.h"
#include "oops/util/Logger.h"

#include "Locations.h"

namespace ioda {
// -----------------------------------------------------------------------------

ObsSpace::ObsSpace(const eckit::Configuration & config,
                   const util::DateTime & bgn, const util::DateTime & end)
  : oops::ObsSpaceBase(config, bgn, end), winbgn_(bgn), winend_(end), commMPI_(oops::mpi::comm())
{
  oops::Log::trace() << "ioda::ObsSpace config  = " << config << std::endl;

  const eckit::Configuration * configc = &config;
  obsname_ = config.getString("ObsType");

  if (obsname_ == "StericHeight")
    ioda_obsdb_stericheight_setup_f90(keyOspace_, &configc);
  else if (obsname_ == "SeaIceFraction")
    ioda_obsdb_seaice_setup_f90(keyOspace_, &configc);
  else if (obsname_ == "SeaIceThickness")
    ioda_obsdb_seaicethick_setup_f90(keyOspace_, &configc);
  else if (obsname_ == "InsituTemperature")
    ioda_obsdb_insitutemperature_setup_f90(keyOspace_, &configc);
  else if (obsname_ == "SeaSurfaceTemp")
    ioda_obsdb_seasurfacetemp_setup_f90(keyOspace_, &configc);
  else if (obsname_ == "ADT")
    ioda_obsdb_adt_setup_f90(keyOspace_, &configc);
  else
    ioda_obsdb_setup_f90(keyOspace_, &configc);

  oops::Log::trace() << "ioda::ObsSpace contructed name = " << obsname_ << std::endl;
}

// -----------------------------------------------------------------------------

ObsSpace::~ObsSpace() {
  if (obsname_ == "StericHeight")
    ioda_obsdb_stericheight_delete_f90(keyOspace_);
  else if (obsname_ == "SeaIceFraction")
    ioda_obsdb_seaice_delete_f90(keyOspace_);
  else if (obsname_ == "SeaIceThickness")
    ioda_obsdb_seaicethick_delete_f90(keyOspace_);
  else if (obsname_ == "InsituTemperature")
    ioda_obsdb_insitutemperature_delete_f90(keyOspace_);
  else if (obsname_ == "SeaSurfaceTemp")
    ioda_obsdb_seasurfacetemp_delete_f90(keyOspace_);
  else if (obsname_ == "ADT")
    ioda_obsdb_adt_delete_f90(keyOspace_);
  else
    ioda_obsdb_delete_f90(keyOspace_);
}

// -----------------------------------------------------------------------------

void ObsSpace::getObsVector(const std::string & name, std::vector<double> & vec) const {
  if (obsname_ == "StericHeight")
    ioda_obsdb_stericheight_get_f90(keyOspace_, name.c_str(), name.size(), vec.data(), vec.size());
  else if (obsname_ == "SeaIceFraction")
    ioda_obsdb_seaice_get_f90(keyOspace_, name.c_str(), name.size(), vec.data(), vec.size());
  else if (obsname_ == "SeaIceThickness")
    ioda_obsdb_seaicethick_get_f90(keyOspace_, name.c_str(), name.size(), vec.data(), vec.size());
  else if (obsname_ == "InsituTemperature")
    ioda_obsdb_insitutemperature_get_f90(keyOspace_, name.c_str(), name.size(), vec.data(), vec.size());
  else if (obsname_ == "SeaSurfaceTemp")
    ioda_obsdb_seasurfacetemp_get_f90(keyOspace_, name.c_str(), name.size(), vec.data(), vec.size());
  else if (obsname_ == "ADT")
    ioda_obsdb_adt_get_f90(keyOspace_, name.c_str(), name.size(), vec.data(), vec.size());
  else
    ioda_obsdb_get_f90(keyOspace_, name.size(), name.c_str(), vec.size(), vec.data());
}

// -----------------------------------------------------------------------------

void ObsSpace::putObsVector(const std::string & name, const std::vector<double> & vec) const {
  ioda_obsdb_put_f90(keyOspace_, name.size(), name.c_str(), vec.size(), vec.data());
  oops::Log::trace() << "In putdb obsname = " << std::endl;
}

// -----------------------------------------------------------------------------

void ObsSpace::getvar(const std::string & name, const int vsize, double vdata[]) const {
//  if (obsname_ == "StericHeight")
//    ioda_obsdb_stericheight_getvar_f90(keyOspace_, name.size(), name.c_str(), vdata, vsize);
//  else if (obsname_ == "SeaIceFraction")
//    ioda_obsdb_seaice_getvar_f90(keyOspace_, name.size(), name.c_str(), vdata, vsize);
//  else if (obsname_ == "SeaIceThickness")
//    ioda_obsdb_seaicethick_getvar_f90(keyOspace_, name.size(), name.c_str(), vdata, vsize);
//  else if (obsname_ == "InsituTemperature")
//    ioda_obsdb_insitutemperature_getvar_f90(keyOspace_, name.size(), name.c_str(), vdata, vsize);
//  else if (obsname_ == "SeaSurfaceTemp")
//    ioda_obsdb_seasurfacetemp_getvar_f90(keyOspace_, name.size(), name.c_str(), vdata, vsize);
//  else if (obsname_ == "ADT")
//    ioda_obsdb_adt_getvar_f90(keyOspace_, name.size(), name.c_str(), vdata, vsize);  
//  else
    ioda_obsdb_getvar_f90(keyOspace_, name.size(), name.c_str(), vsize, vdata);
}

// -----------------------------------------------------------------------------

Locations * ObsSpace::locations(const util::DateTime & t1, const util::DateTime & t2) const {
  const util::DateTime * p1 = &t1;
  const util::DateTime * p2 = &t2;
  int keylocs;
  if (obsname_ == "StericHeight")
    ioda_obsdb_stericheight_getlocations_f90(keyOspace_, &p1, &p2, keylocs);
  else if (obsname_ == "SeaIceFraction")
    ioda_obsdb_seaice_getlocations_f90(keyOspace_, &p1, &p2, keylocs);
  else if (obsname_ == "SeaIceThickness")
    ioda_obsdb_seaicethick_getlocations_f90(keyOspace_, &p1, &p2, keylocs);
  else if (obsname_ == "InsituTemperature")
    ioda_obsdb_insitutemperature_getlocations_f90(keyOspace_, &p1, &p2, keylocs);
  else if (obsname_ == "SeaSurfaceTemp")
    ioda_obsdb_seasurfacetemp_getlocations_f90(keyOspace_, &p1, &p2, keylocs);
  else if (obsname_ == "ADT")
    ioda_obsdb_adt_getlocations_f90(keyOspace_, &p1, &p2, keylocs);
  else
    ioda_obsdb_getlocations_f90(keyOspace_, &p1, &p2, keylocs);

  return new Locations(keylocs);
}

// -----------------------------------------------------------------------------

int ObsSpace::nobs() const {
  int n;
  if (obsname_ == "StericHeight")
    ioda_obsdb_stericheight_nobs_f90(keyOspace_, n);
  else if (obsname_ == "SeaIceFraction")
    ioda_obsdb_seaice_nobs_f90(keyOspace_, n);
  else if (obsname_ == "SeaIceThickness")
    ioda_obsdb_seaicethick_nobs_f90(keyOspace_, n);
  else if (obsname_ == "InsituTemperature")
    ioda_obsdb_insitutemperature_nobs_f90(keyOspace_, n);
  else if (obsname_ == "SeaSurfaceTemp")
    ioda_obsdb_seasurfacetemp_nobs_f90(keyOspace_, n);
  else if (obsname_ == "ADT")
    ioda_obsdb_adt_nobs_f90(keyOspace_, n);
  else
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
  if (obsname_ == "StericHeight")
    ioda_obsdb_stericheight_generate_f90(keyOspace_, &configc, &p1, &p2);
  else if (obsname_ == "SeaIceFraction")
    ioda_obsdb_seaice_generate_f90(keyOspace_, &configc, &p1, &p2);
  else if (obsname_ == "SeaIceThickness")
    ioda_obsdb_seaicethick_generate_f90(keyOspace_, &configc, &p1, &p2);
  else if (obsname_ == "InsituTemperature")
    ioda_obsdb_insitutemperature_generate_f90(keyOspace_, &configc, &p1, &p2);
  else if (obsname_ == "SeaSurfaceTemp")
    ioda_obsdb_seasurfacetemp_generate_f90(keyOspace_, &configc, &p1, &p2);
  else if (obsname_ == "ADT")
    ioda_obsdb_adt_generate_f90(keyOspace_, &configc, &p1, &p2);
  else
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
