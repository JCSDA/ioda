/*
 * (C) Copyright 2017-2019 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#include "fileio/NetcdfIO.h"

#include <netcdf.h>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <typeinfo>

#include "distribution/DistributionFactory.h"
#include "oops/parallel/mpi/mpi.h"
#include "oops/util/abor1_cpp.h"
#include "oops/util/DateTime.h"
#include "oops/util/Duration.h"
#include "oops/util/Logger.h"
#include "oops/util/missingValues.h"

////////////////////////////////////////////////////////////////////////
// Implementation of IodaIO for netcdf.
////////////////////////////////////////////////////////////////////////

namespace ioda {

// -----------------------------------------------------------------------------
static const double missingthreshold = 1.0e08;
// -----------------------------------------------------------------------------
/*!
 * \details This constructor will open the netcdf file. If opening in read
 *          mode, the parameters nlocs, nrecs and nvars will be set
 *          by querying the size of dimensions of the same names in the input
 *          file. If opening in write mode, the parameters will be set from the
 *          same named arguements to this constructor.
 *
 * \param[in]  FileName Path to the netcdf file
 * \param[in]  FileMode "r" for read, "w" for overwrite to an existing file
 *                      and "W" for create and write to a new file.
 * \param[in]  Nlocs Number of unique locations in the obs data.
 * \param[in]  Nrecs Number of unique records in the obs data. Records are
 *                   atomic units that will remain intact when obs are
 *                   distributed across muliple process elements. A single
 *                   radiosonde sounding would be an example.
 * \param[in]  Nvars Number of unique varibles in the obs data.
 */

NetcdfIO::NetcdfIO(const std::string & FileName, const std::string & FileMode,
                   const std::size_t & Nlocs, const std::size_t & Nrecs,
                   const std::size_t & Nvars) {
  int retval_;
  std::string ErrorMsg;

  // Set the data members to the file name, file mode and provide a trace message.
  fname_ = FileName;
  fmode_ = FileMode;
  nlocs_ = Nlocs;
  nrecs_ = Nrecs;
  nvars_ = Nvars;
  have_offset_time_ = false;
  have_date_time_ = false;
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
    oops::Log::error() << __func__ << ": Must use one of: 'r', 'w', 'W'" << std::endl;
    ABORT("Unrecognized file mode for NetcdfIO constructor");
  }

  // Abort if open failed
  if (retval_ != NC_NOERR) {
    oops::Log::error() << __func__ << ": Unable to open file '" << fname_
                       << "' in mode: " << fmode_ << std::endl;
    ABORT("Unable to open file");
  }

  // When in read mode, the constructor is responsible for setting
  // the data members nlocs_, nrecs_, nvars_ and grp_var_info_.
  //
  // The files have nlocs, nrecs, nvars.
  //
  // The way to collect the VALID variable names is controlled by developers.
  //

  if (fmode_ == "r") {
    // Find counts of objects in the file
    int NcNdims;
    int NcNvars;
    int NcNatts;
    ErrorMsg = "NetcdfIO::NetcdfIO: Unable to read file object counts";
    CheckNcCall(nc_inq(ncid_, &NcNdims, &NcNvars, &NcNatts, NULL), ErrorMsg);

    // Record the dimension sizes in the dim_id_to_size_ container.
    // Save nlocs, nrecs and nvars in data members.
    for (std::size_t i = 0; i < NcNdims; i++) {
      char NcName[MAX_NC_NAME];
      std::size_t NcSize;
      ErrorMsg = "NetcdfIO::NetcdfIO: Unable to read dimension number: " + std::to_string(i);
      CheckNcCall(nc_inq_dim(ncid_, i, NcName, &NcSize), ErrorMsg);
      dim_id_to_size_.push_back(NcSize);

      if (strcmp(NcName, "nlocs") == 0) {
        nlocs_ = NcSize;
      } else if (strcmp(NcName, "nrecs") == 0) {
        nrecs_ = NcSize;
      } else if (strcmp(NcName, "nvars") == 0) {
        nvars_ = NcSize;
      }
    }

    // Walk through the variables and record the group and variable information. For
    // now, want to support both datetime strings and ref, offset time so that
    // we can incrementally update the files to datetime strings. Accomplish this
    // by making sure that only the datetime variable appears in the grp_var_info_
    // map.
    //
    //    offset time is variable "time@MetaData"
    //    datetime string is variable "datetime@MetaData"
    //
    //    offset time           datetime         grp_var_info_     read action
    //    in the file           in the file       entry
    //
    //        N                     N             nothing            nothing
    //        N                     Y             datetime          read directly into var
    //        Y                     N             datetime          convert ref, offset
    //                                                               to datetime string
    //        Y                     Y             datetime          read directly into var

    int NcVarId;
    have_date_time_ = (nc_inq_varid(ncid_, "datetime@MetaData", &NcVarId) == NC_NOERR);
    have_offset_time_ = (nc_inq_varid(ncid_, "time@MetaData", &NcVarId) == NC_NOERR);

    for (std::size_t ivar=0; ivar < NcNvars; ++ivar) {
      // nc variable dimension and type information
      char NcVname[NC_MAX_NAME+1];
      nc_type NcDtype;
      int NcNdims;
      int NcDimIds[NC_MAX_VAR_DIMS];
      ErrorMsg = "NetcdfIO::NetcdfIO: Unable to read information for variable number: " +
                  std::to_string(ivar);
      CheckNcCall(nc_inq_var(ncid_, ivar, NcVname, &NcDtype, &NcNdims, NcDimIds, 0), ErrorMsg);
      CheckNcCall(nc_inq_varid(ncid_, NcVname, &NcVarId), ErrorMsg);

      // nc type name
      char NcDtypeName[NC_MAX_NAME+1];
      std::size_t NcDtypeSize;
      ErrorMsg = "NetcdfIO::NetcdfIO: Unable to look up type name";
      CheckNcCall(nc_inq_type(ncid_, NcDtype, NcDtypeName, &NcDtypeSize), ErrorMsg);

      // Collect the sizes for dimensions from the dim_id_to_size_ container.
      std::vector<std::size_t> NcDimSizes;
      for (std::size_t j = 0; j < NcNdims; j++) {
          NcDimSizes.push_back(dim_id_to_size_[NcDimIds[j]]);
      }

      // Record the variable info in the grp_var_into_ container.
      int OffsetTimeVarId;
      std::string VarName{NcVname};
      std::string GroupName{"GroupUndefined"};
      std::size_t Spos = VarName.find("@");
      if (Spos != VarName.npos) {
        GroupName = VarName.substr(Spos+1);
        VarName = VarName.substr(0, Spos);
      }

      // If offset time exists, substitute the datetime specs for the offset time specs.
      if (VarName.compare("time") == 0) {
        // If we have datetime in the file, just let those specs get entered when
        // datetime is encountered. Otherwise, replace the offset time specs
        // with the expected datetime specs. The reader later on will do the
        // conversion.
        if (!have_date_time_) {
          // Replace offset time with datetime specs. We want the offset time
          // variable id, but with the character array specs for after the
          // conversion.
          NcDimSizes.push_back(20);  // datetime strings are 20 character long
          grp_var_info_[GroupName]["datetime"].var_id = NcVarId;
          grp_var_info_[GroupName]["datetime"].dtype = "char";
          grp_var_info_[GroupName]["datetime"].shape = NcDimSizes;
        }
      } else {
        // enter var specs into grp_var_info_ map
        grp_var_info_[GroupName][VarName].var_id = NcVarId;
        grp_var_info_[GroupName][VarName].dtype = NcDtypeName;
        grp_var_info_[GroupName][VarName].shape = NcDimSizes;
      }
    }
  }

  // When in write mode, create dimensions in the output file based on
  // nlocs_, nrecs_, nvars_. Record the names and sizes for these
  // dimensions.
  if ((fmode_ == "W") || (fmode_ == "w")) {
    CreateNcDim("nlocs", Nlocs);
    CreateNcDim("nrecs", Nrecs);
    CreateNcDim("nvars", Nvars);
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
 * \details The four ReadVar methods are the same with the exception of the
 *          datatype that is being read (integer, float, double, char). It is the
 *          caller's responsibility to allocate memory to hold the data being
 *          read. The caller then passes a pointer to that memory for the VarData
 *          argument.
 *
 * \param[in]  GroupName Name of ObsSpace group (ObsValue, ObsError, MetaData, etc.)
 * \param[in]  VarName Name of ObsSpace variable
 * \param[in]  VarShape Dimension sizes of variable
 * \param[out] VarData Pointer to memory that will receive the file data
 */

void NetcdfIO::ReadVar(const std::string & GroupName, const std::string & VarName,
                       const std::vector<std::size_t> & VarShape, int * VarData) {
  ReadVar_helper<int>(GroupName, VarName, VarShape, VarData);
}

void NetcdfIO::ReadVar(const std::string & GroupName, const std::string & VarName,
                       const std::vector<std::size_t> & VarShape, float * VarData) {
  ReadVar_helper<float>(GroupName, VarName, VarShape, VarData);
}

void NetcdfIO::ReadVar(const std::string & GroupName, const std::string & VarName,
                       const std::vector<std::size_t> & VarShape, double * VarData) {
  ReadVar_helper<double>(GroupName, VarName, VarShape, VarData);
}

void NetcdfIO::ReadVar(const std::string & GroupName, const std::string & VarName,
                       const std::vector<std::size_t> & VarShape, char * VarData) {
  ReadVar_helper<char>(GroupName, VarName, VarShape, VarData);
}

/*!
 * \brief Helper method for ReadVar
 *
 * \details This method fills in the code for the four overloaded ReadVar functions. 
 *          This method handles calling the proper netcdf get_var method, and the
 *          replacement of netcdf missing data marks with the IODA missing data marks.
 *
 * \param[in]  GroupName Name of ObsSpace group (ObsValue, ObsError, MetaData, etc.)
 * \param[in]  VarName Name of ObsSpace variable
 * \param[in]  VarShape Dimension sizes of variable
 * \param[out] VarData Pointer to memory that will receive the file data
 */

template <typename DataType>
void NetcdfIO::ReadVar_helper(const std::string & GroupName, const std::string & VarName,
                              const std::vector<std::size_t> & VarShape, DataType * VarData) {
  std::string NcVarName = FormNcVarName(GroupName, VarName);
  std::string NcVarType = var_dtype(GroupName, VarName);

  // Read in the variable values. The netcdf interface has a generic get var (nc_get_var)
  // routine, but calling this interface causes a crash to occur. There also exist type
  // specific get var routines (nc_get_var_int, nc_get_var_float, etc.). These work okay,
  // but the compile fails when VarData comes in as a int pointer, and you give VarData to the
  // nc_get_var_float routine which expects a float pointer. This situation is resolved
  // by putting in casts to the expected pointer type.
  //
  // Normally, forcing the pointer to be the expected type is a dangerous practice,
  // but in this case it is controlled well. The if statment below ensures that the
  // casting is appropriate for the given type that is being read. For example, checking
  // that VarType is "int" only matches in the case where VarData is an int *.

  const std::type_info & VarType = typeid(DataType);  // this matches type of VarData
  int NcVarId = var_id(GroupName, VarName);
  std::string ErrorMsg = "NetcdfIO::ReadVar: Unable to read netcdf variable: " + NcVarName;
  if (VarType == typeid(int)) {
    CheckNcCall(nc_get_var_int(ncid_, NcVarId, reinterpret_cast<int *>(VarData)), ErrorMsg);
  } else if (VarType == typeid(float)) {
    CheckNcCall(nc_get_var_float(ncid_, NcVarId, reinterpret_cast<float *>(VarData)), ErrorMsg);
  } else if (VarType == typeid(double)) {
    CheckNcCall(nc_get_var_double(ncid_, NcVarId, reinterpret_cast<double *>(VarData)),
                ErrorMsg);
  } else if (VarType == typeid(char)) {
    // If reading in datetime, then need to check if we need to convert ref, offset form
    // to datetime strings. If we got here, we either have datetime in the file or
    // we've got offset time in the file.
    if (VarName.compare("datetime") == 0) {
      if (have_date_time_) {
        // datetime exists in the file, simply read it it.
        CheckNcCall(nc_get_var_text(ncid_, NcVarId, reinterpret_cast<char *>(VarData)), ErrorMsg);
      } else {
        // datetime does not exist in the file, read in offset time and convert to
        // datetime strings.
        ReadConvertDateTime(GroupName, VarName, reinterpret_cast<char *>(VarData));
      }
    } else {
      // All other variables beside datetime
      CheckNcCall(nc_get_var_text(ncid_, NcVarId, reinterpret_cast<char *>(VarData)), ErrorMsg);
    }
  } else {
    oops::Log::warning() <<  "NetcdfIO::ReadVar: Unable to read dataset: "
                         << " VarName: " << NcVarName << " with NetCDF type :"
                         << NcVarType << std::endl;
  }

  // Add in the missing data marks.
  std::size_t VarSize = 1;
  for (std::size_t i = 0; i < VarShape.size(); i++) {
    VarSize *= VarShape[i];
  }
  if ((VarType == typeid(int)) || (VarType == typeid(float))) {
    const DataType missing_value = util::missingValue(missing_value);
    for (std::size_t i = 0; i < VarSize; i++) {
      // For now use a large number as an indicator of a missing value. This is not
      // as safe as it should be. In the future, use the netcdf default fill value
      // as the missing value indicator.
      //
      // The fabs() function will convert integers and floats to double, then
      // take the absolute value, then return the double result.
      if (fabs(VarData[i]) > missingthreshold) {
        VarData[i] = missing_value;
      }
    }
  }
}

// -----------------------------------------------------------------------------
/*!
 * \brief Write data from memory to netcdf file
 *
 * \details The three WriteVar methods are the same with the exception of the
 *          datatype that is being written (integer, float, char). It is the
 *          caller's responsibility to allocate and assign memory to the data
 *          that are to be written. The caller then passes a pointer to that
 *          memory for the VarData argument.
 *
 * \param[in]  GroupName Name of ObsSpace group (ObsValue, ObsError, MetaData, etc.)
 * \param[in]  VarName Name of ObsSpace variable
 * \param[in]  VarShape Dimension sizes of variable
 * \param[in]  VarData Pointer to memory that will be written into the file
 */

void NetcdfIO::WriteVar(const std::string & GroupName, const std::string & VarName,
                        const std::vector<std::size_t> & VarShape, int * VarData) {
  WriteVar_helper<int>(GroupName, VarName, VarShape, VarData);
}

void NetcdfIO::WriteVar(const std::string & GroupName, const std::string & VarName,
                        const std::vector<std::size_t> & VarShape, float * VarData) {
  WriteVar_helper<float>(GroupName, VarName, VarShape, VarData);
}

void NetcdfIO::WriteVar(const std::string & GroupName, const std::string & VarName,
                        const std::vector<std::size_t> & VarShape, char * VarData) {
  WriteVar_helper<char>(GroupName, VarName, VarShape, VarData);
}

/*!
 * \brief Helper method for WriteVar
 *
 * \details This method fills in the code for the four overloaded WriteVar functions. 
 *          This method handles calling the proper netcdf put_var method, and the
 *          creation of a netcdf variable in the case that the requested variable
 *          does not exist in the output file.
 *
 * \param[in]  GroupName Name of ObsSpace group (ObsValue, ObsError, MetaData, etc.)
 * \param[in]  VarName Name of ObsSpace variable
 * \param[in]  VarShape Dimension sizes of variable
 * \param[out] VarData Pointer to memory that will receive the file data
 */

template <typename DataType>
void NetcdfIO::WriteVar_helper(const std::string & GroupName, const std::string & VarName,
                               const std::vector<std::size_t> & VarShape, DataType * VarData) {
  std::string NcVarName = FormNcVarName(GroupName, VarName);
  const std::type_info & VarType = typeid(DataType);

  // For now and in order to keep the IodaIO class file type agnostic, infer the
  // dimensions from GroupName and VarShape. This is not a great way to
  // do this, so we may need to remove the IodaIO class and expose the Netcdf
  // and other classes.
  //
  //  GroupName   1st dimension
  //
  //  MetaData      nlocs
  //  VarMetaData   nvars
  //  RecMetaData   nrecs
  //  all others    nlocs
  //
  // If DataType is char, then assume you have a 2D character array where the
  // second dimension is the string lengths.
  //
  // Assume only vectors for now meaning that int and float are just 1D arrays.

  // Form the dimension id list.
  std::vector<int> NcDimIds;
  if (GroupName.compare("MetaData") == 0) {
    NcDimIds.push_back(dim_name_to_id_["nlocs"]);
  } else if (GroupName.compare("VarMetaData") == 0) {
    NcDimIds.push_back(dim_name_to_id_["nvars"]);
  } else if (GroupName.compare("RecMetaData") == 0) {
    NcDimIds.push_back(dim_name_to_id_["nrecs"]);
  } else {
    NcDimIds.push_back(dim_name_to_id_["nlocs"]);
  }

  if (VarType == typeid(char)) {
    NcDimIds.push_back(GetStringDimBySize(VarShape[1]));
  }

  // Limit types to int, float and char
  nc_type NcVarType;
  if (VarType == typeid(int)) {
    NcVarType = NC_INT;
  } else if (VarType == typeid(float)) {
    NcVarType = NC_FLOAT;
  } else if (VarType == typeid(char)) {
    NcVarType = NC_CHAR;
  } else {
    oops::Log::warning() <<  "NetcdfIO::WriteVar: Unable to write dataset: "
                         << " VarName: " << NcVarName << " with data type :"
                         << VarType.name() << std::endl;
  }

  // If var doesn't exist in the file, then create it.
  std::string ErrorMsg;
  int NcVarId;
  if (nc_inq_varid(ncid_, NcVarName.c_str(), &NcVarId) != NC_NOERR) {
    ErrorMsg = "NetcdfIO::WriteVar: Unable to create variable dataset: " + NcVarName;
    CheckNcCall(nc_def_var(ncid_, NcVarName.c_str(), NcVarType, NcDimIds.size(),
                           NcDimIds.data(), &NcVarId),
                ErrorMsg);
  }

  // Write the data into the file according to type
  ErrorMsg = "NetcdfIO::WriteVar: Unable to write dataset: " + NcVarName;
  if (VarType == typeid(int)) {
    CheckNcCall(nc_put_var_int(ncid_, NcVarId, reinterpret_cast<int *>(VarData)), ErrorMsg);
  } else if (VarType == typeid(float)) {
    CheckNcCall(nc_put_var_float(ncid_, NcVarId, reinterpret_cast<float *>(VarData)), ErrorMsg);
  } else if (VarType == typeid(char)) {
    CheckNcCall(nc_put_var_text(ncid_, NcVarId, reinterpret_cast<char *>(VarData)), ErrorMsg);
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

// -----------------------------------------------------------------------------
/*!
 * \brief form the netcdf variable name
 *
 * \details This method will construct the name of the variable in the netcdf
 *           file from the given GroupName and VarName arguments. The netcdf
 *           variable name is typically "VarName@GroupName". When GroupName
 *           is "GroupUndefined", then the netcdf variable name is simply set
 *           equal to VarName (no @GroupName suffix).
 *
 * \param[in] GroupName Name of group in ObsSpace database
 * \param[in] VarName Name of variable in ObsSpace database
 */

std::string NetcdfIO::FormNcVarName(const std::string & GroupName, const std::string & VarName) {
  // Construct the variable name found in the file. If group name is "GroupUndefined",
  // then the file variable name does not include the "@GroupName" suffix.
  std::string NcVarName;
  if (GroupName.compare("GroupUndefined") == 0) {
    // No suffix in file variable name
    NcVarName = VarName;
  } else {
    // File variable has suffix with group name
    NcVarName = VarName + "@" + GroupName;
  }

  return NcVarName;
}

// -----------------------------------------------------------------------------
/*!
 * \brief create a dimension in the netcdf file
 *
 * \details This method will create a dimension in the output netcdf file using
 *          the name given by DimName and the size given by DimSize. This method
 *          also records the dimension name and size for downstream use in the
 *          WriteVar methods.
 *
 * \param[in] DimName Name of netcdf dimension
 * \param[in] DimSize Size of netcdf dimension
 */

void NetcdfIO::CreateNcDim(const std::string DimName, const std::size_t DimSize) {
  int NcDimId;
  std::string ErrorMsg = "NetcdfIO::NetcdfIO: Unable to create dimension: " + DimName;
  CheckNcCall(nc_def_dim(ncid_, DimName.c_str(), DimSize, &NcDimId), ErrorMsg);
  dim_name_to_id_[DimName] = NcDimId;
}

// -----------------------------------------------------------------------------
/*!
 * \brief allocate a dimension for writing a character array
 *
 * \details This method is used for setting up dimensions for a writing a 
 *          character array in the output netcdf file. A character array is
 *          how a vector of strings is represented in netcdf. In order to
 *          minimize storage, this method is part of a scheme to always create
 *          the smallest character array necessary (the first dimension matches
 *          the size of the string vector, the second dimension matches the
 *          maximum string size in that vector). First, the existing dimensions
 *          that have already been allocated for character arrays are checked
 *          and if a match occurs that dimension id is returned. Otherwise, a
 *          new dimension of the size DimSize is created in the output netcdf file
 *          and that new dimension id is returned. New dimensions are named
 *          "nstringN" where N is set to DimSize.
 *
 * \param[in] DimSize Size of netcdf dimension
 */

int NetcdfIO::GetStringDimBySize(const std::size_t DimSize) {
  // Form the name of the dimension by appending DimSize on the end of
  // the string "nstring".
  std::string DimName = "nstring" + std::to_string(DimSize);

  // Look for this dimension in the string dims map
  int DimId;
  DimNameToIdType::iterator istr = string_dims_.find(DimName);
  if (istr == string_dims_.end()) {
    // Not found so create the dimension and get the id
    std::string ErrorMsg = "NetcdfIO::NetcdfIO: Unable to create dimension: " + DimName;
    CheckNcCall(nc_def_dim(ncid_, DimName.c_str(), DimSize, &DimId), ErrorMsg);
    string_dims_[DimName] = DimId;
  } else {
    DimId = istr->second;
  }

  return DimId;
}

// -----------------------------------------------------------------------------
/*!
 * \brief read date and time information from the input netcdf file
 *
 * \details This method will read date and time information from the input netcdf
 *          file and convert that information to absolute date time strings in the
 *          ISO 8601 format. The date and time information in the input file is
 *          represented as an attribute called "date_time" that contains a reference
 *          date and time, and a float variable called "time@MetaData" that contains
 *          offset time values relative to the date_time attribute. The date_time attribute
 *          is an integer or string in the format, YYYYMMDDHH (year, month, day, hour).
 *          The time variable is the offest in units of hours. This is a placeholder
 *          function that will be removed once all input files have been converted to
 *          store absolute date time information in ISO 8601 strings.
 *
 * \param[in] GroupName Name of group in ObsSpace database
 * \param[in] VarName Name of variable in ObsSpace database
 * \param[out] VarData Character array where ISO 8601 date time strings will be placed
 */
void NetcdfIO::ReadConvertDateTime(std::string GroupName, std::string VarName, char * VarData) {
  // Read in the reference date from the date_time attribute and the offset
  // time from the time variable and convert to date_time strings.
  int NcVarId = var_id(GroupName, VarName);
  std::vector<std::size_t> VarShape = var_shape(GroupName, VarName);

  // Read in the date_time attribute and convert to a DateTime object.
  int RefDateAttr;
  std::string ErrorMsg;
  ErrorMsg = "NetcdfIO::ReadDateTime: Unable to read attribute: date_time";
  CheckNcCall(nc_get_att_int(ncid_, NC_GLOBAL, "date_time", &RefDateAttr), ErrorMsg);

  int Year = RefDateAttr / 1000000;
  int TempInt = RefDateAttr % 1000000;
  int Month = TempInt / 10000;
  TempInt = TempInt % 10000;
  int Day = TempInt / 100;
  int Hour = TempInt % 100;
  util::DateTime RefDate(Year, Month, Day, Hour, 0, 0);

  // Read in the time variable.
  std::vector <float> OffsetTime(VarShape[0], 0.0);
  ErrorMsg = "NetcdfIO::ReadDateTime: Unable to read variable: time@" + GroupName;
  CheckNcCall(nc_get_var_float(ncid_, NcVarId, OffsetTime.data()), ErrorMsg);

  // Convert offset time to a Duration and add to RefDate. Then use DateTime to
  // output an ISO 8601 datetime string, and place that string into VarData.
  util::DateTime ObsDateTime;
  std::string ObsDtString;
  std::size_t ichar = 0;
  for (std::size_t i = 0; i < VarShape[0]; ++i) {
    ObsDateTime = RefDate + util::Duration(round(OffsetTime[i] * 3600));
    ObsDtString = ObsDateTime.toString();
    for (std::size_t j = 0; j < ObsDtString.size(); j++) {
        VarData[ichar] = ObsDtString[j];
        ichar++;
    }
  }
}

}  // namespace ioda
