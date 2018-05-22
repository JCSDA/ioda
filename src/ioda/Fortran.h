/*
 * (C) Copyright 2017 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef IODA_FORTRAN_H_
#define IODA_FORTRAN_H_

// Forward declarations
namespace eckit {
  class Configuration;
}

namespace util {
  class DateTime;
  class Duration;
}

namespace ioda {

// Locations key type
typedef int F90locs;
// Goms key type
typedef int F90goms;
// Observation vector key type
typedef int F90ovec;
// Obs operator key type
typedef int F90hop;
// Observation space type
typedef int F90odb;
// Observation check key type
typedef int F90ocheck;
// Observation bias key type
typedef int F90obias;

/// Interface to Fortran IODA routines
/*!
 * The core of the IODA is coded in Fortran.
 * Here we define the interfaces to the Fortran code.
 */

extern "C" {

// -----------------------------------------------------------------------------
//  Locations
// -----------------------------------------------------------------------------
  void ioda_locs_create_f90(F90locs &, const int  &, const double *, const double *);
  void ioda_locs_delete_f90(F90locs &);
  void ioda_locs_nobs_f90(const F90locs &, int &);

// -----------------------------------------------------------------------------
//  Observation Handler (for radiosondes)
// -----------------------------------------------------------------------------

  void ioda_obsdb_radiosonde_setup_f90(F90odb &, const eckit::Configuration * const *);
  void ioda_obsdb_radiosonde_delete_f90(F90odb &);
  void ioda_obsdb_radiosonde_getlocations_f90(const F90odb &,const util::DateTime * const *,
                                             const util::DateTime * const *,F90locs &);
  void ioda_obsdb_radiosonde_generate_f90(const F90odb &, const eckit::Configuration * const *,
                                         const util::DateTime * const *,const util::DateTime * const *);
  void ioda_obsdb_radiosonde_nobs_f90(const F90odb &, int &);
  void ioda_obsdb_radiosonde_get_f90(const F90odb &, const int &, const char *, const F90ovec &);

// -----------------------------------------------------------------------------
//  Observation Handler (for radiances)
// -----------------------------------------------------------------------------

  void ioda_obsdb_radiance_setup_f90(F90odb &, const eckit::Configuration * const *);
  void ioda_obsdb_radiance_delete_f90(F90odb &);
  void ioda_obsdb_radiance_getlocations_f90(const F90odb &,const util::DateTime * const *,
                                           const util::DateTime * const *,F90locs &);
  void ioda_obsdb_radiance_generate_f90(const F90odb &, const eckit::Configuration * const *,
                                       const util::DateTime * const *,const util::DateTime * const *);
  void ioda_obsdb_radiance_nobs_f90(const F90odb &, int &);
  void ioda_obsdb_radiance_get_f90(const F90odb &, const int &, const char *, const F90ovec &);

// -----------------------------------------------------------------------------
//  Observation Handler (for sea ice)
// -----------------------------------------------------------------------------

  void ioda_obsdb_seaice_setup_f90(F90odb &, const eckit::Configuration * const *);
  void ioda_obsdb_seaice_delete_f90(F90odb &);
  void ioda_obsdb_seaice_getlocations_f90(const F90odb &,
                                  const util::DateTime * const *,
                                  const util::DateTime * const *,
                                  F90locs &);
  void ioda_obsdb_seaice_generate_f90(const F90odb &, const eckit::Configuration * const *,
                              const util::DateTime * const *,
                              const util::DateTime * const *);
  void ioda_obsdb_seaice_nobs_f90(const F90odb &, int &);
  void ioda_obsdb_seaice_get_f90(const F90odb &, const int &, const char *, const F90ovec &);

// -----------------------------------------------------------------------------
//  Observation Handler (for sea ice thickness)
// -----------------------------------------------------------------------------

  void ioda_obsdb_seaicethick_setup_f90(F90odb &, const eckit::Configuration * const *);
  void ioda_obsdb_seaicethick_delete_f90(F90odb &);
  void ioda_obsdb_seaicethick_getlocations_f90(const F90odb &,
                                  const util::DateTime * const *,
                                  const util::DateTime * const *,
                                  F90locs &);
  void ioda_obsdb_seaicethick_generate_f90(const F90odb &, const eckit::Configuration * const *,
                              const util::DateTime * const *,
                              const util::DateTime * const *);
  void ioda_obsdb_seaicethick_nobs_f90(const F90odb &, int &);
  void ioda_obsdb_seaicethick_get_f90(const F90odb &, const int &, const char *, const F90ovec &);  


// -----------------------------------------------------------------------------
//  Observation Handler (for steric height)
// -----------------------------------------------------------------------------

  void ioda_obsdb_stericheight_setup_f90(F90odb &, const eckit::Configuration * const *);
  void ioda_obsdb_stericheight_delete_f90(F90odb &);
  void ioda_obsdb_stericheight_getlocations_f90(const F90odb &,
                                  const util::DateTime * const *,
                                  const util::DateTime * const *,
                                  F90locs &);
  void ioda_obsdb_stericheight_generate_f90(const F90odb &, const eckit::Configuration * const *,
                              const util::DateTime * const *,
                              const util::DateTime * const *);
  void ioda_obsdb_stericheight_nobs_f90(const F90odb &, int &);
  void ioda_obsdb_stericheight_get_f90(const F90odb &, const int &, const char *, const F90ovec &);

// -----------------------------------------------------------------------------  
//  Observation Handler (for temperature profiles)
// -----------------------------------------------------------------------------

  void ioda_obsdb_insitutemperature_setup_f90(F90odb &, const eckit::Configuration * const *);
  void ioda_obsdb_insitutemperature_delete_f90(F90odb &);
  void ioda_obsdb_insitutemperature_getlocations_f90(const F90odb &,
                                  const util::DateTime * const *,
                                  const util::DateTime * const *,
                                  F90locs &);
  void ioda_obsdb_insitutemperature_generate_f90(const F90odb &, const eckit::Configuration * const *,
                              const util::DateTime * const *,
                              const util::DateTime * const *);
  void ioda_obsdb_insitutemperature_nobs_f90(const F90odb &, int &);
  void ioda_obsdb_insitutemperature_get_f90(const F90odb &, const int &, const char *, const F90ovec &);

// -----------------------------------------------------------------------------
//  Observation Handler (for AOD)
// -----------------------------------------------------------------------------

  void ioda_obsdb_aod_setup_f90(F90odb &, const eckit::Configuration * const *);
  void ioda_obsdb_aod_delete_f90(F90odb &);
  void ioda_obsdb_aod_getlocations_f90(const F90odb &,
                                  const util::DateTime * const *,
                                  const util::DateTime * const *,
                                  F90locs &);
  void ioda_obsdb_aod_generate_f90(const F90odb &, const eckit::Configuration * const *,
                              const util::DateTime * const *,
                              const util::DateTime * const *);
  void ioda_obsdb_aod_nobs_f90(const F90odb &, int &);
  void ioda_obsdb_aod_get_f90(const F90odb &, const int &, const char *, const F90ovec &);

// -----------------------------------------------------------------------------
//  Observation Vectors
// -----------------------------------------------------------------------------
  void ioda_obsvec_setup_f90(F90ovec &, const F90odb &);
  void ioda_obsvec_clone_f90(const F90ovec &, F90ovec &);
  void ioda_obsvec_delete_f90(F90ovec &);

  void ioda_obsvec_assign_f90(const F90ovec &, const F90ovec &);
  void ioda_obsvec_zero_f90(const F90ovec &);
  void ioda_obsvec_mul_scal_f90(const F90ovec &, const double &);
  void ioda_obsvec_add_f90(const F90ovec &, const F90ovec &);
  void ioda_obsvec_sub_f90(const F90ovec &, const F90ovec &);
  void ioda_obsvec_mul_f90(const F90ovec &, const F90ovec &);
  void ioda_obsvec_div_f90(const F90ovec &, const F90ovec &);
  void ioda_obsvec_axpy_f90(const F90ovec &, const double &, const F90ovec &);
  void ioda_obsvec_invert_f90(const F90ovec &);
  void ioda_obsvec_random_f90(const F90ovec &);
  void ioda_obsvec_dotprod_f90(const F90ovec &, const F90ovec &, double &);
  void ioda_obsvec_minmaxavg_f90(const F90ovec &, double &, double &, double &);
  void ioda_obsvec_nobs_f90(const F90ovec &, int &);

// -----------------------------------------------------------------------------  

}  // extern C

}  // namespace ioda
#endif  // IODA_FORTRAN_H_
