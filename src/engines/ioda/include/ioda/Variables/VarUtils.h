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

#include "ioda/Exception.h"
#include "ioda/Variables/Variable.h"

namespace ioda {

class Group;
class Named_Variable;

namespace VarUtils {

static int DefaultChunkSize = 10000;

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

/// \brief A function object that can be passed to the third parameter of
/// forAnySupportedVariableType() or switchOnVariableType() to throw an exception
/// if the variable is of an unsupported type.
class ThrowIfVariableIsOfUnsupportedType {
 public:
  explicit ThrowIfVariableIsOfUnsupportedType(
      const std::string &varName) : varName_(varName) {}

  void operator()(const ioda::source_location & codeLocation) const {
    std::string ErrorMsg = std::string("Variable '") + varName_ +
                           std::string("' is not of any supported type");
    throw ioda::Exception(ErrorMsg.c_str(), codeLocation);
  }

 private:
  std::string varName_;
};

/// \brief Perform an action dependent on the type of an ObsSpace variable \p var.
///
/// \param var
///   Variable expected to be of one of the types that can be stored in an ObsSpace (`int`,
///   `int64_t`, `float`, `std::string` or `char`).
/// \param action
///   A function object callable with a single argument of any type from the list above.
///   If the variable `var` is of type `int`, this function will be given a default-initialized
///   `int`; if it is of type `float` the function will be given a default-initialized `float`,
///   and so on. In practice, it is likely to be a generic lambda expression taking a single
///   parameter of type auto whose value is ignored, but whose type is used in the implementation.
/// \param errorHandler
///   A function object callable with a single argument of type eckit::CodeLocation, called if
///   `var` is not of a type that can be stored in an ObsSpace.
///
/// Example of use:
///
///     Variable sourceVar = ...;
///     std::string varName = ...;
///     std::vector<Variable> dimVars = ...;
///     forAnySupportedVariableType(
///       sourceVar,
///       [&](auto typeDiscriminator) {
///           typedef decltype(typeDiscriminator) T;  // type of the variable sourceVar
///           obs_frame_.vars.createWithScales<T>(varName, dimVars, VariableCreationParameters());
///       },
///       ThrowIfVariableIsOfUnsupportedType(varName));
template <typename Action, typename ErrorHandler>
auto forAnySupportedVariableType(const ioda::Variable &var, const Action &action,
                                 const ErrorHandler &typeErrorHandler) {
  if (var.isA<int>())
    return action(int());
  if (var.isA<int64_t>())
    return action(int64_t());
  if (var.isA<float>())
    return action(float());
  if (var.isA<std::string>())
    return action(std::string());
  if (var.isA<char>())
    return action(char());
  typeErrorHandler(ioda_Here());
}

/// \brief Perform an action dependent on the type of an ObsSpace variable \p var.
///
/// \param var
///   Variable expected to be of one of the types that can be stored in an ObsSpace (`int`,
///   `int64_t`, `float`, `std::string` or `char`).
/// \param intAction
///   A function object taking an argument of type `int`, which will be called and given a
///   default-initialized `int` if the variable `var` is of type `int`.
/// \param int64Action
///   A function object taking an argument of type `int64_t`, which will be called and given a
///   default-initialized `int64_t` if the variable `var` is of type `int64_t`.
/// \param floatAction
///   A function object taking an argument of type `float`, which will be called and given a
///   default-initialized `float` if the variable `var` is of type `float`.
/// \param stringAction
///   A function object taking an argument of type `std::string`, which will be called and given a
///   default-initialized `std:::string` if the variable `var` is of type `std::string`.
/// \param charAction
///   A function object taking an argument of type `char`, which will be called and given a
///   default-initialized `char` if the variable `var` is of type `char`.
/// \param errorHandler
///   A function object callable with a single argument of type eckit::CodeLocation, called if
///   `var` is not of a type that can be stored in an ObsSpace.
template <typename IntAction, typename Int64Action, typename FloatAction,
          typename StringAction, typename CharAction, typename ErrorHandler>
auto switchOnSupportedVariableType(const ioda::Variable &var,
                                   const IntAction &intAction,
                                   const Int64Action &int64Action,
                                   const FloatAction &floatAction,
                                   const StringAction &stringAction,
                                   const CharAction &charAction,
                                   const ErrorHandler &typeErrorHandler) {
  if (var.isA<int>())
    return intAction(int());
  if (var.isA<int64_t>())
    return int64Action(int64_t());
  if (var.isA<float>())
    return floatAction(float());
  if (var.isA<std::string>())
    return stringAction(std::string());
  if (var.isA<char>())
    return charAction(char());
  typeErrorHandler(ioda_Here());
}

/// \brief Perform a variable-type-dependent action for all types that can be stored in an
/// ObsSpace (`int`, `int64_t`, `float`, `std::string` or `char`).
///
/// \param action
///   A function object callable with a single argument of any type from the list above. It will
///   be called as many times as there are types of variables that can be stored in an ObsSpace;
///   each time it will received a default-initialized value of that type. In practice, it is
///   likely to be a generic lambda expression taking a single parameter of type auto whose value
///   is ignored, but whose type is used in the implementation.
template <typename Action>
void forEachSupportedVariableType(const Action &action) {
  action(int());
  action(int64_t());
  action(float());
  action(std::string());
  action(char());
}

}  // end namespace VarUtils
}  // end namespace ioda
