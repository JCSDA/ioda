#pragma once
/*
 * (C) Copyright 2025 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// \file osdfDistributeUtils.hpp
/// \brief Utilities for distributing data in IFrame (OSDF) containers across MPI tasks

#include <string>
#include <vector>

#include "ioda/containers/IFrame.h"

namespace ioda {
namespace reader {

/// @brief build a list of keys based on the obs grouping variables
/// @note This function is an OSDF adaptation of a similar function in ReaderPoolUtils.h
/// @param[in] srcFrame IFrame object holding only filtered obs source data
/// @param[in] obsGroupVarList List of grouping vars from YAML spec
/// @param[out] groupingKeys Output strings that hold tuple values based on the obs grouping vars
void osdfBuildObsGroupingKeys(const osdf::IFrame & srcFrame,
                              const std::vector<std::string> & obsGroupVarList,
                              std::vector<std::string> & groupingKeys);

/// @brief Assign record numbers based on the obs grouping (if specified)
/// @detail If obs grouping is specified, then form the groups and generate record numbers
/// accordingly. That is, one unique record number per group. If obs grouping is not specified,
/// simply generate sequential record numbers starting with zero (ie, one-to-one mapping
/// with the location indices). This function assumes the data in the IFrame has already
/// been QC filtered, so no rows need to be skipped. (eg., are outside the timing window).
/// @note This function is an OSDF adaptation of a similar function in ReaderPoolUtils.h
/// @param[in]  srcFrame       IFrame object holding only filtered obs source data
/// @param[in]  obsGroupVarList List of grouping vars from YAML spec
/// "obs space.obsdatain.obsgrouping.group variables"
/// @param[out] sourceRecNums  Assigned record number for each location/row in the IFrame
void osdfAssignRecordNumbers(const osdf::IFrame & srcFrame,
                             const std::vector<std::string> & obsGroupVarList,
                             std::vector<std::size_t> & sourceRecNums);

}  // namespace reader
}  // namespace ioda
