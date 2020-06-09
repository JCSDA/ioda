/*
 * (C) Copyright 2017-2019 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#include <string>

#include "oops/parallel/mpi/mpi.h"
#include "oops/util/abor1_cpp.h"
#include "oops/util/Logger.h"

#include "fileio/IodaIO.h"

namespace ioda {

// -----------------------------------------------------------------------------
IodaIO::IodaIO(const std::string & FileName, const std::string & FileMode,
               const std::size_t MaxFrameSize) :
           fname_(FileName), fmode_(FileMode), num_unexpect_dtypes_(0),
           num_excess_dims_(0), max_frame_size_(MaxFrameSize) {
}

// -----------------------------------------------------------------------------
IodaIO::~IodaIO() { }

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the path to the file.
 */

std::string IodaIO::fname() const {
  return fname_;
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the mode (read, write, etc) for access to the file.
 */

std::string IodaIO::fmode() const {
  return fmode_;
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the number of unique locations in the obs data.
 */

std::size_t IodaIO::nlocs() const {
  return nlocs_;
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the number of unique variables in the obs data.
 */

std::size_t IodaIO::nvars() const {
  return nvars_;
}

/*!
 * \details This method returns whether any unexpected data types were encountered
 *          on variables from the input file.
 */

bool IodaIO::unexpected_data_types() const {
  return (num_unexpect_dtypes_ > 0);
}

/*!
 * \details This method returns whether any variables with excess dimensions were
 *          encountered when reading the input file.
 */

bool IodaIO::excess_dims() const {
  return (num_excess_dims_ > 0);
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the begin iterator for the groups contained
 *          in the group, variable information map.
 */

IodaIO::GroupIter IodaIO::group_begin() {
  return grp_var_info_.begin();
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the end iterator for the groups contained
 *          in the group, variable information map.
 */

IodaIO::GroupIter IodaIO::group_end() {
  return grp_var_info_.end();
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the group name for the current iteration
 *          in the group, variable information map.
 *
 * \param[in] igrp Group iterator for GrpVarInfoMap
 */

std::string IodaIO::group_name(IodaIO::GroupIter igrp) {
  return igrp->first;
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the begin iterator for the variables, of a
 *          particular group, contained in the group, variable information map.
 *
 * \param[in] igrp Group iterator for GrpVarInfoMap
 */

IodaIO::VarIter IodaIO::var_begin(GroupIter igrp) {
  return igrp->second.begin();
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the end iterator for the variables, of a
 *          particular group, contained in the group, variable information map.
 *
 * \param[in] igrp Group iterator for GrpVarInfoMap
 */

IodaIO::VarIter IodaIO::var_end(GroupIter igrp) {
  return igrp->second.end();
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the variable name for the current iteration
 *          in the group, variable information map.
 *
 * \param[in] ivar Variable iterator for GrpVarInfoMap
 */

std::string IodaIO::var_name(IodaIO::VarIter ivar) {
  return ivar->first;
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the variable data type for the current iteration
 *          in the group, variable information map.
 *
 * \param[in] ivar Variable iterator for GrpVarInfoMap
 */

std::string IodaIO::var_dtype(IodaIO::VarIter ivar) {
  return ivar->second.dtype;
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the variable data type for the current iteration
 *          in the group, variable information map.
 *
 * \param[in] GroupName Group key for GrpVarInfoMap
 * \param[in] VarName Variable key for GrpVarInfoMap
 */

bool IodaIO::grp_var_exists(const std::string & GroupName, const std::string & VarName) {
  bool GroupExists = false;
  bool VarExists = false;

  // Check for group, and if group exists check for the variable
  GroupIter igrp = grp_var_info_.find(GroupName);
  GroupExists = !(igrp == grp_var_info_.end());
  if (!GroupExists) {
    std::string ErrorMsg = "Group name is not available: " + GroupName;
    oops::Log::error() << ErrorMsg << std::endl;
  }

  if (GroupExists) {
    VarIter ivar = igrp->second.find(VarName);
    VarExists = !(ivar == igrp->second.end());
    if (!VarExists) {
      std::string ErrorMsg = "Group name, variable name combination is not available: " +
                   GroupName + ", " + VarName;
      oops::Log::error() << ErrorMsg << std::endl;
    }
  }

  return GroupExists & VarExists;
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the variable data type for the group name, variable
 *          name combination in the group, variable information map.
 *
 * \param[in] GroupName Group key for GrpVarInfoMap
 * \param[in] VarName Variable key for GrpVarInfoMap
 */

std::string IodaIO::var_dtype(const std::string & GroupName, const std::string & VarName) {
  if (!grp_var_exists(GroupName, VarName)) {
    std::string ErrorMsg = "Group name, variable name combination is not available: " +
                            GroupName + ", " + VarName;
    ABORT(ErrorMsg);
  }

  GroupIter igrp = grp_var_info_.find(GroupName);
  VarIter ivar = igrp->second.find(VarName);
  return ivar->second.dtype;
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the variable shape for the current iteration
 *          in the group, variable information map.
 *
 * \param[in] ivar Variable iterator for GrpVarInfoMap
 */

std::vector<std::size_t> IodaIO::var_shape(IodaIO::VarIter ivar) {
  return ivar->second.shape;
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the variable shape for the group name, variable name
 *          combination in the group, variable information map.
 *
 * \param[in] GroupName Group key for GrpVarInfoMap
 * \param[in] VarName Variable key for GrpVarInfoMap
 */

std::vector<std::size_t> IodaIO::var_shape(const std::string & GroupName,
                                           const std::string & VarName) {
  if (!grp_var_exists(GroupName, VarName)) {
    std::string ErrorMsg = "Group name, variable name combination is not available: " +
                            GroupName + ", " + VarName;
    ABORT(ErrorMsg);
  }

  GroupIter igrp = grp_var_info_.find(GroupName);
  VarIter ivar = igrp->second.find(VarName);
  return ivar->second.shape;
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the variable shape in the file for the current iteration
 *          in the group, variable information map.
 *
 * \param[in] ivar Variable iterator for GrpVarInfoMap
 */

std::vector<std::size_t> IodaIO::file_shape(IodaIO::VarIter ivar) {
  return ivar->second.file_shape;
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the variable shape in the file for the group name,
 *          variable name combination in the group, variable information map.
 *
 * \param[in] GroupName Group key for GrpVarInfoMap
 * \param[in] VarName Variable key for GrpVarInfoMap
 */

std::vector<std::size_t> IodaIO::file_shape(const std::string & GroupName,
                                           const std::string & VarName) {
  if (!grp_var_exists(GroupName, VarName)) {
    std::string ErrorMsg = "Group name, variable name combination is not available: " +
                            GroupName + ", " + VarName;
    ABORT(ErrorMsg);
  }

  GroupIter igrp = grp_var_info_.find(GroupName);
  VarIter ivar = igrp->second.find(VarName);
  return ivar->second.file_shape;
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the variable name in the file for the current iteration
 *          in the group, variable information map.
 *
 * \param[in] ivar Variable iterator for GrpVarInfoMap
 */

std::string IodaIO::file_name(IodaIO::VarIter ivar) {
  return ivar->second.file_name;
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the variable name in the file for the group name,
 *          variable name combination in the group, variable information map.
 *
 * \param[in] GroupName Group key for GrpVarInfoMap
 * \param[in] VarName Variable key for GrpVarInfoMap
 */

std::string IodaIO::file_name(const std::string & GroupName, const std::string & VarName) {
  if (!grp_var_exists(GroupName, VarName)) {
    std::string ErrorMsg = "Group name, variable name combination is not available: " +
                            GroupName + ", " + VarName;
    ABORT(ErrorMsg);
  }

  GroupIter igrp = grp_var_info_.find(GroupName);
  VarIter ivar = igrp->second.find(VarName);
  return ivar->second.file_name;
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the variable type in the file for the current iteration
 *          in the group, variable information map.
 *
 * \param[in] ivar Variable iterator for GrpVarInfoMap
 */

std::string IodaIO::file_type(IodaIO::VarIter ivar) {
  return ivar->second.file_type;
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the variable type in the file for the group name,
 *          variable name combination in the group, variable information map.
 *
 * \param[in] GroupName Group key for GrpVarInfoMap
 * \param[in] VarName Variable key for GrpVarInfoMap
 */

std::string IodaIO::file_type(const std::string & GroupName, const std::string & VarName) {
  if (!grp_var_exists(GroupName, VarName)) {
    std::string ErrorMsg = "Group name, variable name combination is not available: " +
                            GroupName + ", " + VarName;
    ABORT(ErrorMsg);
  }

  GroupIter igrp = grp_var_info_.find(GroupName);
  VarIter ivar = igrp->second.find(VarName);
  return ivar->second.file_type;
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the variable id for the current iteration
 *          in the group, variable information map.
 *
 * \param[in] ivar Variable iterator for GrpVarInfoMap
 */

std::size_t IodaIO::var_id(IodaIO::VarIter ivar) {
  return ivar->second.var_id;
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the variable id for the group name, variable
 *          name combination in the group, variable information map.
 *
 * \param[in] GroupName Group key for GrpVarInfoMap
 * \param[in] VarName Variable key for GrpVarInfoMap
 */

std::size_t IodaIO::var_id(const std::string & GroupName, const std::string & VarName) {
  if (!grp_var_exists(GroupName, VarName)) {
    std::string ErrorMsg = "Group name, variable name combination is not available: " +
                            GroupName + ", " + VarName;
    ABORT(ErrorMsg);
  }

  GroupIter igrp = grp_var_info_.find(GroupName);
  VarIter ivar = igrp->second.find(VarName);
  return ivar->second.var_id;
}

// -----------------------------------------------------------------------------
/*!
 * \details This method adds an entry to the group, variable information map.
 *
 * \param[in] GroupName Group key for GrpVarInfoMap
 * \param[in] VarName Variable key for GrpVarInfoMap
 * \param[in] 
 */

void IodaIO::grp_var_insert(const std::string & GroupName, const std::string & VarName,
                 const std::string & VarType, const std::vector<std::size_t> & VarShape,
                 const std::string & FileVarName, const std::string & FileType,
                 const std::size_t MaxStringSize) {
  GrpVarInsert(GroupName, VarName, VarType, VarShape, FileVarName, FileType, MaxStringSize);
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns a flag indicating the existence of the given
 *          dimension name. True indicates the dimension exists, false
 *          indicates the dimension does not exist.
 *
 * \param[in] name Dimension name
 */

bool IodaIO::dim_exists(const std::string & name) {
  return (dim_info_.find(name) != dim_info_.end());
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the begin iterator for the dimension container
 */

IodaIO::DimIter IodaIO::dim_begin() {
  return dim_info_.begin();
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the end iterator for the dimension container
 */

IodaIO::DimIter IodaIO::dim_end() {
  return dim_info_.end();
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the dimension name given a dimension iterator.
 *
 * \param[in] idim Dimension iterator
 */

std::string IodaIO::dim_name(IodaIO::DimIter idim) {
  return idim->first;
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the dimension id given a dimension iterator.
 *
 * \param[in] idim Dimension iterator
 */

int IodaIO::dim_id(IodaIO::DimIter idim) {
  return idim->second.id;
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the dimension size given a dimension iterator.
 *
 * \param[in] idim Dimension iterator
 */

std::size_t IodaIO::dim_size(IodaIO::DimIter idim) {
  return idim->second.size;
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the dimension size given a dimension id.
 *
 * \param[in] id Dimension id
 */

std::size_t IodaIO::dim_id_size(const int & id) {
  DimIter idim;
  for (idim = dim_info_.begin(); idim != dim_info_.end(); idim++) {
    if (id == idim->second.id) {
      break;
    }
  }

  if (idim == dim_info_.end()) {
    std::string ErrorMsg =
      "IodaIO::dim_id_size: Dimension id does not exist: " + std::to_string(id);
    ABORT(ErrorMsg);
  }

  return idim->second.size;
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the dimension name given a dimension id.
 *
 * \param[in] id Dimension id
 */

std::string IodaIO::dim_id_name(const int & id) {
  DimIter idim;
  for (idim = dim_info_.begin(); idim != dim_info_.end(); idim++) {
    if (id == idim->second.id) {
      break;
    }
  }

  if (idim == dim_info_.end()) {
    std::string ErrorMsg =
      "IodaIO::dim_id_name: Dimension id does not exist: " + std::to_string(id);
    ABORT(ErrorMsg);
  }

  return idim->first;
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the dimension size given a dimension name.
 *
 * \param[in] name Dimension name
 */

std::size_t IodaIO::dim_name_size(const std::string & name) {
  if (!dim_exists(name)) {
    std::string ErrorMsg = "IodaIO::dim_name_size: Dimension name does not exist: " + name;
    ABORT(ErrorMsg);
  }

  return dim_info_.find(name)->second.size;
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the dimension id given a dimension name.
 *
 * \param[in] name Dimension name
 */

int IodaIO::dim_name_id(const std::string & name) {
  if (!dim_exists(name)) {
    std::string ErrorMsg = "IodaIO::dim_name_id: Dimension name does not exist: " + name;
    ABORT(ErrorMsg);
  }

  return dim_info_.find(name)->second.id;
}

/*!
 * \details This method inserts an entry into the dim info container
 *
 * \param[in] name Dimension name
 */

void IodaIO::dim_insert(const std::string & Name, const std::size_t Size) {
  DimInsert(Name, Size);
}
// -----------------------------------------------------------------------------
/*!
 * \brief beginning frame iterator
 */

IodaIO::FrameIter IodaIO::frame_begin() {
  return frame_info_.begin();
}

// -----------------------------------------------------------------------------
/*!
 * \brief ending frame iterator
 */

IodaIO::FrameIter IodaIO::frame_end() {
  return frame_info_.end();
}

// -----------------------------------------------------------------------------
/*!
 * \brief initialize frame access
 */

void IodaIO::frame_initialize() {
  InitializeFrame();
}

// -----------------------------------------------------------------------------
/*!
 * \brief finalize frame access
 */

void IodaIO::frame_finalize() {
  FinalizeFrame();
}

// -----------------------------------------------------------------------------
/*!
 * \brief start value of current frame
 */

std::size_t IodaIO::frame_start(IodaIO::FrameIter & iframe) {
  return iframe->start;
}

// -----------------------------------------------------------------------------
/*!
 * \brief size value of current frame
 */

std::size_t IodaIO::frame_size(IodaIO::FrameIter & iframe) {
  return iframe->size;
}

// -----------------------------------------------------------------------------
/*!
 * \brief initialize the frame info container
 */

void IodaIO::frame_info_init(std::size_t MaxVarSize) {
  // Chop the MaxVarSize into max_frame_size_ pieces. Make sure the total
  // of the sizes of all frames adds up to MaxVarSize.
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
}

// -----------------------------------------------------------------------------
/*!
 * \brief insert item into the frame info container
 */

void IodaIO::frame_info_insert(std::size_t Start, std::size_t Size) {
  IodaIO::FrameInfoRec Finfo(Start, Size);
  frame_info_.push_back(Finfo);
}

// -----------------------------------------------------------------------------
/*!
 * \brief initialize the frame data container
 */

void IodaIO::frame_data_init() {
  int_frame_data_.reset(new ioda::FrameDataMap<int>);
  float_frame_data_.reset(new ioda::FrameDataMap<float>);
  string_frame_data_.reset(new ioda::FrameDataMap<std::string>);
}

// -----------------------------------------------------------------------------
/*!
 * \brief read from the file into the frame containers
 */

void IodaIO::frame_read(IodaIO::FrameIter & iframe) {
  ReadFrame(iframe);
}

// -----------------------------------------------------------------------------
/*!
 * \brief write from the frame containers into the file
 */

void IodaIO::frame_write(IodaIO::FrameIter & iframe) {
  WriteFrame(iframe);
}

// -----------------------------------------------------------------------------
/*!
 * \details This method extracts the group and variable names from
 *          the given compound name.
 *
 * \param[in] Name Compound name (eg, Temperature@ObsValue)
 * \param[out] GroupName Group name (eg, ObsValue)
 * \param[out] VarName Variable name (eg, Temperature)
 */

void IodaIO::ExtractGrpVarName(const std::string & Name, std::string & GroupName,
                               std::string & VarName) {
  std::size_t Spos = Name.find("@");
  if (Spos != Name.npos) {
    GroupName = Name.substr(Spos+1);
    VarName = Name.substr(0, Spos);
  } else {
    std::string ErrMsg =
      std::string("IodaIO::ExtractGrpVarName: Input file contains variables ") +
      std::string("that are missing group names (ie, no @GroupName suffix).");
    ABORT(ErrMsg);
  }
}

}  // namespace ioda
