/*
 * (C) Copyright 2018 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#include <iostream>

#include "oops/util/Logger.h"
#include "oops/util/abor1_cpp.h"
#include "oops/util/datetime_f.h"
#include "oops/util/Duration.h"

#include "fileio/OdbApiIO.h"

////////////////////////////////////////////////////////////////////////
// Implementation of IodaIO for ODB API.
////////////////////////////////////////////////////////////////////////

// This way of logging errors is taken from the ODB API C Example code. May 
// want to modify.
#define checkRC(return_code, message, db_) { \
    if (return_code != ODBQL_OK) { \
        oops::Log::error() << __func__ << ": ODB ERROR: " << message << std::endl; \
        odbql_close(db_); \
    } \
}

namespace ioda {
// -----------------------------------------------------------------------------
/*!
 * \details This constructor will open the ODB API file. If opening in read
 *          mode, the parameters nlocs, nobs, nrecs and nvars will be set
 *          by querying the size of dimensions of the same names in the input
 *          file. If opening in write mode, the parameters will be set from the
 *          same named arguements to this constructor.
 *
 * \param[in]  FileName Path to the ODB API file
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
OdbApiIO::OdbApiIO(const std::string & FileName, const std::string & FileMode,
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
  odbql_stmt *res = nullptr;
  int rc = ODBQL_OK;
  int i, column, number_of_columns;
  
  if (fmode_ == "r") {
    //int i, column, number_of_columns;
    //long long number_of_rows = 0, number_of_rows_in_current_dataset = 0;
    odbql_stmt *res;

    //Uncomment code below if _db is later changed to unique_ptr
    //odbql *temp_db; 
    //rc = odbql_open(fname_.c_str(), &temp_db);
    rc = odbql_open(fname_.c_str(), &db_);
    if (rc != ODBQL_OK) { 
      std::string errmsg = "OdbApi constructor cannot open file: " + fname_ 
        + ". Return code: " + std::to_string(rc);
      oops::Log::error() << __func__ << ": " << errmsg << std::endl; \
      ABORT(errmsg);
      }
    //db_.reset(temp_db);
    }
  else if (fmode_ == "w") {
    oops::Log::error() << __func__ << ": Unimplemented FileMode: " << fmode_ << std::endl;
    ABORT("Unimplemented file mode 'w' for OdbApiIO constructor");
    }
  else if (fmode_ == "W") {
    oops::Log::error() << __func__ << ": Unimplemented FileMode: " << fmode_ << std::endl;
    ABORT("Unimplemented file mode 'W' for OdbApiIO constructor");
    }
  else {
    oops::Log::error() << __func__ << ": Unrecognized FileMode: " << fmode_ << std::endl;
    oops::Log::error() << __func__ << ":   Must use one of: 'r', 'w', 'W'" << std::endl;
    ABORT("Unrecognized file mode for OdbApiIO constructor");
    }

  // When in read mode, the constructor is responsible for setting
  // the data members nlocs_, nobs_, nrecs_ and nvars_.
  //

  if (fmode_ == "r") {

      /* Current code is for radiosonde data only and makes the following big
         assumptions about the format of the ODB API database file:
             * one location per row
             * one variable per location 
             * nrecs_ == nlocs_
         Keeping all these assumptions is probably untenable for the long-term. */

      nlocs_ = 0;

      std::string sqlstmt = "SELECT count(*) FROM '" + fname_ + "';";
      oops::Log::trace() << __func__ << " sql statement:  " << sqlstmt << std::endl;
      
      rc = odbql_prepare_v2(db_, sqlstmt.c_str(), -1, &res, 0); 
      checkRC(rc, "Failed to prepare statement to count records.", db_); 
      //number_of_columns = odbql_column_count(res);
      //int columnType = odbql_column_type(res, 0);
      //oops::Log::trace() << __func__ << " number of columns: " << std::to_string(number_of_columns) << std::endl;
      //oops::Log::trace() << __func__ << " column type: " << std::to_string(columnType) << std::endl;

      rc = odbql_step(res); 
      //oops::Log::trace() << __func__ << " return code from odbql_step: " << std::to_string(rc) << std::endl;
      if (rc == ODBQL_ROW) {
	odbql_value* pv = odbql_column_value(res, 0);
	if (pv != 0) {
          nlocs_ = odbql_value_double(pv);
          oops::Log::trace() << __func__ << " nlocs_ set to: " << std::to_string(nlocs_) << std::endl;
	  }
	else {
	  oops::Log::error() << __func__ << " unexpected pv null. " << std::endl;
	  }
	}

      rc = odbql_finalize(res);
      checkRC(rc, "odbql_finalize failed.", db_); 
      res = nullptr;

      nrecs_ = nlocs_; //Assumption for now
      nvars_ = 1; //Hardcoded for now
      nobs_ = nlocs_ * nvars_;
    }
/*
  // When in write mode, create dimensions in the output file based on
  // nlocs_, nobs_, nrecs_, nvars_.
  if ((fmode_ == "W") || (fmode_ == "w")) {
    ncfile_->addDim("nlocs", nlocs_);
    ncfile_->addDim("nobs", nobs_);
    ncfile_->addDim("nrecs", nrecs_);
    ncfile_->addDim("nvars", nvars_);
    }
*/
  }

// -----------------------------------------------------------------------------

OdbApiIO::~OdbApiIO() {
  oops::Log::trace() << __func__ << " fname_: " << fname_ << std::endl;
  if (db_ != nullptr) {
    // Uncomment code below if we can later make db_ a unique_ptr
    //int rc = odbql_close(db_.get());
    //checkRC(rc, "odbql_close failed", db_.get());
    //db_.reset();
    int rc = odbql_close(db_);
    checkRC(rc, "odbql_close failed", db_);
    db_ = nullptr;
    }
}


// -----------------------------------------------------------------------------

/*!
 * \details This method selects the data in the column called VarName and loads 
 *          it into the returned array VarData. The caller is responsible for
 *          allocating the memory for the VarData array.
 *
 * \param[in]      VarName       Name of variable to select
 * \param[in/out]  VarData       Pointer to the array where the data is loaded.
 */
template <class T>
void OdbApiIO::ReadVarTemplate(const std::string & VarName, T* VarData) {

  oops::Log::trace() << __func__ << " VarName: " << VarName << std::endl;
  int rc;
  odbql_stmt *res = nullptr;

  std::string sql = "SELECT " + VarName + " FROM '" + fname_ + "';";
  oops::Log::trace() << __func__ << " sql: " << sql << std::endl;

  rc = odbql_prepare_v2(db_, sql.c_str(), -1, &res, 0); 
  if (rc != ODBQL_OK) {
    std::string errorString = "ODB ERROR: error when preparing SQL statemtnt: " +
       sql;
    oops::Log::error() << __func__ << ": " << errorString << std::endl;
    res = nullptr;
    ABORT(errorString); //No way to return errors to ReadVar caller, so we have to just abort.
    }

  int columnType = odbql_column_type(res, 0);
  /* if (columnType != expectedType) {
    //TODO: review if this is the desired behavior for this situation.
    std::string errorString = "WARNING: Data type for '" + VarName + "' in file " +
      fname_ + " does not match the type of the allocated memory. Requires cast.";
    oops::Log::error() << __func__ << ": " << errorString << std::endl;
    }*/
  int index = 0;
  odbql_value* pv = nullptr;

  while (((rc = odbql_step(res)) != ODBQL_DONE) && (index < nlocs_)) {
      if (rc == ODBQL_ROW) {
        pv = odbql_column_value(res, 0);
        if (pv == nullptr) {
          std::string errorString = "ODB ERROR: unexpected NULL in a column of file: " + fname_;
          oops::Log::error() << __func__ << errorString << std::endl;
          odbql_finalize(res);
          ABORT(errorString); //No way to return errors to ReadVar caller, so we have to just abort.
          } 
          /*if (RetrieveColValPtr(res, 0, &pv)) {
              VarData[index] = odbql_value_int(pv);
              index++;
            }*/
        }
      else {
        oops::Log::error() << __func__ << ": odbql_step returned unimplemented code: " << 
           std::to_string(rc) << " in file " << fname_ << std::endl;
        odbql_finalize(res);
        ABORT("Encountered unimplemented odbql_step return code.");
        }

        //put the value in the return array
        //We cast it to the right type for the array and hope the caller
        //got the type right.
      switch (columnType) {
          case ODBQL_INTEGER:
            VarData[index] = (T)odbql_value_int(pv);
            break;
          case ODBQL_FLOAT:
            //TODO: Possible overflow or truncation happening here if T is int or float
            VarData[index] = (T)odbql_value_double(pv);
            break;
          default:
            std::string errorString = "Unimplemented data type for '" + 
              VarName + "' in file " + fname_;
            oops::Log::error() << __func__ << ": " << errorString << std::endl;
            odbql_finalize(res);
            res = nullptr;
            ABORT(errorString);
            break;
        }
      index++;
    }
  oops::Log::trace() << __func__ << "finished sql: " << sql << std::endl;
    //TODO: Validate that we came to the end of the table data and filled up the array 
  rc = odbql_finalize(res);
  checkRC(rc, "odbql_finalize failed.", db_); 
  res = nullptr;
}

// -----------------------------------------------------------------------------

void OdbApiIO::ReadVar(const std::string & VarName, int* VarData) {

  oops::Log::trace() << __func__ << " VarName: " << VarName << std::endl;
  ReadVarTemplate<int>(VarName, VarData);
}

// -----------------------------------------------------------------------------

void OdbApiIO::ReadVar(const std::string & VarName, float* VarData) {

  oops::Log::trace() << __func__ << " VarName: " << VarName << std::endl;
  ReadVarTemplate<float>(VarName, VarData);
}

// -----------------------------------------------------------------------------

void OdbApiIO::ReadVar(const std::string & VarName, double* VarData) {

  oops::Log::trace() << __func__ << " VarName: " << VarName << std::endl;
  ReadVarTemplate<double>(VarName, VarData);
}

// -----------------------------------------------------------------------------

void OdbApiIO::WriteVar(const std::string & VarName, int* VarData) {

  oops::Log::trace() << __func__ << " VarName: " << VarName << std::endl;
/*
  netCDF::NcVar ncvar_ = ncfile_->getVar(VarName);
  if (ncvar_.isNull()) {
    // Var does not exist, so create it
    ncvar_ = ncfile_->addVar(VarName, netCDF::ncInt, ncfile_->getDim("nlocs"));
    }
  ncvar_.putVar(VarData);*/
}

// -----------------------------------------------------------------------------

void OdbApiIO::WriteVar(const std::string & VarName, float* VarData) {

  oops::Log::trace() << __func__ << " VarName: " << VarName << std::endl;

/*  netCDF::NcVar ncvar_ = ncfile_->getVar(VarName);
  if (ncvar_.isNull()) {
    // Var does not exist, so create it
    ncvar_ = ncfile_->addVar(VarName, netCDF::ncFloat, ncfile_->getDim("nlocs"));
    }
  ncvar_.putVar(VarData);*/
}

// -----------------------------------------------------------------------------

void OdbApiIO::WriteVar(const std::string & VarName, double* VarData) {

  oops::Log::trace() << __func__ << " VarName: " << VarName << std::endl;

  /*netCDF::NcVar ncvar_ = ncfile_->getVar(VarName);
  if (ncvar_.isNull()) {
    // Var does not exist, so create it
    ncvar_ = ncfile_->addVar(VarName, netCDF::ncDouble, ncfile_->getDim("nlocs"));
    }
  ncvar_.putVar(VarData);*/
}

// -----------------------------------------------------------------------------

void OdbApiIO::ReadDateTime(int* VarDate, int* VarTime) {

  oops::Log::trace() << __func__ << std::endl;

  // Right now we have to hard-code the names of the date/time columns.
  const std::string dateColumnName = "date@odb";
  const std::string timeColumnName = "time@odb";

  ReadVarTemplate<int>(dateColumnName, VarDate);
  ReadVarTemplate<int>(timeColumnName, VarTime);
}

// -----------------------------------------------------------------------------

void OdbApiIO::print(std::ostream & os) const {

  os << "OdbApi: In " << __FILE__ << " @ " << __LINE__ << std::endl;

}


}  // namespace ioda
