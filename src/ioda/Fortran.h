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
//  Observation Handler
// -----------------------------------------------------------------------------
  void ioda_obsdb_setup_f90(F90odb &, const eckit::Configuration * const *,
                            const util::DateTime * const *, const util::DateTime * const *,
                            const double &);
  void ioda_obsdb_delete_f90(F90odb &);
  void ioda_obsdb_nobs_f90(const F90odb &, int &);
  void ioda_obsdb_nlocs_f90(const F90odb &, int &);
  void ioda_obsdb_getrefdate_f90(const F90odb &, util::DateTime &);
  void ioda_obsdb_generate_f90(const F90odb &, const eckit::Configuration * const *,
                               const util::DateTime * const *, const util::DateTime * const *,
                               const double &);
  void ioda_obsdb_geti_f90(const F90odb &, const int &, const char *, const int &, int32_t[]);
  void ioda_obsdb_getd_f90(const F90odb &, const int &, const char *, const int &, double[]);
  void ioda_obsdb_puti_f90(const F90odb &, const int &, const char *, const int &, const int32_t[]);
  void ioda_obsdb_putd_f90(const F90odb &, const int &, const char *, const int &, const double[]);
  void ioda_obsdb_has_f90(const F90odb &, const int &, const char *, int &);

// -----------------------------------------------------------------------------

}  // extern C

}  // namespace ioda
#endif  // IODA_FORTRAN_H_
