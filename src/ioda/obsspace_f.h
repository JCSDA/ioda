/*
 * (C) Copyright 2017 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#ifndef IODA_OBSSPACE_F_H_
#define IODA_OBSSPACE_F_H_

#include "ObsSpaceView.h"
#include "oops/util/DateTime.h"

// -----------------------------------------------------------------------------
// These functions provide a Fortran-callable interface to ObsSpaceView.
// -----------------------------------------------------------------------------

namespace ioda {

extern "C" {
  const ObsSpaceView * obsspace_construct_f(const eckit::Configuration *, const util::DateTime *,
                                        const util::DateTime *);
  void obsspace_destruct_f(ObsSpaceView *);
  int obsspace_get_gnlocs_f(const ObsSpaceView &);
  int obsspace_get_nlocs_f(const ObsSpaceView &);
  int obsspace_get_nrecs_f(const ObsSpaceView &);
  int obsspace_get_nvars_f(const ObsSpaceView &);
  void obsspace_get_recnum_f(const ObsSpaceView &, const std::size_t &, std::size_t *);
  void obsspace_get_index_f(const ObsSpaceView &, const std::size_t &, std::size_t *);
  bool obsspace_has_f(const ObsSpaceView &, const char *, const char *);

  void obsspace_get_int32_f(const ObsSpaceView &, const char *, const char *,
                            const std::size_t &, int32_t*);
  void obsspace_get_int64_f(const ObsSpaceView &, const char *, const char *,
                            const std::size_t &, int64_t*);
  void obsspace_get_real32_f(const ObsSpaceView &, const char *, const char *,
                             const std::size_t &, float*);
  void obsspace_get_real64_f(const ObsSpaceView &, const char *, const char *,
                             const std::size_t &, double*);
  void obsspace_get_datetime_f(const ObsSpaceView &, const char *, const char *,
                               const std::size_t &, int32_t*, int32_t*);

  void obsspace_put_int32_f(ObsSpaceView &, const char *, const char *,
                            const std::size_t &, int32_t*);
  void obsspace_put_int64_f(ObsSpaceView &, const char *, const char *,
                            const std::size_t &, int64_t*);
  void obsspace_put_real32_f(ObsSpaceView &, const char *, const char *,
                             const std::size_t &, float*);
  void obsspace_put_real64_f(ObsSpaceView &, const char *, const char *,
                             const std::size_t &, double*);
}

}  // namespace ioda

#endif  // IODA_OBSSPACE_F_H_
