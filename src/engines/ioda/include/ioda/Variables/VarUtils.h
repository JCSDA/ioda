#pragma once
/*
 * (C) Copyright 2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \defgroup ioda_cxx_variable Variables, Data Access, and Selections
 * \brief The main data storage methods and objects in IODA.
 * \ingroup ioda_cxx_api
 *
 * @{
 * \file VarUtils.h
 * \brief Utility functions for querying variable information.
 */

#include <list>
#include <map>
#include <string>
#include <vector>

#include "../defs.h"

namespace ioda {

class Group;
class Named_Variable;
class Variable;

namespace VarUtils {

/*! @brief Convenience lambda to hint if a variable @b might be a scale.
 *
 * @details This is not definitive,
 * but has a high likelihood of being correct. The idea is that all variables
 * will have either a "@" or "/" in their names, whereas dimension scales
 * will not. This lambda returns true if the name has neither "@" nor "/" in
 * its value.
 * @param name is the variable name
 * @returns true if yes, false if no.
*/
IODA_DL bool isPossiblyScale(const std::string& name);

/*! @brief Sort variable names in a preferential way so that likely scales end up first. For speed.
* @param allVars is an unordered vector of all variables.
* @returns an ordered list. "nlocs" is first, then all potential scales, then all other variables.
*/
IODA_DL std::list<std::string> preferentialSortVariableNames(const std::vector<std::string>& allVars);

typedef std::vector<ioda::Named_Variable> Vec_Named_Variable;
typedef std::map<ioda::Named_Variable, Vec_Named_Variable> VarDimMap;

/// @brief Traverse file structure and determine dimension scales and regular variables. Also
///   determine which dimensions are attached to which variables at which dimension numbers.
/// @param[in] grp is the incoming group. Really any group works.
/// @param[out] varList is the list of variables (not dimension scales).
/// @param[out] dimVarList is the list of dimension scales.
/// @param[out] dimsAttachedToVars is the mapping of the scales attached to each variable.
/// @param[out] maxVarSize0 is the max dimension length that was detected (nlocs). Used in ioda's main code,
///   but otherwise forgettable.
IODA_DL void collectVarDimInfo(const ioda::Group& grp, Vec_Named_Variable& varList,
                       Vec_Named_Variable& dimVarList, VarDimMap& dimsAttachedToVars,
                       ioda::Dimensions_t& maxVarSize0);


}  // end namespace VarUtils
}  // end namespace ioda
