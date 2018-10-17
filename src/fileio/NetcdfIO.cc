/*
 * (C) Copyright 2017 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#include <iostream>

#include "ncDim.h"
#include "ncVar.h"
#include "ncGroupAtt.h"

#include "oops/util/Logger.h"
#include "oops/util/abor1_cpp.h"
#include "oops/util/datetime_f.h"
#include "oops/util/Duration.h"

#include "fileio/NetcdfIO.h"

////////////////////////////////////////////////////////////////////////
// Implementation of IodaIO for netcdf.
////////////////////////////////////////////////////////////////////////

namespace ioda {
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
                   const std::size_t & Nlocs, const std::size_t & Nobs,
                   const std::size_t & Nrecs, const std::size_t & Nvars) {

  // Set the data members to the file name, file mode and provide a trace message.
  fname_ = FileName;
  fmode_ = FileMode;
  nlocs_ = Nlocs;
  nobs_  = Nobs;
  nrecs_ = Nrecs;
  nvars_ = Nvars;
  oops::Log::trace() << __func__ << " fname_: " << fname_ << " fmode_: " << fmode_ << std::endl;

  // Open the file. The fmode_ values that are recognized are:
  //    "r" - read
  //    "w" - write, disallow overriting an existing file
  //    "W" - write, allow overwriting an existing file
  if (fmode_ == "r") {
    ncfile_.reset(new netCDF::NcFile(fname_.c_str(), netCDF::NcFile::read));
    }
  else if (fmode_ == "w") {
    ncfile_.reset(new netCDF::NcFile(fname_.c_str(), netCDF::NcFile::newFile));
    }
  else if (fmode_ == "W") {
    ncfile_.reset(new netCDF::NcFile(fname_.c_str(), netCDF::NcFile::replace));
    }
  else {
    oops::Log::error() << __func__ << ": Unrecognized FileMode: " << fmode_ << std::endl;
    oops::Log::error() << __func__ << ":   Must use one of: 'r', 'w', 'W'" << std::endl;
    ABORT("Unrecognized file mode for NetcdfIO constructor");
    }

  // When in read mode, the constructor is responsible for setting
  // the data members nlocs_, nobs_, nrecs_ and nvars_.
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

  if (fmode_ == "r") {
    netCDF::NcDim NrecsDim  = ncfile_->getDim("nrecs");
    netCDF::NcDim NobsDim   = ncfile_->getDim("nobs");

    if (NrecsDim.isNull()) {
      // nrecs is not present --> old file
      netCDF::NcDim NchansDim = ncfile_->getDim("nchans");

      nobs_ = NobsDim.getSize();
      if (NchansDim.isNull()) {
        nvars_ = 1;
        }
      else {
        nvars_ = NchansDim.getSize();
        }

      nlocs_ = nobs_ / nvars_;
      nrecs_ = nlocs_;
      }
    else {
      // nrecs is present --> new file
      netCDF::NcDim NlocsDim  = ncfile_->getDim("nlocs");
      netCDF::NcDim NvarsDim  = ncfile_->getDim("nvars");

      nlocs_ = NlocsDim.getSize();
      nobs_  = NobsDim.getSize();
      nrecs_ = NrecsDim.getSize();
      nvars_ = NvarsDim.getSize();
      }
    }

  // When in write mode, create dimensions in the output file based on
  // nlocs_, nobs_, nrecs_, nvars_.
  if ((fmode_ == "W") || (fmode_ == "w")) {
    ncfile_->addDim("nlocs", nlocs_);
    ncfile_->addDim("nobs", nobs_);
    ncfile_->addDim("nrecs", nrecs_);
    ncfile_->addDim("nvars", nvars_);
    }
  }

// -----------------------------------------------------------------------------

NetcdfIO::~NetcdfIO() {
  // Netcdf destructor will close the file. No need to do the close.
  oops::Log::trace() << __func__ << " fname_: " << fname_ << std::endl;
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

void NetcdfIO::ReadVar(const std::string & VarName, int* VarData) {

  oops::Log::trace() << __func__ << " VarName: " << VarName << std::endl;

  netCDF::NcVar ncvar_ = ncfile_->getVar(VarName);
  ncvar_.getVar(VarData);
  }

// -----------------------------------------------------------------------------

void NetcdfIO::ReadVar(const std::string & VarName, float* VarData) {

  oops::Log::trace() << __func__ << " VarName: " << VarName << std::endl;

  netCDF::NcVar ncvar_ = ncfile_->getVar(VarName);
  ncvar_.getVar(VarData);
  }

// -----------------------------------------------------------------------------

void NetcdfIO::ReadVar(const std::string & VarName, double* VarData) {

  oops::Log::trace() << __func__ << " VarName: " << VarName << std::endl;

  netCDF::NcVar ncvar_ = ncfile_->getVar(VarName);
  ncvar_.getVar(VarData);
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

  netCDF::NcVar ncvar_ = ncfile_->getVar(VarName);
  if (ncvar_.isNull()) {
    // Var does not exist, so create it
    ncvar_ = ncfile_->addVar(VarName, netCDF::ncInt, ncfile_->getDim("nlocs"));
    }
  ncvar_.putVar(VarData);
  }

// -----------------------------------------------------------------------------

void NetcdfIO::WriteVar(const std::string & VarName, float* VarData) {

  oops::Log::trace() << __func__ << " VarName: " << VarName << std::endl;

  netCDF::NcVar ncvar_ = ncfile_->getVar(VarName);
  if (ncvar_.isNull()) {
    // Var does not exist, so create it
    ncvar_ = ncfile_->addVar(VarName, netCDF::ncFloat, ncfile_->getDim("nlocs"));
    }
  ncvar_.putVar(VarData);
  }

// -----------------------------------------------------------------------------

void NetcdfIO::WriteVar(const std::string & VarName, double* VarData) {

  oops::Log::trace() << __func__ << " VarName: " << VarName << std::endl;

  netCDF::NcVar ncvar_ = ncfile_->getVar(VarName);
  if (ncvar_.isNull()) {
    // Var does not exist, so create it
    ncvar_ = ncfile_->addVar(VarName, netCDF::ncDouble, ncfile_->getDim("nlocs"));
    }
  ncvar_.putVar(VarData);
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

  std::unique_ptr<float[]> OffsetTime;
  std::size_t Vsize;
  int Year;
  int Month;
  int Day;
  int Hour;
  int Minute;
  int Second;

  oops::Log::trace() << __func__ << std::endl;

  // Read in the date_time attribute which is in the form: yyyymmddhh
  // Convert the date_time to a Datetime object.
  netCDF::NcGroupAtt dtattr_ = ncfile_->getAtt("date_time");
  int dtvals_;
  util::DateTime refdt_;
  dtattr_.getValues(&dtvals_);
  datetime_setints_f(&refdt_, dtvals_/100, dtvals_%100);

  // Read in the time variable and convert to a Duration object. Time is an
  // offset from the date_time attribute. This fits in nicely with a Duration
  // object.
  netCDF::NcVar nctime_ = ncfile_->getVar("time");
  if (nctime_.isNull()) {
    nctime_ = ncfile_->getVar("Obs_Time");
    }

  Vsize = nctime_.getDim(0).getSize();
  OffsetTime.reset(new float[Vsize]);
  nctime_.getVar(OffsetTime.get());

  // Combine the refdate with the offset time, and convert to yyyymmdd and
  // hhmmss values.
  std::unique_ptr<util::DateTime> dt_(new util::DateTime[Vsize]);
  for (std::size_t i = 0; i < Vsize; ++i) {
    dt_.get()[i] = refdt_ + util::Duration(int(OffsetTime.get()[i] * 3600));
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


}  // namespace ioda
