/*
 * (C) Copyright 2017-2019 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#include "ioda/fileio/NetcdfIO.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <iostream>
#include <memory>
#include <numeric>
#include <typeinfo>

#include <boost/math/special_functions/fpclassify.hpp>

#include "netcdf.h"

#include "oops/parallel/mpi/mpi.h"
#include "oops/util/abor1_cpp.h"
#include "oops/util/Duration.h"
#include "oops/util/Logger.h"
#include "oops/util/missingValues.h"

#include "ioda/core/IodaUtils.h"

////////////////////////////////////////////////////////////////////////
// Implementation of IodaIO for netcdf.
////////////////////////////////////////////////////////////////////////

namespace ioda {

// -----------------------------------------------------------------------------
/*!
 * \details This constructor will open the netcdf file. If opening in read
 *          mode, the parameters nlocs and nvars will be set
 *          by querying the size of dimensions of the same names in the input
 *          file. If opening in write mode, the parameters will be set from the
 *          same named arguements to this constructor.
 *
 * \param[in]  FileName Path to the netcdf file
 * \param[in]  FileMode "r" for read, "w" for overwrite to an existing file
 *                      and "W" for create and write to a new file.
 */

NetcdfIO::NetcdfIO(const std::string & FileName, const std::string & FileMode,
                   const std::size_t MaxFrameSize) :
               IodaIO(FileName, FileMode, MaxFrameSize), have_offset_time_(false),
               have_date_time_(false) {
  oops::Log::trace() << __func__ << " fname_: " << fname_ << " fmode_: " << fmode_ << std::endl;

  // Open the file. The fmode_ values that are recognized are:
  //    "r" - read
  //    "w" - write, disallow overriting an existing file
  //    "W" - write, allow overwriting an existing file
  std::string ErrorMsg = "NetcdfIO::NetcdfIO: Unable to open file: '" + fname_ +
                         "' in mode: " + fmode_;
  if (fmode_ == "r") {
    CheckNcCall(nc_open(fname_.c_str(), NC_NOWRITE, &ncid_), ErrorMsg);
  } else if (fmode_ == "w") {
    CheckNcCall(nc_create(fname_.c_str(), NC_NOCLOBBER|NC_NETCDF4, &ncid_), ErrorMsg);
  } else if (fmode_ == "W") {
    CheckNcCall(nc_create(fname_.c_str(), NC_CLOBBER|NC_NETCDF4, &ncid_), ErrorMsg);
  } else {
    oops::Log::error() << "NetcdfIO::NetcdfIO: Unrecognized FileMode: " << fmode_ << std::endl;
    oops::Log::error() << "NetcdfIO::NetcdfIO: Must use one of: 'r', 'w', 'W'" << std::endl;
    ABORT("Unrecognized file mode for NetcdfIO constructor");
  }

  // When in read mode, the constructor is responsible for setting
  // the data members nlocs_, nvars_ and grp_var_info_.
  //
  // The files have nlocs, nvars.
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

    // Record the dimension ids and sizes in the dim_info_ container.
    // Save nlocs, nvars in data members.
    for (std::size_t i = 0; i < NcNdims; i++) {
      char NcName[NC_MAX_NAME+1];
      std::size_t NcSize;
      ErrorMsg = "NetcdfIO::NetcdfIO: Unable to read dimension number: " + std::to_string(i);
      CheckNcCall(nc_inq_dim(ncid_, i, NcName, &NcSize), ErrorMsg);
      dim_info_[NcName].size = NcSize;
      dim_info_[NcName].id = i;

      if (strcmp(NcName, "nlocs") == 0) {
        nlocs_ = NcSize;
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

    std::size_t MaxVarSize = 0;
    for (std::size_t ivar=0; ivar < NcNvars; ++ivar) {
      // nc variable dimension and type information
      char NcVname[NC_MAX_NAME+1];
      nc_type NcDtype;
      int NcNdims;
      int NcDimIds[NC_MAX_VAR_DIMS];
      ErrorMsg = "NetcdfIO::NetcdfIO: Unable to read information for variable number: " +
                  std::to_string(ivar);
      CheckNcCall(nc_inq_var(ncid_, ivar, NcVname, &NcDtype, &NcNdims, NcDimIds, 0), ErrorMsg);

      // nc type name
      char NcDtypeName[NC_MAX_NAME+1];
      std::size_t NcDtypeSize;
      ErrorMsg = "NetcdfIO::NetcdfIO: Unable to look up type name";
      CheckNcCall(nc_inq_type(ncid_, NcDtype, NcDtypeName, &NcDtypeSize), ErrorMsg);
      if (strcmp(NcDtypeName, "char") == 0) {
        strncpy(NcDtypeName, "string", NC_MAX_NAME);
      }
      std::string VarType(NcDtypeName);

      if (((VarType != "string") && (NcNdims == 1)) ||
          ((VarType == "string") && (NcNdims == 2))) {
        // Collect the sizes for dimensions from the dim_info_ container.
        std::vector<std::size_t> NcDimSizes;
        for (std::size_t j = 0; j < NcNdims; j++) {
            NcDimSizes.push_back(dim_id_size(NcDimIds[j]));
        }
        // Copy shape from NcDims to VarShape. If we have a string data type, then
        // copy only the first dimension size to VarShape since the internal variable
        // is a vector of strings, and the file variable is a 2D character array.
        std::vector<std::size_t> VarShape(1, NcDimSizes[0]);

        // Record the maximum variable size for the frame construction below.
        MaxVarSize = std::max(MaxVarSize, VarShape[0]);

        // Record the variable info in the grp_var_info_ container.
        std::string VarName;
        std::string GroupName;
        ExtractGrpVarName(NcVname, GroupName, VarName);

        // If the file type is double, change to float and increment the unexpected
        // data type counter.
        if (strcmp(NcDtypeName, "double") == 0) {
          VarType = "float";
          num_unexpect_dtypes_++;
        }

        // If the group name is "PreQC" and the file type is not integer, change to
        // integer and increment the unexpected data type counter.
        if ((GroupName == "PreQC") && (strcmp(NcDtypeName, "int") != 0)) {
          VarType = "int";
          num_unexpect_dtypes_++;
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
            // datetime strings are 20 character long
            grp_var_insert(GroupName, "datetime", "string", VarShape, NcVname, NcDtypeName, 20);
          }
        } else {
          // enter var specs into grp_var_info_ map
          if (strcmp(NcDtypeName, "string") == 0) {
            grp_var_insert(GroupName, VarName, VarType, VarShape, NcVname, NcDtypeName,
                           NcDimSizes[1]);
          } else {
            grp_var_insert(GroupName, VarName, VarType, VarShape, NcVname, NcDtypeName);
          }
        }
      } else {
        num_excess_dims_++;
      }
    }

    // Set up the frames based on the largest variable size (first dimension of each
    // variable). The netcdf file doesn't really have frames per se, but we want to
    // emulate frames to make access to files generic.
    frame_info_init(MaxVarSize);
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
 * \details The four NcReadVar methods are the same with the exception of the
 *          datatype that is being read (integer, float, double, string).
 *
 * \param[in]  GroupName Name of ObsSpace group (ObsValue, ObsError, MetaData, etc.)
 * \param[in]  VarName Name of ObsSpace variable
 * \param[in]  Starts Starting indices in file
 * \param[in]  Counts Size of slices to read from file
 * \param[out] FillValue Netcdf fill value associated with this variable
 * \param[out] VarData Vector that will receive the file data
 */
void NetcdfIO::NcReadVar(const std::string & GroupName, const std::string & VarName,
                         const std::vector<std::size_t> & Starts,
                         const std::vector<std::size_t> & Counts,
                         int & FillValue, std::vector<int> & VarData) {
  // Get the netcdf variable id
  int NcVarId = var_id(GroupName, VarName);

  // Get the variable name
  std::array<char, NC_MAX_NAME> NcVarName;
  std::string ErrorMsg =
      "NetcdfIO::ReadVar: Unable to get netcdf variable name: " + std::to_string(NcVarId);
  CheckNcCall(nc_inq_varname(ncid_, NcVarId, NcVarName.data()), ErrorMsg);

  // Read variable, and get the fill value.
  VarData.assign(Counts[0], 0);  // allocate space for the netcdf read
  ErrorMsg = "NetcdfIO::ReadVar: Unable to read netcdf variable: ";
  ErrorMsg += NcVarName.data();
  CheckNcCall(nc_get_vara_int(ncid_, NcVarId, Starts.data(), Counts.data(),
                              VarData.data()), ErrorMsg);

  if (NcAttrExists(NcVarId, "_FillValue")) {
    ErrorMsg = "NetcdfIO::ReadVar: cannot read _FillValue attribute for variable: ";
    ErrorMsg += NcVarName.data();
    CheckNcCall(nc_get_att_int(ncid_, NcVarId, "_FillValue", &FillValue), ErrorMsg);
  } else {
    FillValue = NC_FILL_INT;
  }
}

void NetcdfIO::NcReadVar(const std::string & GroupName, const std::string & VarName,
                         const std::vector<std::size_t> & Starts,
                         const std::vector<std::size_t> & Counts,
                         float & FillValue, std::vector<float> & VarData) {
  // Get the netcdf variable id
  int NcVarId = var_id(GroupName, VarName);

  // Get the variable name
  std::array<char, NC_MAX_NAME> NcVarName;
  std::string ErrorMsg =
      "NetcdfIO::ReadVar: Unable to get netcdf variable name: " + std::to_string(NcVarId);
  CheckNcCall(nc_inq_varname(ncid_, NcVarId, NcVarName.data()), ErrorMsg);

  // Read variable, and get the fill value.
  VarData.assign(Counts[0], 0.0);  // allocate space for the netcdf read
  ErrorMsg = "NetcdfIO::ReadVar: Unable to read netcdf variable: ";
  ErrorMsg += NcVarName.data();
  CheckNcCall(nc_get_vara_float(ncid_, NcVarId, Starts.data(), Counts.data(),
                               VarData.data()), ErrorMsg);

  if (NcAttrExists(NcVarId, "_FillValue")) {
    ErrorMsg = "NetcdfIO::ReadVar: cannot read _FillValue attribute for variable: ";
    ErrorMsg += NcVarName.data();
    CheckNcCall(nc_get_att_float(ncid_, NcVarId, "_FillValue", &FillValue), ErrorMsg);
  } else {
    FillValue = NC_FILL_FLOAT;
  }
}

void NetcdfIO::NcReadVar(const std::string & GroupName, const std::string & VarName,
                         const std::vector<std::size_t> & Starts,
                         const std::vector<std::size_t> & Counts,
                         double & FillValue, std::vector<double> & VarData) {
  // Get the netcdf variable id
  int NcVarId = var_id(GroupName, VarName);

  // Get the variable name
  std::array<char, NC_MAX_NAME> NcVarName;
  std::string ErrorMsg =
      "NetcdfIO::ReadVar: Unable to get netcdf variable name: " + std::to_string(NcVarId);
  CheckNcCall(nc_inq_varname(ncid_, NcVarId, NcVarName.data()), ErrorMsg);

  // Read variable, and get the fill value.
  VarData.assign(Counts[0], 0.0);  // allocate space for the netcdf read
  ErrorMsg = "NetcdfIO::ReadVar: Unable to read netcdf variable: ";
  ErrorMsg += NcVarName.data();
  CheckNcCall(nc_get_vara_double(ncid_, NcVarId, Starts.data(), Counts.data(),
                                VarData.data()), ErrorMsg);

  if (NcAttrExists(NcVarId, "_FillValue")) {
    ErrorMsg = "NetcdfIO::ReadVar: cannot read _FillValue attribute for variable: ";
    ErrorMsg += NcVarName.data();
    CheckNcCall(nc_get_att_double(ncid_, NcVarId, "_FillValue", &FillValue), ErrorMsg);
  } else {
    FillValue = NC_FILL_DOUBLE;
  }
}

void NetcdfIO::NcReadVar(const std::string & GroupName, const std::string & VarName,
                         const std::vector<std::size_t> & Starts,
                         const std::vector<std::size_t> & Counts,
                         char & FillValue, std::vector<std::string> & VarData) {
  // Get the netcdf variable id
  int NcVarId = var_id(GroupName, VarName);

  // Get the variable name and second dimension size.
  std::array<char, NC_MAX_NAME> NcVarName;
  std::string ErrorMsg =
      "NetcdfIO::ReadVar: Unable to get netcdf variable info: " + std::to_string(NcVarId);
  CheckNcCall(nc_inq_varname(ncid_, NcVarId, NcVarName.data()), ErrorMsg);

  ErrorMsg = "NetcdfIO::ReadVar: Unable to read netcdf variable: ";
  ErrorMsg += NcVarName.data();
  if (VarName == "datetime") {
    if (have_date_time_) {
    // datetime exists in the file, simply read it it. Counts holds the shape of the array
    std::unique_ptr<char[]> CharData(new char[Counts[0] * Counts[1]]);
    CheckNcCall(nc_get_vara_text(ncid_, NcVarId, Starts.data(), Counts.data(),
                                CharData.get()), ErrorMsg);
    VarData = CharArrayToStringVector(CharData.get(), Counts);
    } else {
      // datetime does not exist in the file, read in offset time and convert to
      // datetime strings.
      ReadConvertDateTime(GroupName, VarName, Starts, Counts, VarData);
    }
  } else {
  // All other variables beside datetime. Counts holds the shape of the array
  std::unique_ptr<char[]> CharData(new char[Counts[0] * Counts[1]]);
  CheckNcCall(nc_get_vara_text(ncid_, NcVarId, Starts.data(), Counts.data(),
                              CharData.get()), ErrorMsg);
  VarData = CharArrayToStringVector(CharData.get(), Counts);
  }

  if (NcAttrExists(NcVarId, "_FillValue")) {
    ErrorMsg = "NetcdfIO::ReadVar: cannot read _FillValue attribute for variable: ";
    ErrorMsg += NcVarName.data();
    CheckNcCall(nc_get_att_text(ncid_, NcVarId, "_FillValue", &FillValue), ErrorMsg);
  } else {
    FillValue = NC_FILL_CHAR;
  }
}

// -----------------------------------------------------------------------------
/*!
 * \brief Replace netcdf fill values with JEDI missing values
 *
 * \details This method replaces elements of VarData that match the netcdf
 *          fill value with the corresponding JEDI missing value.
 *
 * \param[inout] VarData Vector holding data to be converted
 * \param[in] NcFillValue Netcdf fill value associated with this variable
 */

template <typename DataType>
void NetcdfIO::ReplaceFillWithMissing(std::vector<DataType> & VarData, DataType NcFillValue) {
  const DataType missing_value = util::missingValue(missing_value);
  for (std::size_t i = 0; i < VarData.size(); i++) {
    if (VarData[i] == NcFillValue || boost::math::isinf(VarData[i])
                                  || boost::math::isnan(VarData[i])) {
      VarData[i] = missing_value;
    }
  }
}

// -----------------------------------------------------------------------------
/*!
 * \brief Write data from memory to netcdf file
 *
 * \details The three NcWriteVar methods are the same with the exception of the
 *          datatype that is being written (integer, float, char).
 *
 * \param[in]  GroupName Name of ObsSpace group (ObsValue, ObsError, MetaData, etc.)
 * \param[in]  VarName Name of ObsSpace variable
 * \param[in]  Starts Starting indices in file
 * \param[in]  Counts Size of slices to read from file
 * \param[in]  VarData Vector that will be written into the file
 */

void NetcdfIO::NcWriteVar(const std::string & GroupName, const std::string & VarName,
                          const std::vector<std::size_t> & Starts,
                          const std::vector<std::size_t> & Counts,
                          const std::vector<int> & VarData) {
  int MissVal = util::missingValue(MissVal);
  int NcFillVal = NC_FILL_INT;

  // Replace missing values with fill value, then write into the file.
  std::vector<int> FileData = VarData;
  for (std::size_t i = 0; i < FileData.size(); i++) {
    if (FileData[i] == MissVal) {
      FileData[i] = NcFillVal;
    }
  }

  int NcVarId = var_id(GroupName, VarName);
  std::string NcVarName = FormNcVarName(GroupName, VarName);
  std::string ErrorMsg = "NetcdfIO::WriteVar: Unable to write dataset: " + NcVarName;
  CheckNcCall(nc_put_vara_int(ncid_, NcVarId, Starts.data(), Counts.data(),
              FileData.data()), ErrorMsg);
}

void NetcdfIO::NcWriteVar(const std::string & GroupName, const std::string & VarName,
                          const std::vector<std::size_t> & Starts,
                          const std::vector<std::size_t> & Counts,
                          const std::vector<float> & VarData) {
  float MissVal = util::missingValue(MissVal);
  float NcFillVal = NC_FILL_FLOAT;

  // Replace missing values with fill value, then write into the file.
  std::vector<float> FileData = VarData;
  for (std::size_t i = 0; i < FileData.size(); i++) {
    if (FileData[i] == MissVal) {
      FileData[i] = NcFillVal;
    }
  }

  int NcVarId = var_id(GroupName, VarName);
  std::string NcVarName = FormNcVarName(GroupName, VarName);
  std::string ErrorMsg = "NetcdfIO::WriteVar: Unable to write dataset: " + NcVarName;
  CheckNcCall(nc_put_vara_float(ncid_, NcVarId, Starts.data(), Counts.data(),
              FileData.data()), ErrorMsg);
}

void NetcdfIO::NcWriteVar(const std::string & GroupName, const std::string & VarName,
                          const std::vector<std::size_t> & Starts,
                          const std::vector<std::size_t> & Counts,
                          const std::vector<std::string> & VarData) {
  std::unique_ptr<char[]> CharData(new char[Counts[0] * Counts[1]]);
  StringVectorToCharArray(VarData, Counts, CharData.get());

  int NcVarId = var_id(GroupName, VarName);
  std::string NcVarName = FormNcVarName(GroupName, VarName);
  std::string ErrorMsg = "NetcdfIO::WriteVar: Unable to write dataset: " + NcVarName;
  CheckNcCall(nc_put_vara_text(ncid_, NcVarId, Starts.data(), Counts.data(),
              CharData.get()), ErrorMsg);
}

// -----------------------------------------------------------------------------
/*!
 * \brief Get the max string size in a vector of strings
 *
 * \details This method will return the size of the longest string in a vector
 *          of strings.
 */
std::size_t NetcdfIO::GetMaxStringSize(const std::vector<std::string> & Strings) {
  std::size_t MaxSize = 0;
  for (std::size_t i = 0; i < Strings.size(); ++i) {
    if (Strings[i].size() > MaxSize) {
      MaxSize = Strings[i].size();
    }
  }
  return MaxSize;
}
// -----------------------------------------------------------------------------
/*!
 * \brief Get the netcdf dimension ids
 *
 * \details This method will determine the netcdf dimension ids associated with
 *          the current group name.
 */
std::vector<int> NetcdfIO::GetNcDimIds(const std::string & GroupName,
                                       const std::vector<std::size_t> & VarShape) {
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
    if (!dim_exists("nlocs")) {
      dim_insert("nlocs", VarShape[0]);
    }
    NcDimIds.push_back(dim_name_id("nlocs"));
  } else if (GroupName.compare("VarMetaData") == 0) {
    if (!dim_exists("nvars")) {
      dim_insert("nvars", VarShape[0]);
    }
    NcDimIds.push_back(dim_name_id("nvars"));
  } else if (GroupName.compare("RecMetaData") == 0) {
    if (!dim_exists("nrecs")) {
      dim_insert("nrecs", VarShape[0]);
    }
    NcDimIds.push_back(dim_name_id("nrecs"));
  } else {
    if (!dim_exists("nlocs")) {
      dim_insert("nlocs", VarShape[0]);
    }
    NcDimIds.push_back(dim_name_id("nlocs"));
  }

  return NcDimIds;
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
 * \param[in] Name Name of netcdf dimension
 * \param[in] Size Size of netcdf dimension
 */

void NetcdfIO::DimInsert(const std::string & Name, const std::size_t Size) {
  int NcDimId;
  std::string ErrorMsg = "NetcdfIO::NetcdfIO: Unable to create dimension: " + Name;
  CheckNcCall(nc_def_dim(ncid_, Name.c_str(), Size, &NcDimId), ErrorMsg);
  dim_info_[Name].size = Size;
  dim_info_[Name].id = NcDimId;
}

// -----------------------------------------------------------------------------
/*!
 * \brief Read data from the file into the frame containers
 *
 */
void NetcdfIO::ReadFrame(IodaIO::FrameIter & iframe) {
  // Grab the specs for the current frame
  std::size_t FrameStart = frame_start(iframe);
  std::size_t FrameSize = frame_size(iframe);

  // Create new containers and read data from the file into them.
  frame_data_init();

  for (IodaIO::GroupIter igrp = group_begin(); igrp != group_end(); ++igrp) {
    std::string GroupName = group_name(igrp);
    for (IodaIO::VarIter ivar = var_begin(igrp); ivar != var_end(igrp); ++ivar) {
      // Since there are variables of different lengths in a netcdf file, check to
      // see if the frame has gone past the end of the variable, and if so skip
      // that variable.
      std::vector<std::size_t> VarShape = var_shape(ivar);
      if (VarShape[0] > FrameStart) {
        // Grab the var specs, and calculate the start and count in the file.
        // Make sure the count doesn't go past the end of the variable in the file.
        // The start and count only apply to the first dimension, the remaining
        // dimensions, if any are always read in full.
        std::string VarName = var_name(ivar);
        std::string VarType = var_dtype(ivar);
        std::string FileType = file_type(ivar);
        std::vector<std::size_t> VarStarts(1, FrameStart);

        std::vector<std::size_t> VarCounts;
        if (FrameStart + FrameSize > VarShape[0]) {
          VarCounts.push_back(VarShape[0] - FrameStart);
        } else {
          VarCounts.push_back(FrameSize);
        }

        // Compare the variable type for memory (VarType) with the variable type
        // from the file (FileType). Do conversions for cases that have come up with
        // various netcdf obs files. The idea is to get all obs files with the expected
        // data types so that these conversions will be unnecessary. Once we are there
        // with the files, then make any unexpected data type in the file an error so that
        // it will get corrected immediately.
        if (VarType == "int") {
          if (FileType == "int") {
            std::vector<int> FileData;
            int NcFillValue;
            NcReadVar(GroupName, VarName, VarStarts, VarCounts, NcFillValue, FileData);
            ReplaceFillWithMissing<int>(FileData, NcFillValue);
            int_frame_data_->put_data(GroupName, VarName, FileData);
          } else if (FileType == "float") {
            std::vector<float> FileData;
            float NcFillValue;
            NcReadVar(GroupName, VarName, VarStarts, VarCounts, NcFillValue, FileData);
            ReplaceFillWithMissing<float>(FileData, NcFillValue);

            std::vector<int> FrameData(FileData.size(), 0);
            ConvertVarType<float, int>(FileData, FrameData);
            int_frame_data_->put_data(GroupName, VarName, FrameData);
          } else if (FileType == "double") {
            std::vector<double> FileData;
            double NcFillValue;
            NcReadVar(GroupName, VarName, VarStarts, VarCounts, NcFillValue, FileData);
            ReplaceFillWithMissing<double>(FileData, NcFillValue);

            std::vector<int> FrameData(FileData.size(), 0);
            ConvertVarType<double, int>(FileData, FrameData);
            int_frame_data_->put_data(GroupName, VarName, FrameData);
          } else {
            std::string ErrorMsg =
              "NetcdfIO::ReadFrame: Conflicting data types for conversion to int: ";
            ErrorMsg += "File variable: " + file_name(ivar) + ", File type: " + FileType;
            ABORT(ErrorMsg);
          }
        } else if (VarType == "float") {
          if (FileType == "float") {
            std::vector<float> FileData;
            float NcFillValue;
            NcReadVar(GroupName, VarName, VarStarts, VarCounts, NcFillValue, FileData);
            ReplaceFillWithMissing<float>(FileData, NcFillValue);
            float_frame_data_->put_data(GroupName, VarName, FileData);
          } else if (FileType == "double") {
            std::vector<double> FileData;
            double NcFillValue;
            NcReadVar(GroupName, VarName, VarStarts, VarCounts, NcFillValue, FileData);
            ReplaceFillWithMissing<double>(FileData, NcFillValue);

            std::vector<float> FrameData(FileData.size(), 0.0);
            ConvertVarType<double, float>(FileData, FrameData);
            float_frame_data_->put_data(GroupName, VarName, FrameData);
          } else {
            std::string ErrorMsg =
              "NetcdfIO::ReadFrame: Conflicting data types for conversion to float: ";
            ErrorMsg += "File variable: " + file_name(ivar) + ", File type: " + FileType;
            ABORT(ErrorMsg);
          }
        } else if (VarType == "string") {
          // Need to expand Starts, Counts with remaining dimensions.
          std::vector<std::size_t> VarFileShape = file_shape(ivar);
          for (std::size_t i = 1; i < VarFileShape.size(); ++i) {
            VarStarts.push_back(0);
            VarCounts.push_back(VarFileShape[i]);
          }
          std::vector<std::string> FileData;
          char NcFillValue;
          NcReadVar(GroupName, VarName, VarStarts, VarCounts, NcFillValue, FileData);
          string_frame_data_->put_data(GroupName, VarName, FileData);
        }
      }
    }
  }
}

// -----------------------------------------------------------------------------
/*!
 * \brief Write data from the frame containers into the file
 *
 */

void NetcdfIO::WriteFrame(IodaIO::FrameIter & iframe) {
  // Grab the specs for the current frame
  std::size_t FrameStart = frame_start(iframe);

  std::vector<std::size_t> Starts(1, FrameStart);
  std::vector<std::size_t> Counts;

  std::string GroupName;
  std::string VarName;
  std::vector<std::size_t> VarShape;

  // Walk through the int, float, and string frame containers and dump
  // their contents into the output file.
  for (IodaIO::FrameIntIter iframe = frame_int_begin();
                            iframe != frame_int_end(); ++iframe) {
    GroupName = frame_int_get_gname(iframe);
    VarName = frame_int_get_vname(iframe);
    std::vector<int> FrameData = frame_int_get_data(iframe);
    Counts.assign(1, FrameData.size());
    NcWriteVar(GroupName, VarName, Starts, Counts, FrameData);
  }

  for (IodaIO::FrameFloatIter iframe = frame_float_begin();
                              iframe != frame_float_end(); ++iframe) {
    GroupName = frame_float_get_gname(iframe);
    VarName = frame_float_get_vname(iframe);
    std::vector<float> FrameData = frame_float_get_data(iframe);
    Counts.assign(1, FrameData.size());
    NcWriteVar(GroupName, VarName, Starts, Counts, FrameData);
  }

  for (IodaIO::FrameStringIter iframe = frame_string_begin();
                               iframe != frame_string_end(); ++iframe) {
    GroupName = frame_string_get_gname(iframe);
    VarName = frame_string_get_vname(iframe);
    std::vector<std::string> FrameData = frame_string_get_data(iframe);
    std::vector<std::size_t> FileShape = file_shape(GroupName, VarName);
    std::vector<std::size_t> CharStarts{ Starts[0], 0 };
    std::vector<std::size_t> CharCounts{ FrameData.size(), FileShape[1] };
    NcWriteVar(GroupName, VarName, CharStarts, CharCounts, FrameData);
  }
}

// -----------------------------------------------------------------------------
/*!
 * \brief Add entry to the group, variable info container
 *
 */

void NetcdfIO::GrpVarInsert(const std::string & GroupName, const std::string & VarName,
                 const std::string & VarType, const std::vector<std::size_t> & VarShape,
                 const std::string & FileVarName, const std::string & FileType,
                 const std::size_t MaxStringSize) {
  int NcVarId;
  std::string ErrorMsg;

  // If string type, append the MaxStringSize to the FileShape since a vector of strings
  // is stored as a 2D character array in netcdf.
  std::vector<std::size_t> FileShape = VarShape;
  if (VarType == "string") {
    FileShape.push_back(MaxStringSize);
  }

  if (fmode_ == "r") {
    ErrorMsg = "NetcdfIO::GrpVarInsert: Unable to get netcdf id for variable: " + FileVarName;
    CheckNcCall(nc_inq_varid(ncid_, FileVarName.c_str(), &NcVarId), ErrorMsg);

    grp_var_info_[GroupName][VarName].var_id = NcVarId;
    grp_var_info_[GroupName][VarName].dtype = VarType;
    grp_var_info_[GroupName][VarName].file_shape = FileShape;
    grp_var_info_[GroupName][VarName].file_name = FileVarName;
    grp_var_info_[GroupName][VarName].file_type = FileType;
    grp_var_info_[GroupName][VarName].shape = VarShape;
  } else {
    // Write mode, create the netcdf variable and insert data into
    // group, variable info container
    nc_type NcVarType = NC_NAT;
    if (VarType == "int") {
      NcVarType = NC_INT;
    } else if (VarType == "float") {
      NcVarType = NC_FLOAT;
    } else if (VarType == "string") {
      NcVarType = NC_CHAR;
    } else {
      ErrorMsg = "NetcdfIO::GrpVarInsert: Unrecognized variable type: " + VarType +
                 ", must use one of: int, float, string";
      ABORT(ErrorMsg);
    }

    std::vector<int> NcDimIds = GetNcDimIds(GroupName, FileShape);
    if (VarType == "string") {
      NcDimIds.push_back(GetStringDimBySize(FileShape[1]));
    }
    ErrorMsg = "NetcdfIO::WriteVar: Unable to create variable dataset: " + FileVarName;
    CheckNcCall(nc_def_var(ncid_, FileVarName.c_str(), NcVarType, NcDimIds.size(),
                           NcDimIds.data(), &NcVarId), ErrorMsg);

    grp_var_info_[GroupName][VarName].var_id = NcVarId;
    grp_var_info_[GroupName][VarName].dtype = VarType;
    grp_var_info_[GroupName][VarName].file_shape = FileShape;
    grp_var_info_[GroupName][VarName].file_name = FileVarName;
    grp_var_info_[GroupName][VarName].file_type = FileType;
    grp_var_info_[GroupName][VarName].shape = VarShape;
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
    oops::Log::error() << ErrorMsg << " [NetCDF message: '"
                       << nc_strerror(RetCode) << "']" << std::endl;
    ABORT(ErrorMsg);
  }
}

// -----------------------------------------------------------------------------
/*!
 * \brief Check existence of netcdf attribute.
 *
 * \details This method will check to see if a netcdf exists in the
 *          input netcdf file. This can be used to check for existence
 *          of group and variable attributes.
 *
 * \param[in] AttrOwnerId Id number of owner of the attribute
 * \param[in] AttrName Name attribute
 */

bool NetcdfIO::NcAttrExists(const int & AttrOwnerId, const std::string & AttrName) {
  nc_type AttrType;
  std::size_t AttrLen;
  return (nc_inq_att(ncid_, AttrOwnerId, AttrName.c_str() , &AttrType, &AttrLen) == NC_NOERR);
}

// -----------------------------------------------------------------------------
/*!
 * \brief form the netcdf variable name
 *
 * \details This method will construct the name of the variable in the netcdf
 *           file from the given GroupName and VarName arguments. The netcdf
 *           variable name is "VarName@GroupName".
 *
 * \param[in] GroupName Name of group in ObsSpace database
 * \param[in] VarName Name of variable in ObsSpace database
 */

std::string NetcdfIO::FormNcVarName(const std::string & GroupName, const std::string & VarName) {
  std::string NcVarName = VarName + "@" + GroupName;
  return NcVarName;
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

  // If the dimenision exists simply return the id, otherwise create the dimension
  // and return the new dimension's id.
  int DimId;
  if (dim_exists(DimName)) {
    // Found so simply return the id
    DimId = dim_name_id(DimName);
  } else {
    // Not found so create the dimension and get the id
    std::string ErrorMsg = "NetcdfIO::NetcdfIO: Unable to create dimension: " + DimName;
    CheckNcCall(nc_def_dim(ncid_, DimName.c_str(), DimSize, &DimId), ErrorMsg);
    dim_info_[DimName].id = DimId;
    dim_info_[DimName].size = DimSize;
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
void NetcdfIO::ReadConvertDateTime(const std::string & GroupName, const std::string & VarName,
                                   const std::vector<std::size_t> & Starts,
                                   const std::vector<std::size_t> & Counts,
                                   std::vector<std::string> & VarData) {
  // Read in the reference date from the date_time attribute and the offset
  // time from the time variable and convert to date_time strings.
  int NcVarId = var_id(GroupName, VarName);

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
  std::vector<float> OffsetTime(Counts[0], 0.0);
  ErrorMsg = "NetcdfIO::ReadDateTime: Unable to read variable: time@" + GroupName;
  CheckNcCall(nc_get_vara_float(ncid_, NcVarId, Starts.data(), Counts.data(),
                                OffsetTime.data()), ErrorMsg);

  // Convert offset time to a Duration and add to RefDate. Then use DateTime to
  // output an ISO 8601 datetime string, and place that string into VarData.
  util::DateTime ObsDateTime;
  VarData.assign(Counts[0], "");  // allocate space for the conversion
  for (std::size_t i = 0; i < Counts[0]; ++i) {
    ObsDateTime = RefDate + util::Duration(round(OffsetTime[i] * 3600));
    VarData[i] = ObsDateTime.toString();
  }
}

}  // namespace ioda
