/*
 * (C) Copyright 2017 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#ifndef IODA_OBSSPACE_F_H_
#define IODA_OBSSPACE_F_H_

#include "ObsSpace.h"
#include "oops/util/DateTime.h"

// -----------------------------------------------------------------------------
// These functions provide a Fortran-callable interface to ObsSpace.
// -----------------------------------------------------------------------------

namespace ioda {

extern "C" {
  int obsspace_get_nobs_f(const ObsSpace &);
  int obsspace_get_nlocs_f(const ObsSpace &);
  double obspace_missing_value_f();

  void obsspace_get_refdate_f(const ObsSpace &, util::DateTime &);

  void obsspace_get_int32_f(const ObsSpace &, const char *, const char *,
                            const std::size_t &, int32_t*);
  void obsspace_get_int64_f(const ObsSpace &, const char *, const char *,
                            const std::size_t &, int64_t*);
  void obsspace_get_real32_f(const ObsSpace &, const char *, const char *,
                             const std::size_t &, float*);
  void obsspace_get_real64_f(const ObsSpace &, const char *, const char *,
                             const std::size_t &, double*);

  void obsspace_put_int32_f(ObsSpace &, const char *, const char *,
                            const std::size_t &, int32_t*);
  void obsspace_put_int64_f(ObsSpace &, const char *, const char *,
                            const std::size_t &, int64_t*);
  void obsspace_put_real32_f(ObsSpace &, const char *, const char *,
                             const std::size_t &, float*);
  void obsspace_put_real64_f(ObsSpace &, const char *, const char *,
                             const std::size_t &, double*);
}

}  // namespace ioda

#endif  // IODA_OBSSPACE_F_H_
