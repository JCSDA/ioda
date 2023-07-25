#pragma once
/*
 * (C) Copyright 2023 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \defgroup ioda_cxx_attribute Attributes, Data Access, and Selections
 * \brief The main data storage methods and objects in IODA.
 * \ingroup ioda_cxx_api
 *
 * @{
 * \file AttrUtils.h
 * \brief Utility functions for querying attribute information.
 */

#include <string>

#include "../defs.h"

#include "eckit/config/LocalConfiguration.h"

#include "ioda/Exception.h"
#include "ioda/Attributes/Attribute.h"
#include "ioda/Attributes/Has_Attributes.h"

namespace ioda {

class Group;

namespace AttrUtils {

/// \brief A function object that can be passed to the third parameter of
/// forAnySupportedAttributeType() or switchOnAttributeType() to throw an exception
/// if the attribute is of an unsupported type.
class ThrowIfAttributeIsOfUnsupportedType {
 public:
  explicit ThrowIfAttributeIsOfUnsupportedType(
      const std::string &attrName) : attrName_(attrName) {}

  void operator()(const ioda::source_location & codeLocation) const {
    std::string ErrorMsg = std::string("Attribute '") + attrName_ +
                           std::string("' is not of any supported type");
    throw ioda::Exception(ErrorMsg.c_str(), codeLocation);
  }

 private:
  std::string attrName_;
};

/// \brief Perform an action dependent on the type of an ObsSpace attribute \p attr.
///
/// \param attr
///   Attribute expected to be of one of the types that can be stored in an ObsSpace (`int`,
///   `long`, `float`, `double`, `std::string` or `char`).
/// \param action
///   A function object callable with a single argument of any type from the list above.
///   If the attribute `attr` is of type `int`, this function will be given a
///   default-initialized `int`; if it is of type `float` the function will be given
///   a default-initialized `float`, and so on. In practice, it is likely to be a
///   generic lambda expression taking a single parameter of type auto whose value is
///   ignored, but whose type is used in the implementation.
/// \param errorHandler
///   A function object callable with a single argument of type eckit::CodeLocation, called if
///   `attr` is not of a type that can be stored in an ObsSpace.
///
/// Example of use:
///
///     Attribute sourceAttr = ...;
///     std::string attrName = ...;
///     forAnySupportedAttributeType(
///       sourceAttr,
///       [&](auto typeDiscriminator) {
///           typedef decltype(typeDiscriminator) T;  // type of the attribute sourceVar
///           obs_group_.attrs.create<T>(attrName, {1});
///       },
///       ThrowIfAttributeIsOfUnsupportedType(attrName));
template <typename Action, typename ErrorHandler>
auto forAnySupportedAttributeType(const ioda::Attribute &attr, const Action &action,
                                 const ErrorHandler &typeErrorHandler) {
  if (attr.isA<int>())
    return action(int());
  if (attr.isA<long>())        // NOLINT
    return action(long());     // NOLINT
  if (attr.isA<float>())
    return action(float());
  if (attr.isA<double>())
    return action(double());
  if (attr.isA<std::string>())
    return action(std::string());
  if (attr.isA<char>())
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
/// \param longAction
///   A function object taking an argument of type `long`, which will be called and given a
///   default-initialized `long` if the variable `var` is of type `long`.
/// \param floatAction
///   A function object taking an argument of type `float`, which will be called and given a
///   default-initialized `float` if the variable `var` is of type `float`.
/// \param doubleAction
///   A function object taking an argument of type `double`, which will be called and given a
///   default-initialized `double` if the variable `var` is of type `double`.
/// \param stringAction
///   A function object taking an argument of type `std::string`, which will be called and given a
///   default-initialized `std:::string` if the variable `var` is of type `std::string`.
/// \param charAction
///   A function object taking an argument of type `char`, which will be called and given a
///   default-initialized `char` if the variable `var` is of type `char`.
/// \param errorHandler
///   A function object callable with a single argument of type eckit::CodeLocation, called if
///   `var` is not of a type that can be stored in an ObsSpace.
template <typename IntAction, typename LongAction, typename FloatAction,
          typename DoubleAction, typename StringAction, typename CharAction,
          typename ErrorHandler>
auto switchOnSupportedAttributeType(const ioda::Attribute &attr,
                                    const IntAction &intAction,
                                    const LongAction &longAction,
                                    const FloatAction &floatAction,
                                    const DoubleAction &doubleAction,
                                    const StringAction &stringAction,
                                    const CharAction &charAction,
                                    const ErrorHandler &typeErrorHandler) {
  if (attr.isA<int>())
    return intAction(int());
  if (attr.isA<long>())              // NOLINT
    return longAction(long());     // NOLINT
  if (attr.isA<float>())
    return floatAction(float());
  if (attr.isA<double>())
    return doubleAction(double());
  if (attr.isA<std::string>())
    return stringAction(std::string());
  if (attr.isA<char>())
    return charAction(char());
  typeErrorHandler(ioda_Here());
}

/// \brief true if attribute belongs to a known set of attributes that need to be ignored
/// \param attrName attribute name
bool ignoreThisAttribute(const std::string & attrName);

/// \brief list out attributes in YAML format given a Has_Attributes container
/// \param atts Has_Attributes container
/// \param indent used for formatting the correct indent level in the output YAML
/// \param yamlStream stringstream containing YAML
void listAttributesAsYaml(const ioda::Has_Attributes& atts, const std::string & indent,
                          std::stringstream & yamlStream);

/// \brief create attributes from an eckit LocalConfiguration
/// \param atts Has_Attributes container
/// \param config eckit LocalConfiguration (list of attributes)
void createAttributesFromConfig(ioda::Has_Attributes & atts,
                                const std::vector<eckit::LocalConfiguration> & attsConfig);

}  // end namespace AttrUtils
}  // end namespace ioda
