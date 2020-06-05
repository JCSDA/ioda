/*
 * (C) Copyright 2017-2019 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#include "fileio/OdcIO.h"

#include <fcntl.h>
#include <sys/stat.h>

#include <iomanip>
#include <sstream>

#include "eckit/exception/Exceptions.h"
#include "oops/util/abor1_cpp.h"
#include "oops/util/Logger.h"
#include "oops/util/missingValues.h"

////////////////////////////////////////////////////////////////////////
// Implementation of IodaIO for ODC.
////////////////////////////////////////////////////////////////////////

namespace ioda {

// -----------------------------------------------------------------------------
/*!
 * \details This constructor will open the ODB2 file. If opening in read
 *          mode, the parameters nlocs and nvars will be set
 *          by querying the size of dimensions of the table in the file file.
 *
 * \param[in]  FileName Path to the netcdf file
 * \param[in]  FileMode "r" for read, "w" for overwrite to an existing file
 *                      and "W" for create and write to a new file.
 */

OdcIO::OdcIO(const std::string & FileName, const std::string & FileMode,
             const std::size_t MaxFrameSize) :
          IodaIO(FileName, FileMode, MaxFrameSize), odc_reader_(nullptr), odc_frame_(nullptr),
                 odc_decoder_(nullptr), next_dim_id_(0) {
  oops::Log::trace() << __func__ << " fname_: " << fname_ << " fmode_: " << fmode_ << std::endl;

  // Initialize the API
  std::string ErrorMsg = "OdcIO::OdcIO: Unable to initialize the ODC API";
  CheckOdcCall(odc_initialise_api(), ErrorMsg);

  // Open the file, recognized modes are:
  //    "r" - read
  //    "w" - write, disallow overwriting an existing file
  //    "W" = write, allow overwriting an existing file
  std::string OdcWriterWarning = "WARNING: ODC writer is not implemented yet, ";
  OdcWriterWarning += "output file will not be created.";
  ErrorMsg = "OdcIO::OdcIO: Unable to open file: '"  + fname_ + "' in mode: " + fmode_;
  if (fmode_ == "r") {
    CheckOdcCall(odc_open_path(&odc_reader_, FileName.c_str()), ErrorMsg);
  } else if (fmode_ == "w") {
    oops::Log::info() << OdcWriterWarning << std::endl;
  } else if (fmode_ == "W") {
    oops::Log::info() << OdcWriterWarning << std::endl;
  } else {
    oops::Log::error() << "OdcIO::OdcIO: Unrecognized FileMode: " << fmode_ << std::endl;
    oops::Log::error() << "OdcIO::OdcIO: Must use one of: 'r', 'w', 'W'" << std::endl;
    ABORT("Unrecognized file mode for OdcIO constructor");
  }

  if (fmode_ == "r") {
    // Make a pass through the file to count the locations and variables, and to fill
    // in the grp_var_info_ map. This can be done quickly as long as you don't do any
    // decoding. The file may contain multiple frames.
    std::map<std::string, std::string> VarTypes;
    std::size_t TotalRows = 0;
    std::size_t FrameIndex = 0;
    ErrorMsg = "OdcIO::OdcIO: Unable to start a new ODC frame";
    CheckOdcCall(odc_new_frame(&odc_frame_, odc_reader_), ErrorMsg);
    while (odc_next_frame(odc_frame_) == ODC_SUCCESS) {
      long NumRows;
      int NumCols;
      ErrorMsg = "OdcIO::OdcIO: Unable to extract ODC frame row count";
      CheckOdcCall(odc_frame_row_count(odc_frame_, &NumRows), ErrorMsg);
      frame_info_insert(TotalRows, NumRows);

      ErrorMsg = "OdcIO::OdcIO: Unable to extract ODC frame column count";
      CheckOdcCall(odc_frame_column_count(odc_frame_, &NumCols), ErrorMsg);
      TotalRows += NumRows;

      if (FrameIndex == 0) {
        // Walk through the ODC column attributes and record the name and type for each
        // column (variable).
        num_odc_cols_ = NumCols;
        nvars_ = 0;
        for (std::size_t i = 0; i < NumCols; i++) {
          // Read column attributes from the frame header
          const char* TempName;
          int OdcDataType;
          int OdcElementSize;
          int OdcBitfieldCount;
          ErrorMsg = "OdcIO::OdcIO: Unable to extract ODC frame column attributes";
#ifdef ODC_RELEASE
          CheckOdcCall(odc_frame_column_attributes(odc_frame_, i, &TempName, &OdcDataType,
                                 &OdcElementSize, &OdcBitfieldCount), ErrorMsg);
#else
          CheckOdcCall(odc_frame_column_attrs(odc_frame_, i, &TempName, &OdcDataType,
                                 &OdcElementSize, &OdcBitfieldCount), ErrorMsg);
#endif
          std::string OdcColName = TempName;

          // Keep track of all variables with their column number (id number).
          // Keep track of variable types for inserting into the grp_var_info_ container below.
          // Skip over time@MetaData since two file variables, date@MetaData and time@MetaData,
          // will be converted to one frame variable, datetime@MetaData.
          var_ids_[OdcColName] = i;
          if (OdcColName != "time@MetaData") {
              VarTypes[OdcColName] = OdcTypeName(OdcDataType);
              nvars_++;
          }
        }
      } else {
        // Additional frame, check to see that it has the same number
        // of columns as the first frame.
        ASSERT(NumCols == num_odc_cols_);
      }
      FrameIndex++;
    }
    ErrorMsg = "OdcIO::OdcIO: Unable to free an ODC frame";
    CheckOdcCall(odc_free_frame(odc_frame_), ErrorMsg);
    nlocs_ = TotalRows;

    // For now, all columns are vectors with length nlocs_ so place this information
    // inside the grp_var_info_ map.
    std::map<std::string, std::string>::const_iterator ivar;
    for (ivar = VarTypes.begin(); ivar != VarTypes.end(); ++ivar) {
      std::string GroupName;
      std::string VarName;
      std::string FileName = ivar->first;
      std::string VarType = ivar->second;
      ExtractGrpVarName(FileName, GroupName, VarName);

      std::vector<std::size_t> VarShape(1, nlocs_);
      std::size_t MaxStringSize = 0;
      if (VarType == "string") {
        // For now only support a column that has one element of char data. One
        // element is 8 bytes (space for one double). Set MaxStringSize to the
        // number of elements (columns), not the actual string size.
        MaxStringSize = 1;
      }

      // Special case for datetime marks. File will contain two integer values, one
      // for date and the other for time.
      std::string FileType = VarType;
      if (FileName == "date@MetaData") {
        VarName = "datetime";
        VarType = "string";
      }

      grp_var_insert(GroupName, VarName, VarType, VarShape, FileName,
                     FileType, MaxStringSize);
    }

    // Again for now, there is only one dimension which is nlocs_. Record this information
    // in the dim_info_ container.
    dim_insert("nlocs", nlocs_);
  }
}

// -----------------------------------------------------------------------------

OdcIO::~OdcIO() {
  oops::Log::trace() << __func__ << " fname_: " << fname_ << std::endl;
  std::string ErrorMsg =
    "OdcIO::~OdcIO: Unable to close file: '"  + fname_ + "' in mode: " + fmode_;
  if (fmode_ == "r") {
    CheckOdcCall(odc_close(odc_reader_), ErrorMsg);
  }
}

// -----------------------------------------------------------------------------
/*!
 * \brief print method for stream output
 *
 * \details This method is supplied for the Printable base class. It defines
 *          how to print an object of this class in an output stream.
 */

void OdcIO::print(std::ostream & os) const {
  os << "ODC: In " << __FILE__ << " @ " << __LINE__ << std::endl;
  }

// -----------------------------------------------------------------------------
/*!
 * \brief convert ODC type number to a type name
 *
 * \details This method will convert the ODC type number to one of the following
 *          names with the associated meaning:
 *
 *          "int"        integer
 *          "float"      single precision real
 *          "double"     double precision real
 *          "char"       character string
 *          "bitfield"   ODC bit field
 *
 * \param[in] OdcDataType Number representing data type from ODC
 */

std::string OdcIO::OdcTypeName(int OdcDataType) {
  std::string Dtype;
  if (OdcDataType == ODC_INTEGER) {
    Dtype = "int";
  } else if (OdcDataType == ODC_REAL) {
    Dtype = "float";
  } else if (OdcDataType == ODC_DOUBLE) {
    Dtype = "double";
  } else if (OdcDataType == ODC_STRING) {
    Dtype = "string";
  } else if (OdcDataType == ODC_BITFIELD) {
    Dtype = "bitfield";
  } else {
    std::string ErrorMsg = "OdcIO::OdcIO: Unrecognized ODC data type: " +
                         std::to_string(OdcDataType);
    ABORT(ErrorMsg);
  }

  return Dtype;
}

// -----------------------------------------------------------------------------
/*!
 * \brief check results of odc call
 *
 * \details This method will check the return code from an ODC API call.
 *          Successful completion of the call is indicated by the return
 *          code being equal to ODC_SUCCESS. If the call was not successful,
 *          then the error message is written to the OOPS log, and is also
 *          sent to the OOPS ABORT call (execution is aborted).
 *
 * \param[in] RetCode Return code from ODC call
 * \param[in] ErrorMsg Message for the OOPS error logger
 */
void OdcIO::CheckOdcCall(int RetCode, std::string & ErrorMsg) {
  if (RetCode != ODC_SUCCESS) {
    oops::Log::error() << ErrorMsg << " [ODC message: '"
                       << odc_error_string(RetCode) << "']" << std::endl;
    ABORT(ErrorMsg);
  }
}

// -----------------------------------------------------------------------------
/*!
 * \brief create a dimension for the odc file
 *
 * \details This method will record the dimension name and size for downstream use in the
 *          WriteVar methods.
 *
 * \param[in] Name Name of netcdf dimension
 * \param[in] Size Size of netcdf dimension
 */

void OdcIO::DimInsert(const std::string & Name, const std::size_t Size) {
  dim_info_[Name].size = Size;
  dim_info_[Name].id = next_dim_id_;
  next_dim_id_++;
}

// -----------------------------------------------------------------------------
/*!
 * \brief Frame initialize
 *
 */
void OdcIO::InitializeFrame() {
  std::string ErrorMsg = "OdcIO::BeginFrame: Unable to start a new ODC frame";
  CheckOdcCall(odc_new_frame(&odc_frame_, odc_reader_), ErrorMsg);
}

// -----------------------------------------------------------------------------
/*!
 * \brief Frame finalize
 *
 */
void OdcIO::FinalizeFrame() {
  std::string ErrorMsg = "OdcIO::EndFrame: Unable to free an ODC frame";
  CheckOdcCall(odc_free_frame(odc_frame_), ErrorMsg);
}

// -----------------------------------------------------------------------------
/*!
 * \brief read date and time from file and convert to date time strings
 *
 */

void OdcIO::OdcReadVar(const std::size_t VarId, std::vector<int> & VarData) {
  long OdcMissingInteger;
  std::string ErrorMsg = "OdcIO::OdcReadVar(int): Unable to obtain ODC missing integer value";
  CheckOdcCall(odc_missing_integer(&OdcMissingInteger), ErrorMsg);
  const int JediMissingInteger = util::missingValue(JediMissingInteger);

  std::size_t Idata = VarId;
  long FrameData;
  for (std::size_t i = 0; i < VarData.size(); ++i) {
    FrameData = static_cast<long>(odc_frame_data_[Idata]);
    if (FrameData == OdcMissingInteger) {
      VarData[i] = JediMissingInteger;
    } else {
      VarData[i] = static_cast<int>(FrameData);
    }
    Idata += num_odc_cols_;
  }
}

void OdcIO::OdcReadVar(const std::size_t VarId, std::vector<float> & VarData) {
  double OdcMissingDouble;
  std::string ErrorMsg = "OdcIO::OdcReadVar(float): Unable to obtain ODC missing float value";
  CheckOdcCall(odc_missing_double(&OdcMissingDouble), ErrorMsg);
  const float JediMissingFloat = util::missingValue(JediMissingFloat);

  std::size_t Idata = VarId;
  double FrameData;
  for (std::size_t i = 0; i < VarData.size(); ++i) {
    FrameData = odc_frame_data_[Idata];
    if (FrameData == OdcMissingDouble) {
      VarData[i] = JediMissingFloat;
    } else {
      VarData[i] = static_cast<float>(FrameData);
    }
    Idata += num_odc_cols_;
  }
}

void OdcIO::OdcReadVar(const std::size_t VarId, std::vector<std::string> & VarData,
                       bool IsDateTime) {
  if (IsDateTime) {
    ReadConvertDateTime(VarData);
  } else {
    std::size_t Idata = VarId;
    std::size_t ElementSize = sizeof(double);
    for (std::size_t i = 0; i < VarData.size(); ++i) {
      char* FrameChar = reinterpret_cast<char *>(&(odc_frame_data_[Idata]));
      char FrameData[ElementSize+1];
      for (std::size_t j = 0; j < sizeof(double); ++j) {
        FrameData[j] = *(FrameChar + j);
      }
      FrameData[ElementSize] = '\0';
      VarData[i] = FrameData;
      Idata += num_odc_cols_;
    }
  }
}

// -----------------------------------------------------------------------------
/*!
 * \brief read date and time from file and convert to date time strings
 *
 */

void OdcIO::ReadConvertDateTime(std::vector<std::string> & DtStrings) {
  // Date and time variables are integers
  std::size_t VarSize = DtStrings.size();
  std::vector<int> Date(VarSize, 0);
  std::vector<int> Time(VarSize, 0);

  // Get the id (column number) of the date and time variables
  std::size_t DateId = var_id_get("date@MetaData");
  std::size_t TimeId = var_id_get("time@MetaData");

  // Read in the date and time data
  OdcReadVar(DateId, Date);
  OdcReadVar(TimeId, Time);

  // Pull out individual year, month, day, hour, minute, second values and form
  // the ISO8601 string. The date format is YYYYMMDD and the time format is hhmmss.
  for (std::size_t i = 0; i < VarSize; ++i) {
    std::size_t TempInt = Date[i];
    std::size_t Year = TempInt / 10000;
    TempInt = TempInt % 10000;
    std::size_t Month = TempInt / 100;
    std::size_t Day = TempInt % 100;

    TempInt = Time[i];
    std::size_t Hour = TempInt / 10000;
    TempInt = TempInt % 10000;
    std::size_t Minute = TempInt / 100;
    std::size_t Second = TempInt % 100;

    std::ostringstream os;
    os << std::setfill('0');
    os << std::setw(4) << Year;
    os.put('-');
    os << std::setw(2) << Month;
    os.put('-');
    os << std::setw(2) << Day;
    os.put('T');
    os << std::setw(2) << Hour;
    os.put(':');
    os << std::setw(2) << Minute;
    os.put(':');
    os << std::setw(2) << Second;
    os.put('Z');

    DtStrings[i] = os.str();
  }
}

// -----------------------------------------------------------------------------
/*!
 * \brief Read data from the file into the frame containers
 *
 */
void OdcIO::ReadFrame(IodaIO::FrameIter & iframe) {
  std::size_t FrameStart = frame_start(iframe);
  std::size_t FrameSize = frame_size(iframe);

  // Create new containers and read data from the file into them.
  frame_data_init();

  // Grab the next frame from the file and decode
  std::string ErrorMsg;
  if (odc_next_frame(odc_frame_) == ODC_SUCCESS) {
    // Start a new decoder
    ErrorMsg = "OdcIO::ReadFrame: Unable to start a new ODC decoder";
    CheckOdcCall(odc_new_decoder(&odc_decoder_), ErrorMsg);
    ErrorMsg = "OdcIO::ReadFrame: Unable to configure ODC decoder";
    CheckOdcCall(odc_decoder_defaults_from_frame(odc_decoder_, odc_frame_), ErrorMsg);

    // Run the decoder and point frame_data_ to the resulting table.
    // The table contains 2D double values with the frame's data values.
    long RowsDecoded;
    ErrorMsg = "OdcIO::ReadFrame: Unable to run ODC decoder";
    CheckOdcCall(odc_decode(odc_decoder_, odc_frame_, &RowsDecoded), ErrorMsg);
    ASSERT(RowsDecoded == FrameSize);

    long TableWidth;
    long TableHeight;
    bool TableColMajor;
    ErrorMsg = "OdcIO::ReadFrame: Unable to run ODC decoder";
    CheckOdcCall(odc_decoder_data_array(odc_decoder_, (const void**) &odc_frame_data_,
                     &TableWidth, &TableHeight, &TableColMajor), ErrorMsg);
    ASSERT(TableHeight == FrameSize);
    ASSERT(TableWidth/sizeof(double) == num_odc_cols_);
  } else {
    std::string ErrorMsg = "OdcIO::ReadFrame: Cannot access next frame in the file";
    ABORT(ErrorMsg);
  }

  // Convert and copy odc frame data into the IodaIO frame containers.
  for (IodaIO::GroupIter igrp = group_begin(); igrp != group_end(); ++igrp) {
    std::string GroupName = group_name(igrp);
    for (IodaIO::VarIter ivar = var_begin(igrp); ivar != var_end(igrp); ++ivar) {
      // Variables are all the same length, and they line up with the frame sizes.
      std::string VarName = var_name(ivar);
      std::string VarType = var_dtype(ivar);
      std::size_t VarId = var_id(ivar);

      if (VarType == "int") {
        std::vector<int> FileData(FrameSize, 0);
        OdcReadVar(VarId, FileData);
        int_frame_data_->put_data(GroupName, VarName, FileData);
      } else if ((VarType == "float") || (VarType == "double")) {
        std::vector<float> FileData(FrameSize, 0.0);
        OdcReadVar(VarId, FileData);
        float_frame_data_->put_data(GroupName, VarName, FileData);
      } else if (VarType == "string") {
        bool IsDateTime = ((GroupName == "MetaData") && (VarName == "datetime"));
        std::vector<std::string> FileData(FrameSize, "");
        OdcReadVar(VarId, FileData, IsDateTime);
        string_frame_data_->put_data(GroupName, VarName, FileData);
      }
    }
  }
  ErrorMsg = "OdcIO::ReadFrame: Unable to free the ODC decoder";
  CheckOdcCall(odc_free_decoder(odc_decoder_), ErrorMsg);
}

// -----------------------------------------------------------------------------
/*!
 * \brief Write data from the frame containers into the file
 *
 */

void OdcIO::WriteFrame(IodaIO::FrameIter & iframe) {
}

// -----------------------------------------------------------------------------
/*!
 * \brief Add entry to the group, variable info container
 *
 */

void OdcIO::GrpVarInsert(const std::string & GroupName, const std::string & VarName,
                 const std::string & VarType, const std::vector<std::size_t> & VarShape,
                 const std::string & FileVarName, const std::string & FileType,
                 const std::size_t MaxStringSize) {
  std::vector<std::size_t> FileShape = VarShape;
  if (FileType == "string") {
    FileShape.push_back(MaxStringSize);
  }

  std::size_t VarId = var_id_get(FileVarName);
  grp_var_info_[GroupName][VarName].var_id = VarId;
  grp_var_info_[GroupName][VarName].dtype = VarType;
  grp_var_info_[GroupName][VarName].file_shape = FileShape;
  grp_var_info_[GroupName][VarName].file_name = FileVarName;
  grp_var_info_[GroupName][VarName].file_type = FileType;
  grp_var_info_[GroupName][VarName].shape = VarShape;
}

// -----------------------------------------------------------------------------
/*!
 * \brief Get the var id associated with the given name
 *
 */

std::size_t OdcIO::var_id_get(const std::string & GrpVarName) {
  std::size_t VarId;
  OdcIO::VarIdIter ivar = var_ids_.find(GrpVarName);
  if (ivar != var_ids_.end()) {
    VarId = ivar->second;
  } else {
    std::string ErrorMsg = "Cannot find variable id for: " + GrpVarName;
    ABORT(ErrorMsg);
  }
  return VarId;
}

}  // namespace ioda
