/*
 * (C) Copyright 2017-2019 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/core/obsspace_f.h"

#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

#include "eckit/exception/Exceptions.h"

#include "oops/util/DateTime.h"
#include "oops/util/TimeWindow.h"

#include "ioda/ObsSpace.h"

namespace ioda {

// -----------------------------------------------------------------------------
  const ObsSpace * obsspace_construct_f(const eckit::Configuration * obsconf,
                                        const eckit::LocalConfiguration * timewinconf) {
  ObsTopLevelParameters params;
  params.validateAndDeserialize(*obsconf);
  return new ObsSpace(params, oops::mpi::world(),
                      util::TimeWindow(*timewinconf),
                      oops::mpi::myself());
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
  return &obss.assimvariables();
}

// -----------------------------------------------------------------------------
std::size_t obsspace_get_gnlocs_f(const ObsSpace & obss) {
  return obss.globalNumLocs();
}
// -----------------------------------------------------------------------------
std::size_t obsspace_get_nlocs_f(const ObsSpace & obss) {
  return obss.nlocs();
}
// -----------------------------------------------------------------------------
std::size_t obsspace_get_nchans_f(const ObsSpace & obss) {
  return obss.nchans();
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
void obsspace_get_dim_name_f(const ObsSpace & obss, const int & dim_id,
                             std::size_t & len_dim_name, char * dim_name) {
  std::string dimName = obss.get_dim_name(static_cast<ioda::ObsDimensionId>(dim_id));
  len_dim_name = dimName.size();
  ASSERT(len_dim_name < 100);  // to not overflow the associated fortran string
  strncpy(dim_name, dimName.c_str(), len_dim_name);
}
// -----------------------------------------------------------------------------
std::size_t obsspace_get_dim_size_f(const ObsSpace & obss, const int & dim_id) {
  return obss.get_dim_size(static_cast<ioda::ObsDimensionId>(dim_id));
}
// -----------------------------------------------------------------------------
int obsspace_get_dim_id_f(const ObsSpace & obss, const char * dim_name) {
  return static_cast<int>(obss.get_dim_id(std::string(dim_name)));
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
// with channel hack -----------------------------------------------------------
// -----------------------------------------------------------------------------
void obsspace_get_int32_f(const ObsSpace & obss, const char * group, const char * vname,
                          const std::size_t & length, int32_t* vec,
                          const std::size_t & len_cs, int* chan_select) {
  ASSERT(len_cs <= obss.nchans());
  std::vector<int> chanSelect(len_cs);
  chanSelect.assign(chan_select, chan_select + len_cs);
  std::vector<int32_t> vdata(length);
  obss.get_db(std::string(group), std::string(vname), vdata, chanSelect);
  std::copy(vdata.begin(), vdata.end(), vec);
}
// -----------------------------------------------------------------------------
void obsspace_get_int64_f(const ObsSpace & obss, const char * group, const char * vname,
                          const std::size_t & length, int64_t* vec,
                          const std::size_t & len_cs, int* chan_select) {
  ASSERT(len_cs <= obss.nchans());
  std::vector<int> chanSelect(len_cs);
  chanSelect.assign(chan_select, chan_select + len_cs);
  std::vector<int32_t> vdata(length);
  obss.get_db(std::string(group), std::string(vname), vdata, chanSelect);
  std::copy(vdata.begin(), vdata.end(), vec);
}
// -----------------------------------------------------------------------------
void obsspace_get_real32_f(const ObsSpace & obss, const char * group, const char * vname,
                           const std::size_t & length, float* vec,
                          const std::size_t & len_cs, int* chan_select) {
  ASSERT(len_cs <= obss.nchans());
  std::vector<int> chanSelect(len_cs);
  chanSelect.assign(chan_select, chan_select + len_cs);
  std::vector<float> vdata(length);
  obss.get_db(std::string(group), std::string(vname), vdata, chanSelect);
  std::copy(vdata.begin(), vdata.end(), vec);
}
// -----------------------------------------------------------------------------
void obsspace_get_real64_f(const ObsSpace & obss, const char * group, const char * vname,
                           const std::size_t & length, double* vec,
                          const std::size_t & len_cs, int* chan_select) {
  ASSERT(len_cs <= obss.nchans());
  std::vector<int> chanSelect(len_cs);
  chanSelect.assign(chan_select, chan_select + len_cs);
  std::vector<double> vdata(length);
  obss.get_db(std::string(group), std::string(vname), vdata, chanSelect);
  std::copy(vdata.begin(), vdata.end(), vec);
}

// without channel hack  -------------------------------------------------------
// -----------------------------------------------------------------------------
void obsspace_get_nd_int32_f(const ObsSpace & obss, const char * group, const char * vname,
                          const std::size_t & length, int32_t* vec) {
  std::vector<int32_t> vdata(length);
  obss.get_db(std::string(group), std::string(vname), vdata);
  std::copy(vdata.begin(), vdata.end(), vec);
}
// -----------------------------------------------------------------------------
void obsspace_get_nd_int64_f(const ObsSpace & obss, const char * group, const char * vname,
                          const std::size_t & length, int64_t* vec) {
  std::vector<int64_t> vdata(length);
  obss.get_db(std::string(group), std::string(vname), vdata);
  std::copy(vdata.begin(), vdata.end(), vec);
}
// -----------------------------------------------------------------------------
void obsspace_get_nd_real32_f(const ObsSpace & obss, const char * group, const char * vname,
                           const std::size_t & length, float* vec) {
  std::vector<float> vdata(length);
  obss.get_db(std::string(group), std::string(vname), vdata);
  std::copy(vdata.begin(), vdata.end(), vec);
}
// -----------------------------------------------------------------------------
void obsspace_get_nd_real64_f(const ObsSpace & obss, const char * group, const char * vname,
                           const std::size_t & length, double* vec) {
  std::vector<double> vdata(length);
  obss.get_db(std::string(group), std::string(vname), vdata);
  std::copy(vdata.begin(), vdata.end(), vec);
}

// -----------------------------------------------------------------------------
void obsspace_get_datetime_f(const ObsSpace & obss, const char * group, const char * vname,
                             const std::size_t & length, int32_t* date, int32_t* time,
                          const std::size_t & len_cs, int* chan_select) {
  ASSERT(len_cs <= obss.nchans());
  std::vector<int> chanSelect(len_cs);
  chanSelect.assign(chan_select, chan_select + len_cs);

  // Load a DateTime vector from the database, then convert to a date and time
  // vector which are then returned.
  util::DateTime temp_dt("0000-01-01T00:00:00Z");
  std::vector<util::DateTime> dt_vect(length, temp_dt);
  obss.get_db(std::string(group), std::string(vname), dt_vect, chanSelect);

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
void obsspace_get_window_f(const ObsSpace & obss, util::DateTime * begin,
                           util::DateTime * end) {
  *begin = obss.windowStart();
  *end = obss.windowEnd();
}
// -----------------------------------------------------------------------------
void obsspace_get_bool_f(const ObsSpace & obss, const char * group, const char * vname,
                         const std::size_t & length, bool* vec,
                         const std::size_t & len_cs, int* chan_select) {
  ASSERT(len_cs <= obss.nchans());
  std::vector<int> chanSelect(len_cs);
  chanSelect.assign(chan_select, chan_select + len_cs);
  std::vector<bool> vdata(length);
  obss.get_db(std::string(group), std::string(vname), vdata, chanSelect);
  std::copy(vdata.begin(), vdata.end(), vec);
}
// -----------------------------------------------------------------------------
void obsspace_put_int32_f(ObsSpace & obss, const char * group, const char * vname,
                          const std::size_t & length, int32_t* vec,
                          const std::size_t & ndims, int* dim_ids) {
  // Generate the dimension list (names) and the total number of elements required
  // (product of dimension sizes). vec is just an allocated memory buffer in which
  // to place the data from the ObsSpace variable.
  std::vector<std::string> dimList;
  for (std::size_t i = 0; i < ndims; ++i) {
      ObsDimensionId dimId = static_cast<ioda::ObsDimensionId>(dim_ids[i]);
      dimList.push_back(obss.get_dim_name(dimId));
  }

  std::vector<int32_t> vdata;
  vdata.assign(vec, vec + length);

  obss.put_db(std::string(group), std::string(vname), vdata, dimList);
}
// -----------------------------------------------------------------------------
void obsspace_put_int64_f(ObsSpace & obss, const char * group, const char * vname,
                          const std::size_t & length, int64_t* vec,
                          const std::size_t & ndims, int* dim_ids) {
  // Generate the dimension list (names) and the total number of elements required
  // (product of dimension sizes). vec is just an allocated memory buffer in which
  // to place the data from the ObsSpace variable.
  std::vector<std::string> dimList;
  for (std::size_t i = 0; i < ndims; ++i) {
      ObsDimensionId dimId = static_cast<ioda::ObsDimensionId>(dim_ids[i]);
      dimList.push_back(obss.get_dim_name(dimId));
  }

  std::vector<int32_t> vdata;
  vdata.assign(vec, vec + length);

  obss.put_db(std::string(group), std::string(vname), vdata, dimList);
}
// -----------------------------------------------------------------------------
void obsspace_put_real32_f(ObsSpace & obss, const char * group, const char * vname,
                           const std::size_t & length, float* vec,
                           const std::size_t & ndims, int* dim_ids) {
  // Generate the dimension list (names) and the total number of elements required
  // (product of dimension sizes). vec is just an allocated memory buffer in which
  // to place the data from the ObsSpace variable.
  std::vector<std::string> dimList;
  for (std::size_t i = 0; i < ndims; ++i) {
      ObsDimensionId dimId = static_cast<ioda::ObsDimensionId>(dim_ids[i]);
      dimList.push_back(obss.get_dim_name(dimId));
  }

  std::vector<float> vdata;
  vdata.assign(vec, vec + length);

  obss.put_db(std::string(group), std::string(vname), vdata, dimList);
}
// -----------------------------------------------------------------------------
void obsspace_put_real64_f(ObsSpace & obss, const char * group, const char * vname,
                           const std::size_t & length, double* vec,
                           const std::size_t & ndims, int* dim_ids) {
  // Generate the dimension list (names) and the total number of elements required
  // (product of dimension sizes). vec is just an allocated memory buffer in which
  // to place the data from the ObsSpace variable.
  std::vector<std::string> dimList;
  for (std::size_t i = 0; i < ndims; ++i) {
      ObsDimensionId dimId = static_cast<ioda::ObsDimensionId>(dim_ids[i]);
      dimList.push_back(obss.get_dim_name(dimId));
  }

  std::vector<double> vdata;
  vdata.assign(vec, vec + length);

  obss.put_db(std::string(group), std::string(vname), vdata, dimList);
}
// -----------------------------------------------------------------------------
void obsspace_put_bool_f(ObsSpace & obss, const char * group, const char * vname,
                          const std::size_t & length, bool* vec,
                          const std::size_t & ndims, int* dim_ids) {
  // Generate the dimension list (names) and the total number of elements required
  // (product of dimension sizes). vec is just an allocated memory buffer in which
  // to place the data from the ObsSpace variable.
  std::vector<std::string> dimList;
  for (std::size_t i = 0; i < ndims; ++i) {
      ObsDimensionId dimId = static_cast<ioda::ObsDimensionId>(dim_ids[i]);
      dimList.push_back(obss.get_dim_name(dimId));
  }

  std::vector<bool> vdata;
  vdata.assign(vec, vec + length);

  obss.put_db(std::string(group), std::string(vname), vdata, dimList);
}
// -----------------------------------------------------------------------------
int obsspace_get_location_dim_id_f() {
  return static_cast<int>(ObsDimensionId::Location);
}
// -----------------------------------------------------------------------------
int obsspace_get_channel_dim_id_f() {
  return static_cast<int>(ObsDimensionId::Channel);
}
// -----------------------------------------------------------------------------

}  // namespace ioda
