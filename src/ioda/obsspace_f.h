/*
 * (C) Copyright 2017 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef IODA_OBSSPACE_F_H_
#define IODA_OBSSPACE_F_H_

#include "ObsSpace.h"
#include "oops/base/Variables.h"
#include "oops/parallel/mpi/mpi.h"
#include "oops/util/DateTime.h"

// -----------------------------------------------------------------------------
// These functions provide a Fortran-callable interface to ObsSpace.
// -----------------------------------------------------------------------------

namespace ioda {

extern "C" {
  const ObsSpace * obsspace_construct_f(const eckit::Configuration *, const util::DateTime *,
                                        const util::DateTime *);
  void obsspace_destruct_f(ObsSpace *);
  const oops::Variables * obsspace_obsvariables_f(const ObsSpace &);
  int obsspace_get_gnlocs_f(const ObsSpace &);
  int obsspace_get_nlocs_f(const ObsSpace &);
  int obsspace_get_nrecs_f(const ObsSpace &);
  int obsspace_get_nvars_f(const ObsSpace &);
  void obsspace_get_comm_f(const ObsSpace &, int &, char *);
  void obsspace_get_recnum_f(const ObsSpace &, const std::size_t &, std::size_t *);
  void obsspace_get_index_f(const ObsSpace &, const std::size_t &, std::size_t *);

  void obsspace_obsname_f(const ObsSpace &, size_t &, char *);

  bool obsspace_has_f(const ObsSpace &, const char *, const char *);

  void obsspace_get_int32_f(const ObsSpace &, const char *, const char *,
                            const std::size_t &, int32_t*);
  void obsspace_get_int64_f(const ObsSpace &, const char *, const char *,
                            const std::size_t &, int64_t*);
  void obsspace_get_real32_f(const ObsSpace &, const char *, const char *,
                             const std::size_t &, float*);
  void obsspace_get_real64_f(const ObsSpace &, const char *, const char *,
                             const std::size_t &, double*);
  void obsspace_get_datetime_f(const ObsSpace &, const char *, const char *,
                               const std::size_t &, int32_t*, int32_t*);

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
