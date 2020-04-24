/*
 * (C) Copyright 2017-2019 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/obsspace_f.h"

#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

#include "eckit/exception/Exceptions.h"

#include "oops/util/DateTime.h"

#include "ioda/ObsSpace.h"

namespace ioda {

// -----------------------------------------------------------------------------
const ObsSpace * obsspace_construct_f(const eckit::Configuration * conf,
                                      const util::DateTime * begin,
                                      const util::DateTime * end) {
  return new ObsSpace(*conf, oops::mpi::comm(), *begin, *end);
}

// -----------------------------------------------------------------------------
void obsspace_destruct_f(ObsSpace * obss) {
  ASSERT(obss != nullptr);
  delete obss;
}

// -----------------------------------------------------------------------------
void obsspace_obsname_f(const ObsSpace & obss, size_t & lcname, char * cname) {
  std::string obsname = obss.obsname();
  lcname = obsname.size();
  ASSERT(lcname < 100);  // to not overflow the associated fortran string
  strncpy(cname, obsname.c_str(), lcname);
}

// -----------------------------------------------------------------------------
const oops::Variables * obsspace_obsvariables_f(const ObsSpace & obss) {
  return &obss.obsvariables();
}

// -----------------------------------------------------------------------------
std::size_t obsspace_get_gnlocs_f(const ObsSpace & obss) {
  return obss.gnlocs();
}
// -----------------------------------------------------------------------------
std::size_t obsspace_get_nlocs_f(const ObsSpace & obss) {
  return obss.nlocs();
}
// -----------------------------------------------------------------------------
std::size_t obsspace_get_nrecs_f(const ObsSpace & obss) {
  return obss.nrecs();
}
// -----------------------------------------------------------------------------
std::size_t obsspace_get_nvars_f(const ObsSpace & obss) {
  return obss.nvars();
}
// -----------------------------------------------------------------------------
void obsspace_get_comm_f(const ObsSpace & obss, int & lcname, char * cname) {
  lcname = obss.comm().name().size();
  ASSERT(lcname < 100);  // to not overflow the associated fortran string
  strncpy(cname, obss.comm().name().c_str(), lcname);
}
// -----------------------------------------------------------------------------
void obsspace_get_recnum_f(const ObsSpace & obss,
                           const std::size_t & length, std::size_t * recnum) {
  ASSERT(length >= obss.nlocs());
  for (std::size_t i = 0; i < length; i++) {
    recnum[i] = obss.recnum()[i];
  }
}
// -----------------------------------------------------------------------------
void obsspace_get_index_f(const ObsSpace & obss,
                          const std::size_t & length, std::size_t * index) {
  ASSERT(length >= obss.nlocs());
  for (std::size_t i = 0; i < length; i++) {
    // Fortran array indices start at 1, whereas C indices start at 0.
    // Add 1 to each index value as it is handed off from C to Fortran.
    index[i] = obss.index()[i] + 1;
  }
}
// -----------------------------------------------------------------------------
bool obsspace_has_f(const ObsSpace & obss, const char * group, const char * vname) {
  return obss.has(std::string(group), std::string(vname));
}
// -----------------------------------------------------------------------------
void obsspace_get_int32_f(const ObsSpace & obss, const char * group, const char * vname,
                          const std::size_t & length, int32_t* vec) {
  if (std::string(group) == "VarMetaData") ASSERT(length >= obss.nvars());
  else
    ASSERT(length >= obss.nlocs());
  std::vector<int32_t> vdata(length);
  obss.get_db(std::string(group), std::string(vname), vdata);
  std::copy(vdata.begin(), vdata.end(), vec);
}
// -----------------------------------------------------------------------------
void obsspace_get_int64_f(const ObsSpace & obss, const char * group, const char * vname,
                          const std::size_t & length, int64_t* vec) {
  if (std::string(group) == "VarMetaData") ASSERT(length >= obss.nvars());
  else
    ASSERT(length >= obss.nlocs());
//  obss.get_db(std::string(group), std::string(vname), vec);
}
// -----------------------------------------------------------------------------
void obsspace_get_real32_f(const ObsSpace & obss, const char * group, const char * vname,
                           const std::size_t & length, float* vec) {
  if (std::string(group) == "VarMetaData") ASSERT(length >= obss.nvars());
  else
    ASSERT(length >= obss.nlocs());
//  obss.get_db(std::string(group), std::string(vname), vec);
}
// -----------------------------------------------------------------------------
void obsspace_get_real64_f(const ObsSpace & obss, const char * group, const char * vname,
                           const std::size_t & length, double* vec) {
  if (std::string(group) == "VarMetaData") ASSERT(length >= obss.nvars());
  else
    ASSERT(length >= obss.nlocs());
  std::vector<double> vdata(length);
  obss.get_db(std::string(group), std::string(vname), vdata);
  std::copy(vdata.begin(), vdata.end(), vec);
}
// -----------------------------------------------------------------------------
void obsspace_get_datetime_f(const ObsSpace & obss, const char * group, const char * vname,
                             const std::size_t & length, int32_t* date, int32_t* time) {
  if (std::string(group) == "VarMetaData") ASSERT(length >= obss.nvars());
  else
    ASSERT(length >= obss.nlocs());

  // Load a DateTime vector from the database, then convert to a date and time
  // vector which are then returned.
  util::DateTime temp_dt("0000-01-01T00:00:00Z");
  std::vector<util::DateTime> dt_vect(length, temp_dt);
  obss.get_db(std::string(group), std::string(vname), dt_vect);

  // Convert to date and time values. The DateTime utilities can return year, month,
  // day, hour, minute second.
  int year;
  int month;
  int day;
  int hour;
  int minute;
  int second;
  for (std::size_t i = 0; i < length; i++) {
    dt_vect[i].toYYYYMMDDhhmmss(year, month, day, hour, minute, second);
    date[i] = (year * 10000) + (month * 100) + day;
    time[i] = (hour * 10000) + (minute * 100) + second;
  }
}
// -----------------------------------------------------------------------------
void obsspace_put_int32_f(ObsSpace & obss, const char * group, const char * vname,
                          const std::size_t & length, int32_t* vec) {
  if (std::string(group) == "VarMetaData") ASSERT(length >= obss.nvars());
  else
    ASSERT(length >= obss.nlocs());
  std::vector<int32_t> vdata;
  vdata.assign(vec, vec + length);
  obss.put_db(std::string(group), std::string(vname), vdata);
}
// -----------------------------------------------------------------------------
void obsspace_put_int64_f(ObsSpace & obss, const char * group, const char * vname,
                          const std::size_t & length, int64_t* vec) {
  if (std::string(group) == "VarMetaData") ASSERT(length >= obss.nvars());
  else
    ASSERT(length >= obss.nlocs());
//  obss.put_db(std::string(group), std::string(vname), vec);
}
// -----------------------------------------------------------------------------
void obsspace_put_real32_f(ObsSpace & obss, const char * group, const char * vname,
                           const std::size_t & length, float* vec) {
  if (std::string(group) == "VarMetaData") ASSERT(length >= obss.nvars());
  else
    ASSERT(length >= obss.nlocs());
//  obss.put_db(std::string(group), std::string(vname), vec);
}
// -----------------------------------------------------------------------------
void obsspace_put_real64_f(ObsSpace & obss, const char * group, const char * vname,
                           const std::size_t & length, double* vec) {
  if (std::string(group) == "VarMetaData") ASSERT(length >= obss.nvars());
  else
    ASSERT(length >= obss.nlocs());
  std::vector<double> vdata;
  vdata.assign(vec, vec + length);
  obss.put_db(std::string(group), std::string(vname), vdata);
}
// -----------------------------------------------------------------------------

}  // namespace ioda
