/*
 * (C) Copyright 2017 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef CORE_OBSSPACE_F_H_
#define CORE_OBSSPACE_F_H_

#include "ioda/ObsSpace.h"
#include "oops/base/Variables.h"
#include "oops/mpi/mpi.h"
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
  std::size_t obsspace_get_gnlocs_f(const ObsSpace &);
  std::size_t obsspace_get_nlocs_f(const ObsSpace &);
  std::size_t obsspace_get_nchans_f(const ObsSpace &);
  std::size_t obsspace_get_nrecs_f(const ObsSpace &);
  std::size_t obsspace_get_nvars_f(const ObsSpace &);

  void obsspace_get_dim_name_f(const ObsSpace &, const int &, std::size_t &, char *);
  std::size_t obsspace_get_dim_size_f(const ObsSpace &, const int &);
  int obsspace_get_dim_id_f(const ObsSpace &, const char *);

  void obsspace_get_comm_f(const ObsSpace &, int &, char *);
  void obsspace_get_recnum_f(const ObsSpace &, const std::size_t &, std::size_t *);
  void obsspace_get_index_f(const ObsSpace &, const std::size_t &, std::size_t *);

  void obsspace_obsname_f(const ObsSpace &, size_t &, char *);

  bool obsspace_has_f(const ObsSpace &, const char *, const char *);

  void obsspace_get_int32_f(const ObsSpace &, const char *, const char *,
                            const std::size_t &, int32_t*,
                            const std::size_t &, int*);
  void obsspace_get_int64_f(const ObsSpace &, const char *, const char *,
                            const std::size_t &, int64_t*,
                            const std::size_t &, int*);
  void obsspace_get_real32_f(const ObsSpace &, const char *, const char *,
                             const std::size_t &, float*,
                             const std::size_t &, int*);
  void obsspace_get_real64_f(const ObsSpace &, const char *, const char *,
                             const std::size_t &, double*,
                             const std::size_t &, int*);
  void obsspace_get_datetime_f(const ObsSpace &, const char *, const char *,
                               const std::size_t &, int32_t*, int32_t*,
                               const std::size_t &, int*);
  void obsspace_get_bool_f(const ObsSpace &, const char *, const char *,
                           const std::size_t &, bool*,
                           const std::size_t &, int*);

  void obsspace_put_int32_f(ObsSpace &, const char *, const char *,
                            const std::size_t &, int32_t*,
                            const std::size_t &, int*);
  void obsspace_put_int64_f(ObsSpace &, const char *, const char *,
                            const std::size_t &, int64_t*,
                            const std::size_t &, int*);
  void obsspace_put_real32_f(ObsSpace &, const char *, const char *,
                             const std::size_t &, float*,
                             const std::size_t &, int*);
  void obsspace_put_real64_f(ObsSpace &, const char *, const char *,
                             const std::size_t &, double*,
                             const std::size_t &, int*);
  void obsspace_put_bool_f(ObsSpace &, const char *, const char *,
                           const std::size_t &, bool*,
                           const std::size_t &, int*);

  int obsspace_get_nlocs_dim_id_f();
  int obsspace_get_nchans_dim_id_f();
}

}  // namespace ioda

#endif  // CORE_OBSSPACE_F_H_
