/*
 * (C) Copyright 2017 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#include "ioda/obsspace_f.h"

#include <string>

#include "ObsSpace.h"
#include "ioda/missingValue.h"
#include "oops/util/DateTime.h"

namespace ioda {

// -----------------------------------------------------------------------------
int obsspace_get_nobs_f(const ObsSpace & obss) {
  return obss.nobs();
}
// -----------------------------------------------------------------------------
int obsspace_get_nlocs_f(const ObsSpace & obss) {
  return obss.nlocs();
}
// -----------------------------------------------------------------------------
bool obsspace_has_f(const ObsSpace & obss, const char * group, const char * vname) {
  return obss.has(std::string(group), std::string(vname));
}
// -----------------------------------------------------------------------------
double obspace_missing_value_f() {
  return missingValue<double>();
}
// -----------------------------------------------------------------------------
void obsspace_get_int32_f(const ObsSpace & obss, const char * group, const char * vname,
                          const std::size_t & length, int32_t* vec) {
  ASSERT(length >= obss.nlocs());
  obss.get_db(std::string(group), std::string(vname), length, vec);
}
// -----------------------------------------------------------------------------
void obsspace_get_int64_f(const ObsSpace & obss, const char * group, const char * vname,
                          const std::size_t & length, int64_t* vec) {
  ASSERT(length >= obss.nlocs());
//  obss.get_db(std::string(group), std::string(vname), length, vec);
}
// -----------------------------------------------------------------------------
void obsspace_get_real32_f(const ObsSpace & obss, const char * group, const char * vname,
                           const std::size_t & length, float* vec) {
  ASSERT(length >= obss.nlocs());
//  obss.get_db(std::string(group), std::string(vname), length, vec);
}
// -----------------------------------------------------------------------------
void obsspace_get_real64_f(const ObsSpace & obss, const char * group, const char * vname,
                           const std::size_t & length, double* vec) {
  ASSERT(length >= obss.nlocs());
  obss.get_db(std::string(group), std::string(vname), length, vec);
}
// -----------------------------------------------------------------------------
void obsspace_put_int32_f(ObsSpace & obss, const char * group, const char * vname,
                          const std::size_t & length, int32_t* vec) {
  ASSERT(length == obss.nlocs());
  obss.put_db(std::string(group), std::string(vname), length, vec);
}
// -----------------------------------------------------------------------------
void obsspace_put_int64_f(ObsSpace & obss, const char * group, const char * vname,
                          const std::size_t & length, int64_t* vec) {
  ASSERT(length == obss.nlocs());
//  obss.put_db(std::string(group), std::string(vname), length, vec);
}
// -----------------------------------------------------------------------------
void obsspace_put_real32_f(ObsSpace & obss, const char * group, const char * vname,
                           const std::size_t & length, float* vec) {
  ASSERT(length == obss.nlocs());
//  obss.put_db(std::string(group), std::string(vname), length, vec);
}
// -----------------------------------------------------------------------------
void obsspace_put_real64_f(ObsSpace & obss, const char * group, const char * vname,
                           const std::size_t & length, double* vec) {
  ASSERT(length == obss.nlocs());
  obss.put_db(std::string(group), std::string(vname), length, vec);
}
// -----------------------------------------------------------------------------

}  // namespace ioda
