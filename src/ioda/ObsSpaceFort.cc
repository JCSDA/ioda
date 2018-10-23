/*
 * (C) Copyright 2017 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#include "ObsSpaceFort.h"

#include <map>
#include <string>

#include "eckit/config/Configuration.h"

#include "oops/util/abor1_cpp.h"
#include "oops/util/Logger.h"

#include "Locations.h"
#include "ObsVector.h"

namespace ioda {
// -----------------------------------------------------------------------------
// Maker for the ObsSpaceFactory
static ObsSpaceMaker<ObsSpaceFort> makerObsSpaceFort_("ObsSpaceFort");


// -----------------------------------------------------------------------------
ObsSpaceFort::ObsSpaceFort(const eckit::Configuration & config,
                   const util::DateTime & bgn, const util::DateTime & end)
  : ObsSpaceBase(config, bgn, end), winbgn_(bgn), winend_(end)
{
  oops::Log::trace() << "ioda::ObsSpaceFort config  = " << config << std::endl;

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

  oops::Log::trace() << "ioda::ObsSpaceFort contructed name = " << obsname_ << std::endl;
}

// -----------------------------------------------------------------------------

ObsSpaceFort::~ObsSpaceFort() {
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

void ObsSpaceFort::getdb(const std::string & col, int & keyData) const {
  if (obsname_ == "StericHeight")
    ioda_obsdb_stericheight_get_f90(keyOspace_, col.size(), col.c_str(), keyData);
  else if (obsname_ == "SeaIceFraction")
    ioda_obsdb_seaice_get_f90(keyOspace_, col.size(), col.c_str(), keyData);
  else if (obsname_ == "SeaIceThickness")
    ioda_obsdb_seaicethick_get_f90(keyOspace_, col.size(), col.c_str(), keyData);  
  else if (obsname_ == "InsituTemperature")
    ioda_obsdb_insitutemperature_get_f90(keyOspace_, col.size(), col.c_str(), keyData);
  else if (obsname_ == "SeaSurfaceTemp")
    ioda_obsdb_seasurfacetemp_get_f90(keyOspace_, col.size(), col.c_str(), keyData);  
  else if (obsname_ == "ADT")
    ioda_obsdb_adt_get_f90(keyOspace_, col.size(), col.c_str(), keyData);  
  else
    ioda_obsdb_get_f90(keyOspace_, col.size(), col.c_str(), keyData);
}

// -----------------------------------------------------------------------------

void ObsSpaceFort::getvar(const std::string & Vname, double Vdata[],
                             const int Vsize) const {
  if (obsname_ == "StericHeight")
    ioda_obsdb_stericheight_getvar_f90(keyOspace_, Vname.size(), Vname.c_str(), Vdata, Vsize);
  else if (obsname_ == "SeaIceFraction")
    ioda_obsdb_seaice_getvar_f90(keyOspace_, Vname.size(), Vname.c_str(), Vdata, Vsize);
  else if (obsname_ == "SeaIceThickness")
    ioda_obsdb_seaicethick_getvar_f90(keyOspace_, Vname.size(), Vname.c_str(), Vdata, Vsize);
  else if (obsname_ == "InsituTemperature")
    ioda_obsdb_insitutemperature_getvar_f90(keyOspace_, Vname.size(), Vname.c_str(), Vdata, Vsize);
  else if (obsname_ == "SeaSurfaceTemp")
    ioda_obsdb_seasurfacetemp_getvar_f90(keyOspace_, Vname.size(), Vname.c_str(), Vdata, Vsize);
  else if (obsname_ == "ADT")
    ioda_obsdb_adt_getvar_f90(keyOspace_, Vname.size(), Vname.c_str(), Vdata, Vsize);  
  else
    ioda_obsdb_getvar_f90(keyOspace_, Vname.size(), Vname.c_str(), Vdata, Vsize);
}

// -----------------------------------------------------------------------------

void ObsSpaceFort::putdb(const std::string & col, const int & keyData) const {
  ioda_obsdb_put_f90(keyOspace_, col.size(), col.c_str(), keyData);
  oops::Log::trace() << "In putdb obsname = " << std::endl;
}

// -----------------------------------------------------------------------------

Locations * ObsSpaceFort::locations(const util::DateTime & t1, const util::DateTime & t2) const {
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

int ObsSpaceFort::nobs() const {
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

int ObsSpaceFort::nlocs() const {
  int n;

  ioda_obsdb_nlocs_f90(keyOspace_, n);

  return n;
}

// -----------------------------------------------------------------------------

void ObsSpaceFort::generateDistribution(const eckit::Configuration & conf) {
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

void ObsSpaceFort::print(std::ostream & os) const {
  os << "ObsSpaceFort::print not implemented";
}

// -----------------------------------------------------------------------------

void ObsSpaceFort::printJo(const ObsVector & dy, const ObsVector & grad) {
  oops::Log::info() << "ObsSpaceFort::printJo not implemented" << std::endl;
}

// -----------------------------------------------------------------------------

}  // namespace ioda
