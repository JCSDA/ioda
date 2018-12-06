/*
 * (C) Copyright 2017 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#include "fileio/NetcdfIO.h"

#include <netcdf.h>

#include <iostream>
#include <memory>
#include <string>
#include <typeinfo>

#include "oops/util/abor1_cpp.h"
#include "oops/util/datetime_f.h"
#include "oops/util/Duration.h"
#include "oops/util/Logger.h"


////////////////////////////////////////////////////////////////////////
// Implementation of IodaIO for netcdf.
////////////////////////////////////////////////////////////////////////

namespace ioda {

// -----------------------------------------------------------------------------
const double NetcdfIO::missingthreshold_ = 1.0e08;
// -----------------------------------------------------------------------------
/*!
 * \details This constructor will open the netcdf file. If opening in read
 *          mode, the parameters nlocs, nobs, nrecs and nvars will be set
 *          by querying the size of dimensions of the same names in the input
 *          file. If opening in write mode, the parameters will be set from the
 *          same named arguements to this constructor.
 *
 * \param[in]  FileName Path to the netcdf file
 * \param[in]  FileMode "r" for read, "w" for overwrite to an existing file
 *                      and "W" for create and write to a new file.
 * \param[in]  Nlocs Number of unique locations in the obs data.
 * \param[in]  Nobs  Number of unique observations in the obs data.
 * \param[in]  Nrecs Number of unique records in the obs data. Records are
 *                   atomic units that will remain intact when obs are
 *                   distributed across muliple process elements. A single
 *                   radiosonde sounding would be an example.
 * \param[in]  Nvars Number of unique varibles in the obs data.
 */

NetcdfIO::NetcdfIO(const std::string & FileName, const std::string & FileMode,
                   const util::DateTime & bgn, const util::DateTime & end,
                   const double & MissingValue, const eckit::mpi::Comm & comm,
                   const std::size_t & Nlocs, const std::size_t & Nobs,
                   const std::size_t & Nrecs, const std::size_t & Nvars)
                   : IodaIO(comm) {
  int retval_;

  // Set the data members to the file name, file mode and provide a trace message.
  fname_ = FileName;
  fmode_ = FileMode;
  nlocs_ = Nlocs;
  nobs_  = Nobs;
  nrecs_ = Nrecs;
  nvars_ = Nvars;
  missingvalue_ = MissingValue;
  oops::Log::trace() << __func__ << " fname_: " << fname_ << " fmode_: " << fmode_ << std::endl;

  // Open the file. The fmode_ values that are recognized are:
  //    "r" - read
  //    "w" - write, disallow overriting an existing file
  //    "W" - write, allow overwriting an existing file
  if (fmode_ == "r") {
    retval_ = nc_open(fname_.c_str(), NC_NOWRITE, &ncid_);
  } else if (fmode_ == "w") {
    retval_ = nc_create(fname_.c_str(), NC_NOCLOBBER|NC_NETCDF4, &ncid_);
  } else if (fmode_ == "W") {
    retval_ = nc_create(fname_.c_str(), NC_CLOBBER|NC_NETCDF4, &ncid_);
  } else {
    oops::Log::error() << __func__ << ": Unrecognized FileMode: " << fmode_ << std::endl;
    oops::Log::error() << __func__ << ":   Must use one of: 'r', 'w', 'W'" << std::endl;
    ABORT("Unrecognized file mode for NetcdfIO constructor");
  }

  // Abort if open failed
  if (retval_ != NC_NOERR) {
    oops::Log::error() << __func__ << ": Unable to open file '" << fname_
                       << "' in mode: " << fmode_ << std::endl;
    ABORT("Unable to open file");
  }

  // When in read mode, the constructor is responsible for setting
  // the data members nlocs_, nobs_, nrecs_, nvars_ and varlist_.
  //
  // The old files have nobs and optionally nchans.
  //   If nchans is present, nvars = nchans
  //   If nchans is not present, nvars = 1
  //   Then:
  //     nlocs = nobs / nvars
  //
  // The new files have nlocs, nobs, nrecs, nvars.
  //
  // The way to tell if you have a new file versus and old file is that
  // only the new files have a dimension named nrecs.
  //
  // The way to collect the VALID variable names is controlled by developers.
  //

  if (fmode_ == "r") {
    // First, check what dimensions we have in the file.
    retval_ = nc_inq_dimid(ncid_, "nrecs", &nrecs_id_);
    have_nrecs_ = (retval_ == NC_NOERR);
    retval_ = nc_inq_dimid(ncid_, "nobs", &nobs_id_);
    have_nobs_ = (retval_ == NC_NOERR);
    retval_ = nc_inq_dimid(ncid_, "nlocs", &nlocs_id_);
    have_nlocs_ = (retval_ == NC_NOERR);
    retval_ = nc_inq_dimid(ncid_, "nvars", &nvars_id_);
    have_nvars_ = (retval_ == NC_NOERR);

    retval_ = nc_inq_dimid(ncid_, "nchans", &nchans_id_);
    have_nchans_ = (retval_ == NC_NOERR);

    if (have_nrecs_) {
      // nrecs is present --> new file
      nc_inq_dimlen(ncid_, nlocs_id_, &nfvlen_);
      nc_inq_dimlen(ncid_, nobs_id_,  &nobs_);
      nc_inq_dimlen(ncid_, nrecs_id_, &nrecs_);
      nc_inq_dimlen(ncid_, nvars_id_, &nvars_);
    } else {
      // nrecs is not present --> old file
      nc_inq_dimlen(ncid_, nobs_id_, &nobs_);

      if (have_nchans_) {
        nc_inq_dimlen(ncid_, nchans_id_, &nvars_);
      } else {
        nvars_ = 1;
      }

      nfvlen_ = nobs_ / nvars_;
      nrecs_ = nlocs_;
    }

    // Apply the round-robin distribution, which yields the size and indices that
    // are to be selected by this process element out of the file.
    dist_.round_robin_distribution(commMPI_, nfvlen_);

    // How many variables should be read in ?
    std::string ErrorMsg;
    int nvarsp;
    ErrorMsg = "NetcdfIO::NetcdfIO: Unable to read number of variables";
    CheckNcCall(nc_inq_nvars(ncid_, &nvarsp), ErrorMsg);

    // Walk through the variables and determine the VALID variables
    for (std::size_t varid=0; varid < nvarsp; ++varid) {
      char name[NC_MAX_NAME+1];
      int ndimsp;
      int dimids[NC_MAX_VAR_DIMS];
      nc_type rh_type;
      ErrorMsg = "NetcdfIO::NetcdfIO: Unable to read variable of varid: " + std::to_string(varid);
      CheckNcCall(nc_inq_var(ncid_, varid, name, &rh_type, &ndimsp, dimids, 0), ErrorMsg);
      // Here is how to define the VALID variables
      if ((ndimsp == 1) && (dimids[0] == nlocs_id_)) {
        std::size_t Spos;
        std::string vname{name};
        Spos = vname.find("@");
        if (Spos == vname.npos) {
          vname_group_.push_back(std::make_tuple(vname.substr(0, Spos), ""));
        } else {
          vname_group_.push_back(std::make_tuple(vname.substr(0, Spos), vname.substr(Spos+1)));
        }
      }
    }

    // Read in the reference date and time
    if (nc_inq_attid(ncid_, NC_GLOBAL, "date_time", &nc_varid_) == NC_NOERR) {
      int datetime;
      ErrorMsg = "NetcdfIO::NetcdfIO: Unable to read reference datetime from this netCDF dataset";
      CheckNcCall(nc_get_att_int(ncid_, NC_GLOBAL, "date_time", &datetime), ErrorMsg);
      datetime_setints_f(& reference_datetime_, datetime/100, datetime%100);

      // Read in time offset vector
      if ((nc_inq_varid(ncid_, "time", &nc_varid_) == NC_NOERR) ||
          (nc_inq_varid(ncid_, "time@MetaData", &nc_varid_) == NC_NOERR)) {
        std::unique_ptr<double[]> time{new double[nfvlen_]};
        ErrorMsg = "NetcdfIO::NetcdfIO: Unable to read dataset: time";
        CheckNcCall(nc_get_var_double(ncid_, nc_varid_, time.get()), ErrorMsg);

        // Calculate the date and time and filter out the obs. outside of window
        util::DateTime dt;
        for (std::size_t ii = 0; ii < nfvlen_; ++ii) {
          dt = reference_datetime_ + util::Duration(static_cast<int>(time.get()[ii] * 3600));
          if ((dt < bgn) || (dt >= end))
            dist_.erase(ii);
        }
      } else {
        oops::Log::debug() << "NetcdfIO::NetcdfIO : not found: time " << std::endl;
      }
    } else {
      oops::Log::debug() << "NetcdfIO::NetcdfIO : not found: reference date_time " << std::endl;
    }

    nlocs_ = dist_.size();
  }

  // When in write mode, create dimensions in the output file based on
  // nlocs_, nobs_, nrecs_, nvars_.
  if ((fmode_ == "W") || (fmode_ == "w")) {
    retval_ = nc_def_dim(ncid_, "nlocs", Nlocs, &nlocs_id_);
    have_nlocs_ = (retval_ == NC_NOERR);
    retval_ = nc_def_dim(ncid_, "nobs",  Nobs,  &nobs_id_);
    have_nobs_ = (retval_ == NC_NOERR);
    retval_ = nc_def_dim(ncid_, "nrecs", Nrecs, &nrecs_id_);
    have_nrecs_ = (retval_ == NC_NOERR);
    retval_ = nc_def_dim(ncid_, "nvars", Nvars, &nvars_id_);
    have_nvars_ = (retval_ == NC_NOERR);
    }
  }

// -----------------------------------------------------------------------------

NetcdfIO::~NetcdfIO() {
  oops::Log::trace() << __func__ << " fname_: " << fname_ << std::endl;

  nc_close(ncid_);
  }

// -----------------------------------------------------------------------------
/*!
 * \brief Read data from netcdf file to memory
 *
 * \details The three ReadVar methods are the same with the exception of the
 *          datatype that is being read (integer, float, double). It is the
 *          caller's responsibility to allocate memory to hold the data being
 *          read. The caller then passes a pointer to that memory for the VarData
 *          argument.
 *
 * \param[in]  VarName Name of dataset in the netcdf file
 * \param[out] VarData Pointer to memory that will receive the file data
 */

void NetcdfIO::ReadVar_any(const std::string & VarName, boost::any * VarData) {
  oops::Log::trace() << __func__ << " VarName: " << VarName << std::endl;

  std::string ErrorMsg;
  nc_type vartype;

  ErrorMsg = "NetcdfIO::ReadVar_any: Netcdf dataset not found: " + VarName;
  CheckNcCall(nc_inq_varid(ncid_, VarName.c_str(), &nc_varid_), ErrorMsg);

  CheckNcCall(nc_inq_vartype(ncid_, nc_varid_, &vartype), ErrorMsg);

  switch (vartype) {
    case NC_INT: {
      std::unique_ptr<int[]> iData{new int[nfvlen_]};
      ReadVar(VarName.c_str(), iData.get());
      for (std::size_t ii = 0; ii < dist_.distribution().size(); ++ii)
        VarData[ii] = iData.get()[dist_.distribution()[ii]];
      break;
    }
    case NC_FLOAT: {
      std::unique_ptr<float[]> rData{new float[nfvlen_]};
      ReadVar(VarName.c_str(), rData.get());
      for (std::size_t ii = 0; ii < dist_.distribution().size(); ++ii) /* Force float to double */
        VarData[ii] = static_cast<double>(rData.get()[dist_.distribution()[ii]]);
      break;
    }
    case NC_DOUBLE: {
      std::unique_ptr<double[]> dData{new double[nfvlen_]};
      ReadVar(VarName.c_str(), dData.get());
      for (std::size_t ii = 0; ii < dist_.distribution().size(); ++ii)
        VarData[ii] = dData.get()[dist_.distribution()[ii]];
      break;
    }
    default:
      oops::Log::warning() <<  "NetcdfIO::ReadVar_any: Unable to read dataset: "
                           << " VarName: " << VarName << " with NetCDF type :"
                           << vartype << std::endl;
  }
}

void NetcdfIO::ReadVar(const std::string & VarName, int* VarData) {
  oops::Log::trace() << __func__ << " VarName: " << VarName << std::endl;

  std::string ErrorMsg;

  ErrorMsg = "NetcdfIO::ReadVar: Netcdf dataset not found: " + VarName;
  CheckNcCall(nc_inq_varid(ncid_, VarName.c_str(), &nc_varid_), ErrorMsg);

  ErrorMsg = "NetcdfIO::ReadVar: Unable to read dataset: " + VarName;
  CheckNcCall(nc_get_var_int(ncid_, nc_varid_, VarData), ErrorMsg);
}

// -----------------------------------------------------------------------------

void NetcdfIO::ReadVar(const std::string & VarName, float* VarData) {
  oops::Log::trace() << __func__ << " VarName: " << VarName << std::endl;

  std::string ErrorMsg;

  ErrorMsg = "NetcdfIO::ReadVar: Netcdf dataset not found: " + VarName;
  CheckNcCall(nc_inq_varid(ncid_, VarName.c_str(), &nc_varid_), ErrorMsg);

  ErrorMsg = "NetcdfIO::ReadVar: Unable to read dataset: " + VarName;
  CheckNcCall(nc_get_var_float(ncid_, nc_varid_, VarData), ErrorMsg);
}

// -----------------------------------------------------------------------------

void NetcdfIO::ReadVar(const std::string & VarName, double* VarData) {
  oops::Log::trace() << __func__ << " VarName: " << VarName << std::endl;

  std::string ErrorMsg;

  ErrorMsg = "NetcdfIO::ReadVar: Netcdf dataset not found: " + VarName;
  CheckNcCall(nc_inq_varid(ncid_, VarName.c_str(), &nc_varid_), ErrorMsg);

  ErrorMsg = "NetcdfIO::ReadVar: Unable to read dataset: " + VarName;
  CheckNcCall(nc_get_var_double(ncid_, nc_varid_, VarData), ErrorMsg);
}

// -----------------------------------------------------------------------------
/*!
 * \brief Write data from memory to netcdf file
 *
 * \details The three WriteVar methods are the same with the exception of the
 *          datatype that is being written (integer, float, double). It is the
 *          caller's responsibility to allocate and assign memory to the data
 *          that are to be written. The caller then passes a pointer to that
 *          memory for the VarData argument.
 *
 * \param[in]  VarName Name of dataset in the netcdf file
 * \param[in]  VarData Pointer to memory that will be written into the file
 */

void NetcdfIO::WriteVar(const std::string & VarName, int* VarData) {
  oops::Log::trace() << __func__ << " VarName: " << VarName << std::endl;

  std::string ErrorMsg;

  if (nc_inq_varid(ncid_, VarName.c_str(), &nc_varid_) != NC_NOERR) {
    // Var does not exist, so create it
    ErrorMsg = "NetcdfIO::WriteVar: Unable to create variable dataset: " + VarName;
    CheckNcCall(nc_def_var(ncid_, VarName.c_str(), NC_FLOAT, 1, &nlocs_id_, &nc_varid_), ErrorMsg);
  }

  ErrorMsg = "NetcdfIO::WriteVar: Unable to write dataset: " + VarName;
  CheckNcCall(nc_put_var_int(ncid_, nc_varid_, VarData), ErrorMsg);
  }

// -----------------------------------------------------------------------------

void NetcdfIO::WriteVar(const std::string & VarName, float* VarData) {
  oops::Log::trace() << __func__ << " VarName: " << VarName << std::endl;

  std::string ErrorMsg;

  if (nc_inq_varid(ncid_, VarName.c_str(), &nc_varid_) != NC_NOERR) {
    // Var does not exist, so create it
    ErrorMsg = "NetcdfIO::WriteVar: Unable to create variable dataset: " + VarName;
    CheckNcCall(nc_def_var(ncid_, VarName.c_str(), NC_FLOAT, 1, &nlocs_id_, &nc_varid_), ErrorMsg);
  }

  ErrorMsg = "NetcdfIO::WriteVar: Unable to write dataset: " + VarName;
  CheckNcCall(nc_put_var_float(ncid_, nc_varid_, VarData), ErrorMsg);
  }

// -----------------------------------------------------------------------------

void NetcdfIO::WriteVar(const std::string & VarName, double* VarData) {
  oops::Log::trace() << __func__ << " VarName: " << VarName << std::endl;

  std::string ErrorMsg;

  if (nc_inq_varid(ncid_, VarName.c_str(), &nc_varid_) != NC_NOERR) {
    // Var does not exist, so create it
    ErrorMsg = "NetcdfIO::WriteVar: Unable to create variable dataset: " + VarName;
    CheckNcCall(nc_def_var(ncid_, VarName.c_str(), NC_FLOAT, 1, &nlocs_id_, &nc_varid_), ErrorMsg);
  }

  ErrorMsg = "NetcdfIO::WriteVar: Unable to write dataset: " + VarName;
  CheckNcCall(nc_put_var_double(ncid_, nc_varid_, VarData), ErrorMsg);
  }

// -----------------------------------------------------------------------------
/*!
 * \brief Read and format the date, time values
 *
 * \details This method will read in the date and time information (timestamp)
 *          from the netcdf file, and convert them into a convenient format for
 *          usage by the JEDI system. Currently, the netcdf files contain an
 *          attribute called "date_time" that holds the analysis time for
 *          the obs data in the format yyyymmddhh. For example April 15, 2018
 *          at 00Z is recorded as 2018041500. The netcdf file also contains
 *          a time variable (float) which is the offset from the date_time
 *          value in hours. This method will convert the date time information to two
 *          integer vectors. The first is the date (yyyymmdd) and the second
 *          is the time (hhmmss). With the above date_time example combined
 *          with a time value of -3.5 (hours), the resulting date and time entries
 *          in the output vectors will be date = 20180414 and time = 233000.
 *
 *          Eventually, the yyyymmdd and hhmmss values can be recorded in the
 *          netcdf file as thier own datasets and this method could be removed.
 *
 * \param[out] VarDate Date portion of the timestamp values (yyyymmdd)
 * \param[out] VarTime Time portion of the timestamp values (hhmmss)
 */

void NetcdfIO::ReadDateTime(int* VarDate, int* VarTime) {
  int Year;
  int Month;
  int Day;
  int Hour;
  int Minute;
  int Second;

  std::string ErrorMsg;

  oops::Log::trace() << __func__ << std::endl;

  // Read in the date_time attribute which is in the form: yyyymmddhh
  // Convert the date_time to a Datetime object.
  int dtvals_;
  ErrorMsg = "NetcdfIO::ReadDateTime: Unable to read attribute: date_time";
  CheckNcCall(nc_get_att_int(ncid_, NC_GLOBAL, "date_time", &dtvals_), ErrorMsg);

  util::DateTime refdt_;
  datetime_setints_f(&refdt_, dtvals_/100, dtvals_%100);

  // Read in the time variable and convert to a Duration object. Time is an
  // offset from the date_time attribute. This fits in nicely with a Duration
  // object.
  // Look for "time" and "Obs_Time" for the time variable.
  if (nc_inq_varid(ncid_, "time", &nc_varid_) != NC_NOERR) {
    ErrorMsg = "NetcdfIO::ReadDateTime: Unable to find time variable: time OR Obs_Time";
    CheckNcCall(nc_inq_varid(ncid_, "time@MetaData", &nc_varid_), ErrorMsg);
  }

  int dimid_;
  std::unique_ptr<float[]> OffsetTime;
  std::size_t vsize_;

  ErrorMsg = "NetcdfIO::ReadDateTime: Unable to find dimension of time variable";
  CheckNcCall(nc_inq_vardimid(ncid_, nc_varid_, &dimid_), ErrorMsg);

  ErrorMsg = "NetcdfIO::ReadDateTime: Unable to find size of dimension of time variable";
  CheckNcCall(nc_inq_dimlen(ncid_, dimid_, &vsize_), ErrorMsg);

  OffsetTime.reset(new float[vsize_]);
  ErrorMsg = "NetcdfIO::ReadDateTime: Unable to read time variable: ";
  CheckNcCall(nc_get_var_float(ncid_, nc_varid_, OffsetTime.get()), ErrorMsg);

  // Combine the refdate with the offset time, and convert to yyyymmdd and
  // hhmmss values.
  std::unique_ptr<util::DateTime> dt_(new util::DateTime[vsize_]);
  for (std::size_t i = 0; i < vsize_; ++i) {
    dt_.get()[i] = refdt_ + util::Duration(static_cast<int>(OffsetTime.get()[i] * 3600));
    dt_.get()[i].toYYYYMMDDhhmmss(Year, Month, Day, Hour, Minute, Second);

    VarDate[i] = Year*10000 + Month*100 + Day;
    VarTime[i] = Hour*10000 + Minute*100 + Second;
    }
  }

// -----------------------------------------------------------------------------
/*!
 * \brief print method for stream output
 *
 * \details This method is supplied for the Printable base class. It defines
 *          how to print an object of this class in an output stream.
 */

void NetcdfIO::print(std::ostream & os) const {
  os << "Netcdf: In " << __FILE__ << " @ " << __LINE__ << std::endl;
  }

// -----------------------------------------------------------------------------
/*!
 * \brief check results of netcdf call
 *
 * \details This method will check the return code from a netcdf API call. 
 *          Successful completion of the call is indicated by the return
 *          code being equal to NC_NOERR. If the call was not successful,
 *          then the error message is written to the OOPS log, and is also
 *          sent to the OOPS ABORT call (execution is aborted).
 *
 * \param[in] RetCode Return code from netcdf call
 * \param[in] ErrorMsg Message for the OOPS error logger
 */

void NetcdfIO::CheckNcCall(int RetCode, std::string & ErrorMsg) {
  if (RetCode != NC_NOERR) {
    oops::Log::error() << ErrorMsg << " (" << RetCode << ")" << std::endl;
    ABORT(ErrorMsg);
  }
}

}  // namespace ioda
