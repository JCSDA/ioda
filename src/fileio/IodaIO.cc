/*
 * (C) Copyright 2017 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#include <string>

#include "oops/parallel/mpi/mpi.h"
#include "oops/util/Logger.h"

#include "fileio/IodaIO.h"

namespace ioda {

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
 * \details This method returns the number of unique recoreds in the obs data.
 *          A record is an atomic unit that will remain intact during distribution
 *          across multiple process elements. An example is a single sounding in
 *          radiosonde obs data.
 */

std::size_t IodaIO::nrecs() const {
  return nrecs_;
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the number of unique variables in the obs data.
 */

std::size_t IodaIO::nvars() const {
  return nvars_;
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
 */

std::string IodaIO::group_name(IodaIO::GroupIter igrp) {
  return igrp->first;
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the begin iterator for the variables, of a
 *          particular group, contained in the group, variable information map.
 */

IodaIO::VarIter IodaIO::var_begin(GroupIter igrp) {
  return igrp->second.begin();
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the end iterator for the variables, of a
 *          particular group, contained in the group, variable information map.
 */

IodaIO::VarIter IodaIO::var_end(GroupIter igrp) {
  return igrp->second.end();
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the variable name for the current iteration
 *          in the group, variable information map.
 */

std::string IodaIO::var_name(IodaIO::VarIter ivar) {
  return ivar->first;
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the variable data type for the current iteration
 *          in the group, variable information map.
 */

std::string IodaIO::var_dtype(IodaIO::VarIter ivar) {
  return ivar->second.dtype;
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the variable data type for the current iteration
 *          in the group, variable information map.
 */

std::string IodaIO::var_dtype(std::string & GroupName, std::string & VarName) {
  return grp_var_info_[GroupName][VarName].dtype;
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the variable shape for the current iteration
 *          in the group, variable information map.
 */

std::vector<std::size_t> IodaIO::var_shape(IodaIO::VarIter ivar) {
  return ivar->second.shape;
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the variable shape for the current iteration
 *          in the group, variable information map.
 */

std::vector<std::size_t> IodaIO::var_shape(std::string & GroupName, std::string & VarName) {
  return grp_var_info_[GroupName][VarName].shape;
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the variable size for the current iteration
 *          in the group, variable information map.
 */

std::size_t IodaIO::var_size(IodaIO::VarIter ivar) {
  return ivar->second.size;
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the variable size for the current iteration
 *          in the group, variable information map.
 */

std::size_t IodaIO::var_size(std::string & GroupName, std::string & VarName) {
  return grp_var_info_[GroupName][VarName].size;
}

}  // namespace ioda
