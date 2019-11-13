/*
 * (C) Copyright 2017-2019 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#include "fileio/NetcdfIO.h"

#include <math.h>
#include <netcdf.h>

#include <functional>
#include <iostream>
#include <memory>
#include <numeric>
#include <string>
#include <typeinfo>

#include <boost/math/special_functions/fpclassify.hpp>
#include "oops/parallel/mpi/mpi.h"
#include "oops/util/abor1_cpp.h"
#include "oops/util/DateTime.h"
#include "oops/util/Duration.h"
#include "oops/util/Logger.h"
#include "oops/util/missingValues.h"
#include "utils/IodaUtils.h"

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
      CheckNcCall(nc_inq_varid(ncid_, NcVname, &NcVarId), ErrorMsg);

      // nc type name
      char NcDtypeName[NC_MAX_NAME+1];
      std::size_t NcDtypeSize;
      ErrorMsg = "NetcdfIO::NetcdfIO: Unable to look up type name";
      CheckNcCall(nc_inq_type(ncid_, NcDtype, NcDtypeName, &NcDtypeSize), ErrorMsg);
      if (strcmp(NcDtypeName, "char") == 0) {
        strncpy(NcDtypeName, "string", NC_MAX_NAME);
      }

      // Collect the sizes for dimensions from the dim_info_ container.
      std::vector<std::size_t> NcDimSizes;
      for (std::size_t j = 0; j < NcNdims; j++) {
          NcDimSizes.push_back(dim_id_size(NcDimIds[j]));
      }
      // Copy shape from NcDims to VarShape. If we have a string data type, then
      // copy only the first dimension size to VarShape since the internal variable
      // is a vector of strings, and the file variable is a 2D character array.
      std::vector<std::size_t> VarShape;
      if (strcmp(NcDtypeName, "string") == 0) {
        VarShape.push_back(NcDimSizes[0]);
      } else {
        VarShape = NcDimSizes;
      }

      // Record the maximum variable size for the frame construction below.
      if (VarShape[0] > MaxVarSize) {
        MaxVarSize = VarShape[0];
      }

      // Record the variable info in the grp_var_into_ container.
      int OffsetTimeVarId;
      std::string VarName;
      std::string GroupName;
      ExtractGrpVarName(NcVname, GroupName, VarName);

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
          grp_var_info_[GroupName]["datetime"].dtype = "string";
          grp_var_info_[GroupName]["datetime"].file_shape = NcDimSizes;
          grp_var_info_[GroupName]["datetime"].shape = VarShape;
        }
      } else {
        // enter var specs into grp_var_info_ map
        grp_var_info_[GroupName][VarName].var_id = NcVarId;
        grp_var_info_[GroupName][VarName].dtype = NcDtypeName;
        grp_var_info_[GroupName][VarName].file_shape = NcDimSizes;
        grp_var_info_[GroupName][VarName].shape = VarShape;
      }
    }

    // Set up the frames based on the largest variable size (first dimension of each
    // variable). The netcdf file doesn't really have frames per se, but we want to
    // emulate frames to make access to files generic. The idea here is to divide the
    // variable size up into max_frame_size_ pieces, but make the final frame size so
    // that the total of all frame sizes equals the largest variable size.
    std::size_t FrameStart = 0;
    while (FrameStart < MaxVarSize) {
      std::size_t FrameSize = max_frame_size_;
      if ((FrameStart + FrameSize) > MaxVarSize) {
        FrameSize = MaxVarSize - FrameStart;
      }
      IodaIO::FrameInfoRec Finfo(FrameStart, FrameSize);
      frame_info_.push_back(Finfo);

      FrameStart += max_frame_size_;
    }

    for (std::size_t i = 0; i < frame_info_.size(); ++i) {
      std::cout << "DEBUG: frame_info_: " << i << " --> " << frame_info_[i].start << ", "
                << frame_info_[i].size << std::endl;
    }
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
 *          datatype that is being read (integer, float, double, string).
 *
 * \param[in]  GroupName Name of ObsSpace group (ObsValue, ObsError, MetaData, etc.)
 * \param[in]  VarName Name of ObsSpace variable
 * \param[in]  VarShape Dimension sizes of variable
 * \param[out] VarData Vector that will receive the file data
 */

void NetcdfIO::ReadVar(const std::string & GroupName, const std::string & VarName,
                       const std::vector<std::size_t> & VarShape, std::vector<int> & VarData) {
  std::string NcVarName = FormNcVarName(GroupName, VarName);
  int NcVarId = var_id(GroupName, VarName);
  int NcFillValue;

  std::string ErrorMsg = "NetcdfIO::ReadVar: Unable to read netcdf variable: " + VarName;
  CheckNcCall(nc_get_var_int(ncid_, NcVarId, VarData.data()), ErrorMsg);

  // Read variable, and get the fill value.
  if (NcAttrExists(NcVarId, "_FillValue")) {
    ErrorMsg = "NetcdfIO::ReadVar: cannot read _FillValue attribute for variable: " + VarName;
    CheckNcCall(nc_get_att_int(ncid_, NcVarId, "_FillValue", &NcFillValue), ErrorMsg);
  } else {
    NcFillValue = NC_FILL_INT;
  }
  ReplaceFillWithMissing<int>(VarData, NcFillValue);
}

void NetcdfIO::ReadVar(const std::string & GroupName, const std::string & VarName,
                       const std::vector<std::size_t> & VarShape, std::vector<float> & VarData) {
  std::string NcVarName = FormNcVarName(GroupName, VarName);
  int NcVarId = var_id(GroupName, VarName);
  float NcFillValue;

  std::string ErrorMsg = "NetcdfIO::ReadVar: Unable to read netcdf variable: " + VarName;
  CheckNcCall(nc_get_var_float(ncid_, NcVarId, VarData.data()), ErrorMsg);

  // Read variable, and get the fill value.
  if (NcAttrExists(NcVarId, "_FillValue")) {
    ErrorMsg = "NetcdfIO::ReadVar: cannot read _FillValue attribute for variable: " + VarName;
    CheckNcCall(nc_get_att_float(ncid_, NcVarId, "_FillValue", &NcFillValue), ErrorMsg);
  } else {
    NcFillValue = NC_FILL_FLOAT;
  }
  ReplaceFillWithMissing<float>(VarData, NcFillValue);
}

void NetcdfIO::ReadVar(const std::string & GroupName, const std::string & VarName,
                       const std::vector<std::size_t> & VarShape, std::vector<double> & VarData) {
  std::string NcVarName = FormNcVarName(GroupName, VarName);
  int NcVarId = var_id(GroupName, VarName);
  double NcFillValue;

  std::string ErrorMsg = "NetcdfIO::ReadVar: Unable to read netcdf variable: " + VarName;
  CheckNcCall(nc_get_var_double(ncid_, NcVarId, VarData.data()), ErrorMsg);

  // Read variable, and get the fill value.
  if (NcAttrExists(NcVarId, "_FillValue")) {
    ErrorMsg = "NetcdfIO::ReadVar: cannot read _FillValue attribute for variable: " + VarName;
    CheckNcCall(nc_get_att_double(ncid_, NcVarId, "_FillValue", &NcFillValue), ErrorMsg);
  } else {
    NcFillValue = NC_FILL_FLOAT;
  }
  ReplaceFillWithMissing<double>(VarData, NcFillValue);
}

void NetcdfIO::ReadVar(const std::string & GroupName, const std::string & VarName,
                       const std::vector<std::size_t> & VarShape,
                       std::vector<std::string> & VarData) {
  std::string NcVarName = FormNcVarName(GroupName, VarName);
  int NcVarId = var_id(GroupName, VarName);
  std::vector<std::size_t> NcVarShape = file_shape(GroupName, VarName);

  std::string ErrorMsg = "NetcdfIO::ReadVar: Unable to read netcdf variable: " + VarName;
  if (VarName == "datetime") {
    if (have_date_time_) {
    // datetime exists in the file, simply read it it.
    std::unique_ptr<char[]> CharData(new char[NcVarShape[0] * NcVarShape[1]]);
    CheckNcCall(nc_get_var_text(ncid_, NcVarId, CharData.get()), ErrorMsg);
    VarData = CharArrayToStringVector(CharData.get(), NcVarShape);
    } else {
    // datetime does not exist in the file, read in offset time and convert to
    // datetime strings.
      ReadConvertDateTime(GroupName, VarName, VarData);
    }
  } else {
  // All other variables beside datetime
  std::unique_ptr<char[]> CharData(new char[NcVarShape[0] * NcVarShape[1]]);
  CheckNcCall(nc_get_var_text(ncid_, NcVarId, CharData.get()), ErrorMsg);
  VarData = CharArrayToStringVector(CharData.get(), NcVarShape);
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
 * \details The three WriteVar methods are the same with the exception of the
 *          datatype that is being written (integer, float, char).
 *
 * \param[in]  GroupName Name of ObsSpace group (ObsValue, ObsError, MetaData, etc.)
 * \param[in]  VarName Name of ObsSpace variable
 * \param[in]  VarShape Dimension sizes of variable
 * \param[in]  VarData Vector that will be written into the file
 */

void NetcdfIO::WriteVar(const std::string & GroupName, const std::string & VarName,
                        const std::vector<std::size_t> & VarShape,
                        const std::vector<int> & VarData) {
  std::string NcVarName = FormNcVarName(GroupName, VarName);
  int MissVal = util::missingValue(MissVal);
  int NcFillVal = NC_FILL_INT;

  // If var doesn't exist in the file, then create it.
  std::string ErrorMsg;
  int NcVarId;
  if (nc_inq_varid(ncid_, NcVarName.c_str(), &NcVarId) != NC_NOERR) {
    std::vector<int> NcDimIds = GetNcDimIds(GroupName, VarShape);
    ErrorMsg = "NetcdfIO::WriteVar: Unable to create variable dataset: " + NcVarName;
    CheckNcCall(nc_def_var(ncid_, NcVarName.c_str(), NC_INT, NcDimIds.size(),
                           NcDimIds.data(), &NcVarId),
                ErrorMsg);
  }

  // Replace missing values with fill value, then write into the file.
  std::vector<int> FileData = VarData;
  for (std::size_t i = 0; i < FileData.size(); i++) {
    if (FileData[i] == MissVal) {
      FileData[i] = NcFillVal;
    }
  }
  ErrorMsg = "NetcdfIO::WriteVar: Unable to write dataset: " + NcVarName;
  CheckNcCall(nc_put_var_int(ncid_, NcVarId, FileData.data()), ErrorMsg);
}

void NetcdfIO::WriteVar(const std::string & GroupName, const std::string & VarName,
                        const std::vector<std::size_t> & VarShape,
                        const std::vector<float> & VarData) {
  std::string NcVarName = FormNcVarName(GroupName, VarName);
  float MissVal = util::missingValue(MissVal);
  float NcFillVal = NC_FILL_FLOAT;

  // If var doesn't exist in the file, then create it.
  std::string ErrorMsg;
  int NcVarId;
  if (nc_inq_varid(ncid_, NcVarName.c_str(), &NcVarId) != NC_NOERR) {
    std::vector<int> NcDimIds = GetNcDimIds(GroupName, VarShape);
    ErrorMsg = "NetcdfIO::WriteVar: Unable to create variable dataset: " + NcVarName;
    CheckNcCall(nc_def_var(ncid_, NcVarName.c_str(), NC_FLOAT, NcDimIds.size(),
                           NcDimIds.data(), &NcVarId),
                ErrorMsg);
  }

  // Replace missing values with fill value, then write into the file.
  std::vector<float> FileData = VarData;
  for (std::size_t i = 0; i < FileData.size(); i++) {
    if (FileData[i] == MissVal) {
      FileData[i] = NcFillVal;
    }
  }
  ErrorMsg = "NetcdfIO::WriteVar: Unable to write dataset: " + NcVarName;
  CheckNcCall(nc_put_var_float(ncid_, NcVarId, FileData.data()), ErrorMsg);
}

void NetcdfIO::WriteVar(const std::string & GroupName, const std::string & VarName,
                        const std::vector<std::size_t> & VarShape,
                        const std::vector<std::string> & VarData) {
  std::string NcVarName = FormNcVarName(GroupName, VarName);
  std::vector<std::size_t> NcVarShape = VarShape;
  NcVarShape.push_back(GetMaxStringSize(VarData));

  // If var doesn't exist in the file, then create it.
  std::string ErrorMsg;
  int NcVarId;
  if (nc_inq_varid(ncid_, NcVarName.c_str(), &NcVarId) != NC_NOERR) {
    std::vector<int> NcDimIds = GetNcDimIds(GroupName, VarShape);
    NcDimIds.push_back(GetStringDimBySize(NcVarShape[1]));
    ErrorMsg = "NetcdfIO::WriteVar: Unable to create variable dataset: " + NcVarName;
    CheckNcCall(nc_def_var(ncid_, NcVarName.c_str(), NC_CHAR, NcDimIds.size(),
                           NcDimIds.data(), &NcVarId),
                ErrorMsg);
  }

  std::unique_ptr<char[]> CharData(new char[NcVarShape[0] * NcVarShape[1]]);
  StringVectorToCharArray(VarData, NcVarShape, CharData.get());
  ErrorMsg = "NetcdfIO::WriteVar: Unable to write dataset: " + NcVarName;
  CheckNcCall(nc_put_var_text(ncid_, NcVarId, CharData.get()), ErrorMsg);
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
      CreateNcDim("nlocs", VarShape[0]);
    }
    NcDimIds.push_back(dim_name_id("nlocs"));
  } else if (GroupName.compare("VarMetaData") == 0) {
    if (!dim_exists("nvars")) {
      CreateNcDim("nvars", VarShape[0]);
    }
    NcDimIds.push_back(dim_name_id("nvars"));
  } else if (GroupName.compare("RecMetaData") == 0) {
    if (!dim_exists("nrecs")) {
      CreateNcDim("nrecs", VarShape[0]);
    }
    NcDimIds.push_back(dim_name_id("nrecs"));
  } else {
    if (!dim_exists("nlocs")) {
      CreateNcDim("nlocs", VarShape[0]);
    }
    NcDimIds.push_back(dim_name_id("nlocs"));
  }

  return NcDimIds;
}

// -----------------------------------------------------------------------------
/*!
 * \brief beginning frame iterator
 */

IodaIO::FrameIter NetcdfIO::frame_begin() {
  return frame_info_.begin();
  }

// -----------------------------------------------------------------------------
/*!
 * \brief ending frame iterator
 */

IodaIO::FrameIter NetcdfIO::frame_end() {
  return frame_info_.end();
  }

// -----------------------------------------------------------------------------
/*!
 * \brief start of current frame
 */

std::size_t NetcdfIO::frame_start(IodaIO::FrameIter iframe) {
  return iframe->start;
  }

// -----------------------------------------------------------------------------
/*!
 * \brief size of current frame
 */

std::size_t NetcdfIO::frame_size(IodaIO::FrameIter iframe) {
  return iframe->size;
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
  dim_info_[DimName].size = DimSize;
  dim_info_[DimName].id = NcDimId;
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
void NetcdfIO::ReadConvertDateTime(std::string GroupName, std::string VarName,
                                   std::vector<std::string> & VarData) {
  // Read in the reference date from the date_time attribute and the offset
  // time from the time variable and convert to date_time strings.
  int NcVarId = var_id(GroupName, VarName);
  std::vector<std::size_t> VarShape = var_shape(GroupName, VarName);
  std::size_t VarSize =
    std::accumulate(VarShape.begin(), VarShape.end(), 1, std::multiplies<std::size_t>());

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
  std::vector<float> OffsetTime(VarSize, 0.0);
  ErrorMsg = "NetcdfIO::ReadDateTime: Unable to read variable: time@" + GroupName;
  CheckNcCall(nc_get_var_float(ncid_, NcVarId, OffsetTime.data()), ErrorMsg);

  // Convert offset time to a Duration and add to RefDate. Then use DateTime to
  // output an ISO 8601 datetime string, and place that string into VarData.
  util::DateTime ObsDateTime;
  std::string ObsDtString;
  std::size_t ichar = 0;
  for (std::size_t i = 0; i < VarSize; ++i) {
    ObsDateTime = RefDate + util::Duration(round(OffsetTime[i] * 3600));
    VarData[i] = ObsDateTime.toString();
  }
}

}  // namespace ioda
