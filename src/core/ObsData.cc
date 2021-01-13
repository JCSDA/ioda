/*
 * (C) Copyright 2017-2019 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/core/ObsData.h"

#include <algorithm>
#include <fstream>
#include <functional>
#include <iomanip>
#include <map>
#include <memory>
#include <numeric>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "eckit/config/Configuration.h"
#include "eckit/exception/Exceptions.h"

#include "oops/mpi/mpi.h"
#include "oops/util/abor1_cpp.h"
#include "oops/util/DateTime.h"
#include "oops/util/Duration.h"
#include "oops/util/Logger.h"
#include "oops/util/Random.h"
#include "oops/util/stringFunctions.h"

#include "ioda/distribution/DistributionFactory.h"
#include "ioda/io/IodaIOfactory.h"

#include "atlas/util/Earth.h"

namespace ioda {

// -----------------------------------------------------------------------------
/*!
 * \details Config based constructor for an ObsData object. This constructor will read
 *          in from the obs file and transfer the variables into the obs container. Obs
 *          falling outside the DA timing window, specified by bgn and end, will be
 *          discarded before storing them in the obs container.
 *
 * \param[in] config ECKIT configuration segment holding obs types specs
 * \param[in] bgn    DateTime object holding the start of the DA timing window
 * \param[in] end    DateTime object holding the end of the DA timing window
 */
ObsData::ObsData(const eckit::Configuration & config, const eckit::mpi::Comm & comm,
                 const util::DateTime & bgn, const util::DateTime & end,
                 const eckit::mpi::Comm & timeComm)
  : config_(config), winbgn_(bgn), winend_(end), commMPI_(comm),
    gnlocs_(0), nlocs_(0), nvars_(0), nrecs_(0), file_unexpected_dtypes_(false),
    file_excess_dims_(false), in_max_frame_size_(0), out_max_frame_size_(0),
    int_database_(), float_database_(), string_database_(), datetime_database_(),
    obsvars_(), next_rec_num_(0)
{
  oops::Log::trace() << "ObsData::ObsData config  = " << config << std::endl;

  obsname_ = config.getString("name");
  distname_ = config.getString("distribution", "RoundRobin");

  obsvars_ = oops::Variables(config, "simulated variables");
  oops::Log::debug() << obsname_ << " vars: " << obsvars_ << std::endl;

  // Create the MPI distribution object
  std::unique_ptr<DistributionFactory> distFactory;
  dist_.reset(distFactory->createDistribution(this->comm(), config, distname_));

  // Initialize the obs space container
  if (config.has("obsdatain")) {
    // Initialize the container from an input obs file
    obs_group_variable_ = config.getString("obsdatain.obsgrouping.group variable", "");
    obs_sort_variable_ = config.getString("obsdatain.obsgrouping.sort variable", "");
    obs_sort_order_ = config.getString("obsdatain.obsgrouping.sort order", "ascending");
    if ((obs_sort_order_ != "ascending") && (obs_sort_order_ != "descending")) {
      std::string ErrMsg =
        std::string("ObsData::ObsData: Must use one of 'ascending' or 'descending' ") +
        std::string("for the 'sort order:' YAML configuration keyword.");
      ABORT(ErrMsg);
    }
    filein_ = config.getString("obsdatain.obsfile");
    in_max_frame_size_ = config.getUnsigned("obsdatain.max frame size",
                                            IODAIO_DEFAULT_FRAME_SIZE);
    oops::Log::trace() << obsname_ << " file in = " << filein_ << std::endl;
    InitFromFile(filein_, in_max_frame_size_);
    if (file_unexpected_dtypes_) {
      if (commMPI_.rank() == 0) {
        oops::Log::warning()
          << "ObsData::ObsData:: WARNING: Input file contains variables "
          << "with unexpected data types" << std::endl
          << "  Input file: " << filein_ << std::endl;
      }
    }

    if (file_excess_dims_) {
      if (commMPI_.rank() == 0) {
        oops::Log::warning()
          << "ObsData::ObsData:: WARNING: Input file contains variables "
          << "with excess number of dimensions (these variables were skipped)" << std::endl
          << "  Input file: " << filein_ << std::endl;
      }
    }

    // assign a record to a unique PE
    nlocspatch_ = dist_->computePatchLocs(gnlocs_);
    if (distname_ != "Halo") {
      nlocspatch_ = nlocs_;
    }

    if (obs_sort_variable_ != "") {
      BuildSortedObsGroups();
    }
  } else if (config.has("generate")) {
    // Initialize the container from the generateDistribution method
    eckit::LocalConfiguration genconfig(config, "generate");
    generateDistribution(genconfig);
  } else {
    // Error - must have one of obsdatain or Generate
    std::string ErrorMsg =
      "ObsData::ObsData: Must use one of 'obsdatain' or 'generate' in the YAML configuration.";
    ABORT(ErrorMsg);
  }
  nrecs_ = unique_rec_nums_.size();

  // Check to see if an output file has been requested.
  if (config.has("obsdataout.obsfile")) {
    std::string filename = config.getString("obsdataout.obsfile");
    out_max_frame_size_ = config.getUnsigned("obsdataout.max frame size",
                                             IODAIO_DEFAULT_FRAME_SIZE);

    // If present, change '%{member}%' to 'iii'
    util::stringfunctions::swapNameMember(config, filename);

    // Find the left-most dot in the file name, and use that to pick off the file name
    // and file extension.
    std::size_t found = filename.find_last_of(".");
    if (found == std::string::npos)
      found = filename.length();

    // Get the process rank number and format it
    std::ostringstream ss;
    ss << "_" << std::setw(4) << std::setfill('0') << this->comm().rank();
    if (timeComm.size() > 1) ss << "_" << timeComm.rank();

    // Construct the output file name
    fileout_ = filename.insert(found, ss.str());

    // Check to see if user is trying to overwrite an existing file. For now always allow
    // the overwrite, but issue a warning if we are about to clobber an existing file.
    std::ifstream infile(fileout_);
    if (infile.good() && (commMPI_.rank() == 0)) {
        oops::Log::warning() << "ObsData::ObsData WARNING: Overwriting output file "
                             << fileout_ << std::endl;
      }
  } else {
    oops::Log::debug() << "ObsData::ObsData output file is not required " << std::endl;
  }

  oops::Log::trace() << "ObsData::ObsData contructed name = " << obsname() << std::endl;
}

// -----------------------------------------------------------------------------
/*!
 * \details Destructor for an ObsData object. This destructor will clean up the ObsData
 *          object and optionally write out the contents of the obs container into
 *          the output file. The save-to-file operation is invoked when an output obs
 *          file is specified in the ECKIT configuration segment associated with the
 *          ObsData object.
 */
ObsData::~ObsData() {
  oops::Log::trace() << "ObsData::ObsData destructor begin" << std::endl;
  if (fileout_.size() != 0) {
    oops::Log::info() << obsname() << ": save database to " << fileout_ << std::endl;
    SaveToFile(fileout_, out_max_frame_size_);
  } else {
    oops::Log::info() << obsname() << " :  no output" << std::endl;
  }
  oops::Log::trace() << "ObsData::ObsData destructor end" << std::endl;
}

// -----------------------------------------------------------------------------
/*!
 * \brief transfer data from the obs container to vdata
 *
 * \details The following four get_db methods are the same except for the data type
 *          of the data being transferred (integer, float, double, DateTime). The
 *          caller needs to allocate the memory that the vdata parameter points to.
 *
 * \param[in]  group Name of container group (ObsValue, ObsError, MetaData, etc.)
 * \param[in]  name  Name of container variable
 * \param[out] vdata Vector where container data is being transferred to
 */

void ObsData::get_db(const std::string & group, const std::string & name,
                      std::vector<int> & vdata) const {
  std::vector<std::size_t> vshape(1, vdata.size());
  int_database_.LoadFromDb(group, name, vshape, vdata);
}

void ObsData::get_db(const std::string & group, const std::string & name,
                      std::vector<float> & vdata) const {
  std::vector<std::size_t> vshape(1, vdata.size());
  float_database_.LoadFromDb(group, name, vshape, vdata);
}

void ObsData::get_db(const std::string & group, const std::string & name,
                      std::vector<double> & vdata) const {
  std::vector<std::size_t> vshape(1, vdata.size());
  // load the float values from the database and convert to double
  std::vector<float> FloatData(vdata.size(), 0.0);
  float_database_.LoadFromDb(group, name, vshape, FloatData);
  ConvertVarType<float, double>(FloatData, vdata);
}

void ObsData::get_db(const std::string & group, const std::string & name,
                      std::vector<std::string> & vdata) const {
  std::vector<std::size_t> vshape(1, vdata.size());
  string_database_.LoadFromDb(group, name, vshape, vdata);
}

void ObsData::get_db(const std::string & group, const std::string & name,
                      std::vector<util::DateTime> & vdata) const {
  std::vector<std::size_t> vshape(1, vdata.size());
  datetime_database_.LoadFromDb(group, name, vshape, vdata);
}

// -----------------------------------------------------------------------------
/*!
 * \brief transfer data from vdata to the obs container
 *
 * \details The following four put_db methods are the same except for the data type
 *          of the data being transferred (integer, float, double, DateTime). The
 *          caller needs to allocate and assign the memory that the vdata parameter
 *          points to.
 *
 * \param[in]  group Name of container group (ObsValue, ObsError, MetaData, etc.)
 * \param[in]  name  Name of container variable
 * \param[out] vdata Vector where container data is being transferred from
 */

void ObsData::put_db(const std::string & group, const std::string & name,
                      const std::vector<int> & vdata) {
  std::vector<std::size_t> vshape(1, vdata.size());
  int_database_.StoreToDb(group, name, vshape, vdata);
}

void ObsData::put_db(const std::string & group, const std::string & name,
                      const std::vector<float> & vdata) {
  std::vector<std::size_t> vshape(1, vdata.size());
  float_database_.StoreToDb(group, name, vshape, vdata);
}

void ObsData::put_db(const std::string & group, const std::string & name,
                      const std::vector<double> & vdata) {
  std::vector<std::size_t> vshape(1, vdata.size());
  // convert to float, then load into the database
  std::vector<float> FloatData(vdata.size());
  ConvertVarType<double, float>(vdata, FloatData);
  float_database_.StoreToDb(group, name, vshape, FloatData);
}

void ObsData::put_db(const std::string & group, const std::string & name,
                      const std::vector<std::string> & vdata) {
  std::vector<std::size_t> vshape(1, vdata.size());
  string_database_.StoreToDb(group, name, vshape, vdata);
}

void ObsData::put_db(const std::string & group, const std::string & name,
                      const std::vector<util::DateTime> & vdata) {
  std::vector<std::size_t> vshape(1, vdata.size());
  datetime_database_.StoreToDb(group, name, vshape, vdata);
}

// -----------------------------------------------------------------------------
/*!
 * \details This method checks for the existence of the group, name combination
 *          in the obs container. If the combination exists, "true" is returned,
 *          otherwise "false" is returned.
 */

bool ObsData::has(const std::string & group, const std::string & name) const {
  return (int_database_.has(group, name) || float_database_.has(group, name) ||
          string_database_.has(group, name) || datetime_database_.has(group, name));
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the data type of the variable stored in the obs
 *          container.
 */

ObsDtype ObsData::dtype(const std::string & group, const std::string & name) const {
  ObsDtype VarType = ObsDtype::None;
  if (int_database_.has(group, name)) {
    VarType = ObsDtype::Integer;
  } else if (float_database_.has(group, name)) {
    VarType = ObsDtype::Float;
  } else if (string_database_.has(group, name)) {
    VarType = ObsDtype::String;
  } else if (datetime_database_.has(group, name)) {
    VarType = ObsDtype::DateTime;
  }

  return VarType;
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the setting of the YAML configuration
 *          parameter: obsdatain.obsgrouping.group variable
 */
std::string ObsData::obs_group_var() const {
  return obs_group_variable_;
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the setting of the YAML configuration
 *          parameter: obsdatain.obsgrouping.sort variable
 */
std::string ObsData::obs_sort_var() const {
  return obs_sort_variable_;
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the setting of the YAML configuration
 *          parameter: obsdatain.obsgrouping.sort order
 */
std::string ObsData::obs_sort_order() const {
  return obs_sort_order_;
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the number of unique locations in the input
 *          obs file. Note that nlocs from the obs container may be smaller
 *          than nlocs from the input obs file due to the removal of obs outside
 *          the DA timing window and/or due to distribution of obs across
 *          multiple process elements.
 */
std::size_t ObsData::gnlocs() const {
  return gnlocs_;
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the number of unique locations in the obs
 *          container. Note that nlocs from the obs container may be smaller
 *          than nlocs from the input obs file due to the removal of obs outside
 *          the DA timing window and/or due to distribution of obs across
 *          multiple process elements.
 */
std::size_t ObsData::nlocs() const {
  return nlocs_;
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the number of patch locations on this PE.
 * the following equality holds ObsData::gnlocs() == distribution().sum(nlocspatch())
 */
std::size_t ObsData::nlocspatch() const {
  return nlocspatch_;
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the number of unique records in the obs
 *          container. A record is an atomic unit of locations that belong
 *          together such as a single radiosonde sounding.
 */
std::size_t ObsData::nrecs() const {
  return nrecs_;
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the number of unique variables in the obs
 *          container. "Variables" refers to the quantities that can be
 *          assimilated as opposed to meta data.
 */
std::size_t ObsData::nvars() const {
  return nvars_;
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns a reference to the record number vector
 *          data member. This is for read only access.
 */
const std::vector<std::size_t> & ObsData::recnum() const {
  return recnums_;
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns a reference to the index vector
 *          data member. This is for read only access.
 *
 * The returned vector has length nlocs() and contains the original indices of locations from the
 * input ioda file corresponding to locations stored in this ObsData object -- i.e. those that were
 * selected by the timing window filter and the MPI distribution.
 *
 * Example 1: Suppose the RoundRobin distribution is used and and there are two MPI tasks (ranks 0
 * and 1). The even-numbered locations from the file will go to rank 0, and the odd-numbered
 * locations will go to rank 1. This means that `ObsData::index()` will return the vector `0, 2,
 * 4, 6, ...` on rank 0 and `1, 3, 5, 7, ...` on rank 1.
 *
 * Example 2: Suppose MPI is not used and the file contains 10 locations in total, but locations 2,
 * 3 and 7 are outside the DA timing window. In this case, `ObsData::index()` will return `0, 1, 4,
 * 5, 6, 8, 9`.
 */
const std::vector<std::size_t> & ObsData::index() const {
  return indx_;
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the begin iterator associated with the
 *          recidx_ data member.
 */
const ObsData::RecIdxIter ObsData::recidx_begin() const {
  return recidx_.begin();
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the end iterator associated with the
 *          recidx_ data member.
 */
const ObsData::RecIdxIter ObsData::recidx_end() const {
  return recidx_.end();
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns a boolean value indicating whether the
 *          given record number exists in the recidx_ data member.
 */
bool ObsData::recidx_has(const std::size_t RecNum) const {
  RecIdxIter irec = recidx_.find(RecNum);
  return (irec != recidx_.end());
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the current record number, pointed to by the
 *          given iterator, from the recidx_ data member.
 */
std::size_t ObsData::recidx_recnum(const RecIdxIter & Irec) const {
  return Irec->first;
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the current vector, pointed to by the
 *          given iterator, from the recidx_ data member.
 */
const std::vector<std::size_t> & ObsData::recidx_vector(const RecIdxIter & Irec) const {
  return Irec->second;
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the current vector, pointed to by the
 *          given iterator, from the recidx_ data member.
 */
const std::vector<std::size_t> & ObsData::recidx_vector(const std::size_t RecNum) const {
  RecIdxIter Irec = recidx_.find(RecNum);
  if (Irec == recidx_.end()) {
    std::string ErrMsg =
      "ObsData::recidx_vector: Record number, " + std::to_string(RecNum) +
      ", does not exist in record index map.";
    ABORT(ErrMsg);
  }
  return Irec->second;
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the all of the record numbers from the
 *          recidx_ data member (ie, all the key values) in a vector.
 */
std::vector<std::size_t> ObsData::recidx_all_recnums() const {
  std::vector<std::size_t> RecNums(nrecs_);
  std::size_t recnum = 0;
  for (RecIdxIter Irec = recidx_.begin(); Irec != recidx_.end(); ++Irec) {
    RecNums[recnum] = Irec->first;
    recnum++;
  }
  return RecNums;
}

// -----------------------------------------------------------------------------
/*!
 * \details This method will generate a set of latitudes, longitudes and datetimes
 *          which can be used for testing without reading in an obs file. Two methods
 *          are supported, the first generating random values between specified latitudes,
 *          longitudes and a timing window, and the second copying lists specified by
 *          the user. This method is triggered using the "Generate" keyword in the
 *          configuration file and either of the two methods above are specified using
 *          the sub keywords "Random" or "List".
 *
 * \param[in] conf ECKIT configuration segment built from an input configuration file.
 */
void ObsData::generateDistribution(const eckit::Configuration & conf) {
  // Generate lat, lon, time values according to method specified in the config
  std::vector<float> latitude;
  std::vector<float> longitude;
  std::vector<util::DateTime> obs_datetimes;
  if (conf.has("random")) {
    genDistRandom(conf, latitude, longitude, obs_datetimes);
  } else if (conf.has("list")) {
    genDistList(conf, latitude, longitude, obs_datetimes);
  } else {
    std::string ErrorMsg =
      std::string("ObsData::generateDistribution: Must specify either ") +
      std::string("'random' or 'list' with 'generate' configuration keyword");
    ABORT(ErrorMsg);
  }

  // number of variables specified in simulate section
  nvars_ = obsvars_.size();

  // Read obs errors (one for each variable)
  const std::vector<float> err = conf.getFloatVector("obs errors");
  ASSERT(nvars_ == err.size());

  put_db("MetaData", "datetime", obs_datetimes);
  put_db("MetaData", "latitude", latitude);
  put_db("MetaData", "longitude", longitude);
  for (std::size_t ivar = 0; ivar < nvars_; ivar++) {
    std::vector<float> obserr(nlocs_, err[ivar]);
    put_db("ObsError", obsvars_[ivar], obserr);
  }
}

// -----------------------------------------------------------------------------
/*!
 * \details This method will generate a set of latitudes and longitudes of which
 *          can be used for testing without reading in an obs file. Two latitude
 *          values, two longitude values, the number of locations (nobs keyword)
 *          and an optional random seed are specified in the configuration given
 *          by the conf parameter. Random locations between the two latitudes and
 *          two longitudes are generated and stored in the obs container as meta data.
 *          Random time stamps that fall inside the given timing window (which is
 *          specified in the configuration file) are alos generated and stored
 *          in the obs container as meta data.
 *
 * \param[in]  conf  ECKIT configuration segment built from an input configuration file.
 * \param[out] Lats  Generated latitude values
 * \param[out] Lons  Generated longitude values
 * \param[out] Dtims Generated datetime values
 */
void ObsData::genDistRandom(const eckit::Configuration & conf, std::vector<float> & Lats,
                            std::vector<float> & Lons, std::vector<util::DateTime> & Dtimes) {
  gnlocs_  = conf.getInt("random.nobs");
  float lat1 = conf.getFloat("random.lat1");
  float lat2 = conf.getFloat("random.lat2");
  float lon1 = conf.getFloat("random.lon1");
  float lon2 = conf.getFloat("random.lon2");

  // Make the random_seed keyword optional. Can spec it for testing to get repeatable
  // values, and the user doesn't have to spec it if they want subsequent runs to
  // use different random sequences.
  unsigned int ran_seed;
  if (conf.has("random.random seed")) {
    ran_seed = conf.getInt("random.random seed");
  } else {
    ran_seed = std::time(0);  // based on the current date/time.
  }

  // Generate the indexing for MPI distribution
  std::unique_ptr<IodaIO> NoIO(nullptr);
  std::vector<std::size_t> DummyIndex = GenFrameIndexRecNums(NoIO, 0, gnlocs_);

  // Use the following formula to generate random lat, lon and time values.
  //
  //   val = val1 + (random_number_between_0_and_1 * (val2-val1))
  //
  // where val2 > val1.
  //
  // Create a list of random values between 0 and 1 to be used for genearting
  // random lat, lon and time vaules.
  //
  // Use different seeds for lat and lon so that in the case where lat and lon ranges
  // are the same, you get a different sequences for lat compared to lon.
  //
  // Have rank 0 generate the full length random sequences, and then
  // broadcast these to the other ranks. This ensures that every rank
  // contains the same random sequences. If all ranks generated their
  // own sequences, which they could do, the sequences between ranks
  // would be different in the case where random_seed is not specified.
  std::vector<float> RanVals(gnlocs_, 0.0);
  std::vector<float> RanVals2(gnlocs_, 0.0);
  if (this->comm().rank() == 0) {
    util::UniformDistribution<float> RanUD(gnlocs_, 0.0, 1.0, ran_seed);
    util::UniformDistribution<float> RanUD2(gnlocs_, 0.0, 1.0, ran_seed+1);

    RanVals = RanUD.data();
    RanVals2 = RanUD2.data();
  }
  this->comm().broadcast(RanVals, 0);
  this->comm().broadcast(RanVals2, 0);

  // Form the ranges val2-val for lat, lon, time
  float LatRange = lat2 - lat1;
  float LonRange = lon2 - lon1;
  util::Duration WindowDuration(this->windowEnd() - this->windowStart());
  float TimeRange = static_cast<float>(WindowDuration.toSeconds());

  // Create vectors for lat, lon, time, fill them with random values
  // inside their respective ranges, and put results into the obs container.
  Lats.assign(nlocs_, 0.0);
  Lons.assign(nlocs_, 0.0);
  Dtimes.assign(nlocs_, this->windowStart());

  util::Duration DurZero(0);
  util::Duration DurOneSec(1);
  for (std::size_t ii = 0; ii < nlocs_; ii++) {
    std::size_t index = indx_[ii];
    Lats[ii] = lat1 + (RanVals[index] * LatRange);
    Lons[ii] = lon1 + (RanVals2[index] * LonRange);

    // Currently the filter for time stamps on obs values is:
    //
    //     windowStart < ObsTime <= windowEnd
    //
    // If we get a zero OffsetDt, then change it to 1 second so that the observation
    // will remain inside the timing window.
    util::Duration OffsetDt(static_cast<int64_t>(RanVals[index] * TimeRange));
    if (OffsetDt == DurZero) {
      OffsetDt = DurOneSec;
    }
    // Dtimes elements were initialized to the window start
    Dtimes[ii] += OffsetDt;
  }
}

// -----------------------------------------------------------------------------
/*!
 * \details This method will generate a set of latitudes and longitudes of which
 *          can be used for testing without reading in an obs file. The values
 *          are simply read from lists in the configuration file. The purpose of
 *          this method is to allow the user to exactly specify obs locations.
 *
 * \param[in]  conf  ECKIT configuration segment built from an input configuration file.
 * \param[out] Lats  Generated latitude values
 * \param[out] Lons  Generated longitude values
 * \param[out] Dtims Generated datetime values
 */
void ObsData::genDistList(const eckit::Configuration & conf, std::vector<float> & Lats,
                          std::vector<float> & Lons, std::vector<util::DateTime> & Dtimes) {
  std::vector<float> latitudes = conf.getFloatVector("list.lats");
  std::vector<float> longitudes = conf.getFloatVector("list.lons");
  std::vector<std::string> DtStrings = conf.getStringVector("list.datetimes");
  std::vector<util::DateTime> datetimes;
  for (std::size_t i = 0; i < DtStrings.size(); ++i) {
    util::DateTime TempDt(DtStrings[i]);
    datetimes.push_back(TempDt);
  }

  // Generate the indexing for MPI distribution
  gnlocs_ = latitudes.size();
  std::unique_ptr<IodaIO> NoIO(nullptr);
  std::vector<std::size_t> DummyIndex = GenFrameIndexRecNums(NoIO, 0, gnlocs_);

  // Create vectors for lat, lon, time, fill them with the values from the
  // lists in the configuration.
  Lats.assign(nlocs_, 0.0);
  Lons.assign(nlocs_, 0.0);
  Dtimes.assign(nlocs_, this->windowStart());

  for (std::size_t ii = 0; ii < nlocs_; ii++) {
    std::size_t index = indx_[ii];
    Lats[ii] = latitudes[index];
    Lons[ii] = longitudes[index];
    Dtimes[ii] = datetimes[index];
  }
}

// -----------------------------------------------------------------------------
/*!
 * \details This method provides a way to print an ObsData object in an output
 *          stream. It simply prints a dummy message for now.
 */
void ObsData::print(std::ostream & os) const {
  os << "ObsData::print not implemented";
}

// -----------------------------------------------------------------------------
/*!
 * \details This method will initialize the obs container from the input obs file.
 *          All the variables from the input file will be read in and loaded into
 *          the obs container. Obs that fall outside the DA timing window will be
 *          filtered out before loading into the container. This method will also
 *          apply obs distribution across multiple process elements. For these reasons,
 *          the number of locations in the obs container may be smaller than the
 *          number of locations in the input obs file.
 *
 * \param[in] filename Path to input obs file
 */
void ObsData::InitFromFile(const std::string & filename, const std::size_t MaxFrameSize) {
  oops::Log::trace() << "ObsData::InitFromFile opening file: " << filename << std::endl;

  // Open the file for reading and record nlocs and nvars from the file.
  std::unique_ptr<IodaIO> fileio {ioda::IodaIOfactory::Create(filename, "r", MaxFrameSize)};
  gnlocs_ = fileio->nlocs();

  // Walk through the frames and select the records according to the MPI distribution
  // and if the records fall inside the DA timing window. nvars_ for ObsData is the
  // number of variables with the GroupName ObsValue. Since we can be reading in
  // multiple frames, only check for the ObsValue group on the first frame.
  nvars_ = 0;
  bool FirstFrame = true;
  fileio->frame_initialize();
  for (IodaIO::FrameIter iframe = fileio->frame_begin();
                       iframe != fileio->frame_end(); ++iframe) {
    std::size_t FrameStart = fileio->frame_start(iframe);
    std::size_t FrameSize = fileio->frame_size(iframe);

    // Fill in the current frame from the file
    fileio->frame_read(iframe);

    // Calculate the corresponding segments of indx_ and recnums_ vectors for this
    // frame. Use these segments to select the rows from the frame before storing in
    // the obs space container.
    std::vector<std::size_t> FrameIndex = GenFrameIndexRecNums(fileio, FrameStart, FrameSize);

    // Integer variables
    for (IodaIO::FrameIntIter idata = fileio->frame_int_begin();
                              idata != fileio->frame_int_end(); ++idata) {
      std::string GroupName = fileio->frame_int_get_gname(idata);
      if (FirstFrame && (GroupName == "ObsValue")) { nvars_++; }
      std::string VarName = fileio->frame_int_get_vname(idata);
      std::vector<std::size_t> VarShape = fileio->var_shape(GroupName, VarName);
      std::vector<int> FrameData;
      fileio->frame_int_get_data(GroupName, VarName, FrameData);
      std::vector<std::size_t> FrameShape = VarShape;
      FrameShape[0] = FrameData.size();
      if (VarShape[0] == gnlocs_) {
        std::vector<std::size_t> IndexedShape;
        std::vector<int> SelectedData =
             ApplyIndex(FrameData, VarShape, FrameIndex, FrameShape);
        int_database_.StoreToDb(GroupName, VarName, FrameShape, SelectedData, true);
      } else {
        int_database_.StoreToDb(GroupName, VarName, FrameShape, FrameData, true);
      }
    }

    // Float variables
    for (IodaIO::FrameFloatIter idata = fileio->frame_float_begin();
                                idata != fileio->frame_float_end(); ++idata) {
      std::string GroupName = fileio->frame_float_get_gname(idata);
      if (FirstFrame && (GroupName == "ObsValue")) { nvars_++; }
      std::string VarName = fileio->frame_float_get_vname(idata);
      std::vector<std::size_t> VarShape = fileio->var_shape(GroupName, VarName);
      std::vector<float> FrameData;
      fileio->frame_float_get_data(GroupName, VarName, FrameData);
      std::vector<std::size_t> FrameShape = VarShape;
      FrameShape[0] = FrameData.size();
      if (VarShape[0] == gnlocs_) {
        std::vector<std::size_t> IndexedShape;
        std::vector<float> SelectedData =
             ApplyIndex(FrameData, VarShape, FrameIndex, FrameShape);
        float_database_.StoreToDb(GroupName, VarName, FrameShape, SelectedData, true);
      } else {
        float_database_.StoreToDb(GroupName, VarName, FrameShape, FrameData, true);
      }
    }

    // String variables
    for (IodaIO::FrameStringIter idata = fileio->frame_string_begin();
                                 idata != fileio->frame_string_end(); ++idata) {
      std::string GroupName = fileio->frame_string_get_gname(idata);
      if (FirstFrame && (GroupName == "ObsValue")) { nvars_++; }
      std::string VarName = fileio->frame_string_get_vname(idata);
      std::vector<std::size_t> VarShape = fileio->var_shape(GroupName, VarName);
      std::vector<std::string> FrameData;
      fileio->frame_string_get_data(GroupName, VarName, FrameData);
      std::vector<std::size_t> FrameShape = VarShape;
      FrameShape[0] = FrameData.size();
      if (VarShape[0] == gnlocs_) {
        std::vector<std::size_t> IndexedShape;
        std::vector<std::string> SelectedData =
             ApplyIndex(FrameData, VarShape, FrameIndex, FrameShape);
        if (VarName == "datetime") {
          // Convert to DateTime objects and store in datetime database
          std::vector<util::DateTime> DtData;
          for (std::size_t i = 0; i < SelectedData.size(); ++i) {
            util::DateTime ObsDt(SelectedData[i]);
            DtData.push_back(ObsDt);
          }
          datetime_database_.StoreToDb(GroupName, VarName, FrameShape, DtData, true);
        } else {
          string_database_.StoreToDb(GroupName, VarName, FrameShape, SelectedData, true);
        }
      } else {
        string_database_.StoreToDb(GroupName, VarName, FrameShape, FrameData, true);
      }
    }
    FirstFrame = false;
  }
  fileio->frame_finalize();

  // Record whether any problems occurred when reading the file.
  file_unexpected_dtypes_ = fileio->unexpected_data_types();
  file_excess_dims_ = fileio->excess_dims();
  oops::Log::trace() << "ObsData::InitFromFile opening file ends " << std::endl;
}

// -----------------------------------------------------------------------------
/*!
 * \details This method generates an list of indices with their corresponding
 *          record numbers, where the indices denote which locations are to be
 *          read into this process element.
 *
 * \param[in] FileIO File id (pointer to IodaIO object)
 * \param[in] FrameStart Row number at beginning of frame.
 * \param[out] FrameIndex Vector of indices indicating rows belonging to this process element
 * \param[out] FrameRecNums Vector containing record numbers corresponding to FrameIndex
 */
std::vector<std::size_t> ObsData::GenFrameIndexRecNums(const std::unique_ptr<IodaIO> & FileIO,
                                 const std::size_t FrameStart, const std::size_t FrameSize) {
  // It's possible that the total number of locations (gnlocs_) is smaller than
  // another dimension (eg, nchans or nvars for a hyperspectral instrument). If that
  // is the case, we don't want to read past the end of the datetime or obs group
  // variable which are dimensioned by nlocs.
  std::size_t LocSize = FrameSize;
  if ((FrameStart + FrameSize) > gnlocs_) { LocSize = gnlocs_ - FrameStart; }

  // Apply the timing window if we are reading from a file. Need to filter out locations
  // that are outside the timing window before generating record numbers. This is because
  // we are generating record numbers on the fly since we want to get to the point where
  // we can do the MPI distribution without knowing how many obs (and records) we are going
  // to encounter.
  //
  // Create two vectors as the timing windows are checked, one for location indices the
  // other for frame indices. Location indices are relative to FrameStart, and frame
  // indices are relative to this frame (start at zero).
  //
  // If we are not reading from a file, then load up the locations and frame indices
  // with all locations in the frame.
  std::vector<std::size_t> LocIndex;
  std::vector<std::size_t> FrameIndex;
  if (FileIO != nullptr) {
    // Grab the datetime strings for checking the timing window
    std::string DtGroupName = "MetaData";
    std::string DtVarName = "datetime";
    std::vector<std::string> DtStrings;
    FileIO->frame_string_get_data(DtGroupName, DtVarName, DtStrings);

    // Convert the datetime strings to DateTime objects
    std::vector<util::DateTime> ObsDtimes;
    for (std::size_t i = 0; i < DtStrings.size(); ++i) {
      util::DateTime ObsDt(DtStrings[i]);
      ObsDtimes.push_back(ObsDt);
    }

    // Keep all locations that fall inside the timing window
    for (std::size_t i = 0; i < LocSize; ++i) {
      if (InsideTimingWindow(ObsDtimes[i])) {
        LocIndex.push_back(FrameStart + i);
        FrameIndex.push_back(i);
      }
    }
    LocSize = LocIndex.size();  // in case any locations were rejected
  } else {
    // Not reading from file, keep all locations.
    LocIndex.assign(LocSize, 0);
    std::iota(LocIndex.begin(), LocIndex.end(), FrameStart);

    FrameIndex.assign(LocSize, 0);
    std::iota(FrameIndex.begin(), FrameIndex.end(), 0);
  }

  // Generate record numbers for this frame
  std::vector<std::size_t> Records(LocSize);
  if ((obs_group_variable_.empty()) || (FileIO == nullptr)) {
    // Grouping is not specified, so use the location indices as the record indicators.
    // Using the obs grouping object will make the record numbering go sequentially
    // from 0 to nrecs_ - 1.
    for (std::size_t i = 0; i < LocSize; ++i) {
      int RecValue = LocIndex[i];
      if (!int_obs_grouping_.has(RecValue)) {
        int_obs_grouping_.insert(RecValue, next_rec_num_);
        next_rec_num_++;
        nrecs_ = next_rec_num_;
      }
      Records[i] = int_obs_grouping_.at(RecValue);
    }
  } else {
    // Group according to the data in obs_group_variable_
    std::string GroupName = "MetaData";
    std::string VarName = obs_group_variable_;
    std::string VarType = FileIO->var_dtype(GroupName, VarName);

    if (VarType == "int") {
      std::vector<int> GroupVar;
      FileIO->frame_int_get_data(GroupName, VarName, GroupVar);
      for (std::size_t i = 0; i < LocSize; ++i) {
        int RecValue = GroupVar[FrameIndex[i]];
        if (!int_obs_grouping_.has(RecValue)) {
          int_obs_grouping_.insert(RecValue, next_rec_num_);
          next_rec_num_++;
          nrecs_ = next_rec_num_;
        }
        Records[i] = int_obs_grouping_.at(RecValue);
      }
    } else if (VarType == "float") {
      std::vector<float> GroupVar;
      FileIO->frame_float_get_data(GroupName, VarName, GroupVar);
      for (std::size_t i = 0; i < LocSize; ++i) {
        float RecValue = GroupVar[FrameIndex[i]];
        if (!float_obs_grouping_.has(RecValue)) {
          float_obs_grouping_.insert(RecValue, next_rec_num_);
          next_rec_num_++;
        }
        Records[i] = float_obs_grouping_.at(RecValue);
      }
    } else if (VarType == "string") {
      std::vector<std::string> GroupVar;
      FileIO->frame_string_get_data(GroupName, VarName, GroupVar);
      for (std::size_t i = 0; i < LocSize; ++i) {
        std::string RecValue = GroupVar[FrameIndex[i]];
        if (!string_obs_grouping_.has(RecValue)) {
          string_obs_grouping_.insert(RecValue, next_rec_num_);
          next_rec_num_++;
          nrecs_ = next_rec_num_;
        }
        Records[i] = string_obs_grouping_.at(RecValue);
      }
    }
  }

  // Read lat/lon for this frame
  std::vector<float> lats(LocSize, 0);
  std::vector<float> lons(LocSize, 0);
  if (FileIO != nullptr) {
    std::string GroupName = "MetaData";
    std::string GroupVar = "longitude";
    FileIO->frame_float_get_data(GroupName, GroupVar, lons);
    GroupVar = "latitude";
    FileIO->frame_float_get_data(GroupName, GroupVar, lats);
  }

  // Generate the index and recnums for this frame. We are done with FrameIndex
  // so it can be reused here.
  FrameIndex.clear();
  std::set<std::size_t> PatchRecNums;
  for (std::size_t i = 0; i < LocSize; ++i) {
    std::size_t RowNum = LocIndex[i];
    std::size_t RecNum = Records[i];
    eckit::geometry::Point2 point(lons[i], lats[i]);
    dist_->assignRecord(RecNum, RowNum, point);
    if (dist_->isMyRecord(RecNum)) {
      indx_.push_back(RowNum);
      recnums_.push_back(RecNum);
      unique_rec_nums_.insert(RecNum);
      FrameIndex.push_back(RowNum - FrameStart);
    }
  }

  nlocs_ += FrameIndex.size();
  return FrameIndex;
}

// -----------------------------------------------------------------------------
/*!
 * \details This method will return true/false according to whether the
 *          observation datetime (ObsDt) is inside the DA timing window.
 *
 * \param[in] ObsDt Observation date time object
 */
bool ObsData::InsideTimingWindow(const util::DateTime & ObsDt) {
  return ((ObsDt > winbgn_) && (ObsDt <= winend_));
}

// -----------------------------------------------------------------------------
/*!
 * \details This method will construct a data structure that holds the
 *          location order within each group sorted by the values of
 *          the specified sort variable.
 */
void ObsData::BuildSortedObsGroups() {
  typedef std::map<std::size_t, std::vector<std::pair<float, std::size_t>>> TmpRecIdxMap;
  typedef TmpRecIdxMap::iterator TmpRecIdxIter;

  // Get the sort variable from the data store, and convert to a vector of floats.
  std::vector<float> SortValues(nlocs_);
  if (obs_sort_variable_ == "datetime") {
    std::vector<util::DateTime> Dates(nlocs_);
    get_db("MetaData", obs_sort_variable_, Dates);
    for (std::size_t iloc = 0; iloc < nlocs_; iloc++) {
      SortValues[iloc] = (Dates[iloc] - Dates[0]).toSeconds();
    }
  } else {
    get_db("MetaData", obs_sort_variable_, SortValues);
  }

  // Construct a temporary structure to do the sorting, then transfer the results
  // to the data member recidx_.
  TmpRecIdxMap TmpRecIdx;
  for (size_t iloc = 0; iloc < nlocs_; iloc++) {
    TmpRecIdx[recnums_[iloc]].push_back(std::make_pair(SortValues[iloc], iloc));
  }

  for (TmpRecIdxIter irec = TmpRecIdx.begin(); irec != TmpRecIdx.end(); ++irec) {
    if (obs_sort_order_ == "ascending") {
      sort(irec->second.begin(), irec->second.end());
    } else {
      // Use a lambda function to access the std::pair greater-than operator to
      // implement a descending order sort, ensuring the associated indices remain
      // in ascending order.
      sort(irec->second.begin(), irec->second.end(),
                  [](const std::pair<float, std::size_t> & p1,
                     const std::pair<float, std::size_t> & p2){
             return (p2.first < p1.first ||
                     (!(p1.first < p2.first) && p2.second > p1.second));});
    }
  }

  // Copy indexing to the recidx_ data member.
  for (TmpRecIdxIter irec = TmpRecIdx.begin(); irec != TmpRecIdx.end(); ++irec) {
    recidx_[irec->first].resize(irec->second.size());
    for (std::size_t iloc = 0; iloc < irec->second.size(); iloc++) {
      recidx_[irec->first][iloc] = irec->second[iloc].second;
    }
  }
}

// -----------------------------------------------------------------------------
/*!
 * \details This method will save the contents of the obs container into the
 *          given file. Currently, all variables in the obs container are written
 *          into the file. This may change in the future where we can select which
 *          variables we want saved.
 *
 * \param[in] file_name Path to output obs file.
 */
void ObsData::SaveToFile(const std::string & file_name, const std::size_t MaxFrameSize) {
  // Open the file for output
  std::unique_ptr<IodaIO> fileio
    {ioda::IodaIOfactory::Create(file_name, "W", MaxFrameSize)};

  // Add dimensions for nlocs and nvars
  fileio->dim_insert("nlocs", nlocs_);
  fileio->dim_insert("nvars", nvars_);

  // Build the group, variable info container. This defines the variables
  // that will be written into the output file.
  std::size_t MaxVarSize = 0;
  for (ObsSpaceContainer<int>::VarIter ivar = int_database_.var_iter_begin();
                                       ivar != int_database_.var_iter_end(); ++ivar) {
    std::string GroupName = int_database_.var_iter_gname(ivar);
    std::string VarName = int_database_.var_iter_vname(ivar);
    std::string GrpVarName = VarName + "@" + GroupName;
    std::vector<std::size_t> VarShape = int_database_.var_iter_shape(ivar);
    if (VarShape[0] > MaxVarSize) { MaxVarSize = VarShape[0]; }
    fileio->grp_var_insert(GroupName, VarName, "int", VarShape, GrpVarName, "int");
  }
  for (ObsSpaceContainer<float>::VarIter ivar = float_database_.var_iter_begin();
                                       ivar != float_database_.var_iter_end(); ++ivar) {
    std::string GroupName = float_database_.var_iter_gname(ivar);
    std::string VarName = float_database_.var_iter_vname(ivar);
    std::string GrpVarName = VarName + "@" + GroupName;
    std::vector<std::size_t> VarShape = float_database_.var_iter_shape(ivar);
    if (VarShape[0] > MaxVarSize) { MaxVarSize = VarShape[0]; }
    fileio->grp_var_insert(GroupName, VarName, "float", VarShape, GrpVarName, "float");
  }
  for (ObsSpaceContainer<std::string>::VarIter ivar = string_database_.var_iter_begin();
                                       ivar != string_database_.var_iter_end(); ++ivar) {
    std::string GroupName = string_database_.var_iter_gname(ivar);
    std::string VarName = string_database_.var_iter_vname(ivar);
    std::string GrpVarName = VarName + "@" + GroupName;
    std::vector<std::size_t> VarShape = string_database_.var_iter_shape(ivar);
    if (VarShape[0] > MaxVarSize) { MaxVarSize = VarShape[0]; }
    std::vector<std::string> DbData(VarShape[0], "");
    string_database_.LoadFromDb(GroupName, VarName, VarShape, DbData);
    std::size_t MaxStringSize = FindMaxStringLength(DbData);
    fileio->grp_var_insert(GroupName, VarName, "string", VarShape, GrpVarName, "string",
                           MaxStringSize);
  }
  for (ObsSpaceContainer<util::DateTime>::VarIter ivar = datetime_database_.var_iter_begin();
                                       ivar != datetime_database_.var_iter_end(); ++ivar) {
    std::string GroupName = datetime_database_.var_iter_gname(ivar);
    std::string VarName = datetime_database_.var_iter_vname(ivar);
    std::string GrpVarName = VarName + "@" + GroupName;
    std::vector<std::size_t> VarShape = datetime_database_.var_iter_shape(ivar);
    if (VarShape[0] > MaxVarSize) { MaxVarSize = VarShape[0]; }
    fileio->grp_var_insert(GroupName, VarName, "string", VarShape, GrpVarName, "string", 20);
  }

  // Build the frame info container
  fileio->frame_info_init(MaxVarSize);

  // For every frame, dump out the int, float, string variables.
  for (IodaIO::FrameIter iframe = fileio->frame_begin();
                         iframe != fileio->frame_end(); ++iframe) {
    fileio->frame_data_init();
    std::size_t FrameStart = fileio->frame_start(iframe);
    std::size_t FrameSize = fileio->frame_size(iframe);

    // Integer data
    for (ObsSpaceContainer<int>::VarIter ivar = int_database_.var_iter_begin();
                                         ivar != int_database_.var_iter_end(); ++ivar) {
      std::string GroupName = int_database_.var_iter_gname(ivar);
      std::string VarName = int_database_.var_iter_vname(ivar);
      std::vector<std::size_t> VarShape = int_database_.var_iter_shape(ivar);

      if (VarShape[0] > FrameStart) {
        std::size_t Count = FrameSize;
        if ((FrameStart + FrameSize) > VarShape[0]) { Count = VarShape[0] - FrameStart; }
        std::vector<int> FrameData(Count, 0);
        int_database_.LoadFromDb(GroupName, VarName, VarShape, FrameData, FrameStart, Count);
        fileio->frame_int_put_data(GroupName, VarName, FrameData);
      }
    }

    // Float data
    for (ObsSpaceContainer<float>::VarIter ivar = float_database_.var_iter_begin();
                                         ivar != float_database_.var_iter_end(); ++ivar) {
      std::string GroupName = float_database_.var_iter_gname(ivar);
      std::string VarName = float_database_.var_iter_vname(ivar);
      std::vector<std::size_t> VarShape = float_database_.var_iter_shape(ivar);

      if (VarShape[0] > FrameStart) {
        std::size_t Count = FrameSize;
        if ((FrameStart + FrameSize) > VarShape[0]) { Count = VarShape[0] - FrameStart; }
        std::vector<float> FrameData(Count, 0.0);
        float_database_.LoadFromDb(GroupName, VarName, VarShape, FrameData, FrameStart, Count);
        fileio->frame_float_put_data(GroupName, VarName, FrameData);
      }
    }

    // String data
    for (ObsSpaceContainer<std::string>::VarIter ivar = string_database_.var_iter_begin();
                                         ivar != string_database_.var_iter_end(); ++ivar) {
      std::string GroupName = string_database_.var_iter_gname(ivar);
      std::string VarName = string_database_.var_iter_vname(ivar);
      std::vector<std::size_t> VarShape = string_database_.var_iter_shape(ivar);

      if (VarShape[0] > FrameStart) {
        std::size_t Count = FrameSize;
        if ((FrameStart + FrameSize) > VarShape[0]) { Count = VarShape[0] - FrameStart; }
        std::vector<std::string> FrameData(Count, "");
        string_database_.LoadFromDb(GroupName, VarName, VarShape, FrameData,
                                    FrameStart, Count);
        fileio->frame_string_put_data(GroupName, VarName, FrameData);
      }
    }

    for (ObsSpaceContainer<util::DateTime>::VarIter ivar = datetime_database_.var_iter_begin();
                                       ivar != datetime_database_.var_iter_end(); ++ivar) {
      std::string GroupName = datetime_database_.var_iter_gname(ivar);
      std::string VarName = datetime_database_.var_iter_vname(ivar);
      std::vector<std::size_t> VarShape = datetime_database_.var_iter_shape(ivar);

      if (VarShape[0] > FrameStart) {
        std::size_t Count = FrameSize;
        if ((FrameStart + FrameSize) > VarShape[0]) { Count = VarShape[0] - FrameStart; }
        util::DateTime TempDt("0000-01-01T00:00:00Z");
        std::vector<util::DateTime> FrameData(Count, TempDt);
        datetime_database_.LoadFromDb(GroupName, VarName, VarShape, FrameData,
                                      FrameStart, Count);

        // Convert the DateTime vector to a string vector, then save into the file.
        std::vector<std::string> StringVector(FrameData.size(), "");
        for (std::size_t i = 0; i < FrameData.size(); i++) {
          StringVector[i] = FrameData[i].toString();
        }
        fileio->frame_string_put_data(GroupName, VarName, StringVector);
      }
    }

    fileio->frame_write(iframe);
  }
}

// -----------------------------------------------------------------------------
/*!
 * \details This method applys the distribution index on data read from the input obs file.
 *          It is expected that when this method is called that the distribution index will
 *          have the process element and DA timing window effects accounted for.
 *
 * \param[in]  FullData     Vector holding data to be indexed
 * \param[in]  FullShape    Shape (dimension sizes) of FullData
 * \param[in]  Index        Index to be applied to FullData
 * \param[out] IndexedShape Shape (dimension sizes) of data after indexing
 */
template<typename VarType>
std::vector<VarType> ObsData::ApplyIndex(const std::vector<VarType> & FullData,
                              const std::vector<std::size_t> & FullShape,
                              const std::vector<std::size_t> & Index,
                              std::vector<std::size_t> & IndexedShape) const {
  std::vector<VarType> SelectedData;
  for (std::size_t i = 0; i < Index.size(); ++i) {
    std::size_t isrc = Index[i];
    SelectedData.push_back(FullData[isrc]);
  }
  IndexedShape = FullShape;
  IndexedShape[0] = SelectedData.size();
  return SelectedData;
}

// -----------------------------------------------------------------------------
/*!
 * \details This method will return the desired numeric data type for variables
 *          read from the input obs file. The rule for now is any variable
 *          in the group "PreQC" is to be an integer, and any variable that is
 *          a double is to be a float (single precision). For cases outside of this
 *          rule, the data type from the file is used.
 *
 * \param[in] GroupName   Name of obs container group
 * \param[in] FileVarType Name of the data type of the variable from the input obs file
 */
std::string ObsData::DesiredVarType(std::string & GroupName, std::string & FileVarType) {
  // By default, make the DbVarType equal to the FileVarType
  // Exceptions are:
  //   Force the group "PreQC" to an integer type.
  //   Force double to float.
  std::string DbVarType = FileVarType;

  if (GroupName == "PreQC") {
    DbVarType = "int";
  } else if (FileVarType == "double") {
    DbVarType = "float";
  }

  return DbVarType;
}

// -----------------------------------------------------------------------------
/*!
 * \details This method provides a means for printing Jo in
 *          an output stream. For now a dummy message is printed.
 */
void ObsData::printJo(const ObsVector & dy, const ObsVector & grad) {
  oops::Log::info() << "ObsData::printJo not implemented" << std::endl;
}
// -----------------------------------------------------------------------------
/*!
 * \details This method creates a private KDTree class member that can be used
 *          for searching for local obs to create an ObsSpace.
 */
void ObsData::createKDTree() {
  // Initialize KDTree class member
  kd_ = std::shared_ptr<ObsData::KDTree> ( new KDTree() );
  // Define lats,lons
  std::vector<float> lats(nlocs_);
  std::vector<float> lons(nlocs_);

  // Get latitudes and longitudes of all observations.
  this -> get_db("MetaData", "longitude", lons);
  this -> get_db("MetaData", "latitude", lats);

  // Define points list from lat/lon values
  typedef KDTree::PointType Point;
  std::vector<KDTree::Value> points;
  for (unsigned int i = 0; i < nlocs_; i++) {
    eckit::geometry::Point2 lonlat(lons[i], lats[i]);
    Point xyz = Point();
    // FIXME: get geometry from yaml, for now assume spherical Earth radius.
    atlas::util::Earth::convertSphericalToCartesian(lonlat, xyz);
    double index = static_cast<double>(i);
    KDTree::Value v(xyz, index);
    points.push_back(v);
  }

  // Create KDTree class member from points list.
  kd_->build(points.begin(), points.end());
}
// -----------------------------------------------------------------------------
/*!
 * \details This method returns the KDTree class member that can be used
 *          for searching for local obs when creating an ObsSpace.
 */
ObsData::KDTree & ObsData::getKDTree() {
  // Create the KDTree if it doesn't yet exist
  if (kd_ == NULL)
    createKDTree();

  return * kd_;
}
// -----------------------------------------------------------------------------

}  // namespace ioda
