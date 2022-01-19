#pragma once
/*
 * (C) Copyright 2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! @file AttributeChecks.h
* @brief Attribute checks
*/
#include <map>
#include <string>
#include <vector>

#include "Log.h"
#include "Params.h"
#include "ioda/Attributes/Has_Attributes.h"

namespace ioda_validate {

/// @brief Checks that a container has certain required symbols.
/// @details
///   Example YAML:
///
///   ```yaml
///   Valid Attributes:
///      Required: [ "ioda_object_type", "ioda_object_version" ]
///   ```
///
///   Second example:
///
///   ```yaml
///   Required Variables: [ "latitude", "longitude" ]
///   ```
/// @param vYAMLreqIDs is a list of the required attribute / variable names, as in the YAML params
/// @param sObjNames is a set of the object names, as specified in the YAML parameters
/// @param params is the YAML parameters
/// @param res is a running total of errors and warnings caught by the checks
void requiredSymbolsCheck(const std::vector<std::string> &vYAMLreqIDs,
                          const std::set<std::string> &sObjNames,
                          const IODAvalidateParameters &params, Results &res);

/// @brief Checks that a container's attributes are appropriate for that object.
/// @details Appropriate means that the attribute is commonly paired with this variable or group.
///   Ex.: A group never has "Units". A latitude never has a "sensor" attribute.
/// @param vObjAttNames is a list of attribute names attached to the object
/// @param sYAMLreqAtts is a set of the required attribute names, as specified in the YAML params
/// @param sYAMLoptAtts is a set of the optional attribute names, as specified in the YAML params
/// @param params is the YAML parameters
/// @param res is a running total of errors and warnings caught by the checks
void appropriateAttributesCheck(const std::vector<std::string> &vObjAttNames,
                                const std::set<std::string> &sYAMLreqAtts,
                                const std::set<std::string> &sYAMLoptAtts,
                                const IODAvalidateParameters &params, Results &res);

/// @brief Checks that attributes match the definitions in the YAML spec.
/// @param YAMLattributes a map of the YAML-defined attributes (name, parameters).
/// @param vAttNames is a vector of the attribute names, as specified in the YAML parameters
/// @param atts is the container for the attributes within the file
/// @param params is the YAML parameters
/// @param res is a running total of errors and warnings caught by the checks
void matchingAttributesCheck(const std::map<std::string, AttributeParameters> &YAMLattributes,
                             const std::vector<std::string> &vAttNames,
                             const ioda::Has_Attributes &atts, const IODAvalidateParameters &params,
                             Results &res);

}  // end namespace ioda_validate
