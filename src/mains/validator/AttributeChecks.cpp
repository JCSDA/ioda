/*
 * (C) Copyright 2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! @file AttributeChecks.cpp
* @brief Attribute checks
*/
#include "AttributeChecks.h"

#include <map>
#include <string>
#include <vector>

#include "Log.h"
#include "Params.h"
#include "ioda/Attributes/Has_Attributes.h"

namespace ioda_validate {

void requiredSymbolsCheck(const std::vector<std::string> &vYAMLreqIDs,
                          const std::set<std::string> &sObjNames,
                          const IODAvalidateParameters &params, Results &res) {
  for (const auto &name : vYAMLreqIDs) {
    if (sObjNames.count(name)) {
      Log::log(Severity::Debug) << "Required identifier (attribute or variable) '" << name
                           << "' exists.\n";
    } else {
      Log::log(params.policies.value().GroupHasRequiredAttributes.value(), res)
        << "Required identifier (attribute or variable) '" << name << "' is missing.\n";
    }
  }
}

void appropriateAttributesCheck(const std::vector<std::string> &vObjAttNames,
                                const std::set<std::string> &sYAMLreqAtts,
                                const std::set<std::string> &sYAMLoptAtts,
                                const IODAvalidateParameters &params, Results &res) {
  for (const auto &attname : vObjAttNames) {
    if (sYAMLreqAtts.count(attname) || sYAMLoptAtts.count(attname)) {
      Log::log(Severity::Debug) << "Attribute '" << attname
                           << "' is listed as either a required or optional attribute.\n";
    } else {
      const std::set<std::string> Ignored{"DIMENSION_LIST", "REFERENCE_LIST", "_FillValue"};
      if (!Ignored.count(attname))
        Log::log(params.policies.value().GroupHasKnownAttributes.value(), res)
          << "Attribute '" << attname
          << "' is present but is not listed as a required or optional attribute in the spec.\n";
    }
  }
}

void matchingAttributesCheck(const std::map<std::string, AttributeParameters> &YAMLattributes,
                             const std::vector<std::string> &vAttNames,
                             const ioda::Has_Attributes &atts, const IODAvalidateParameters &params,
                             Results &res) {
  // using namespace ioda;
  // LogContext lg3("Verifying that all attributes match the YAML spec");
  for (const auto &attname : vAttNames) {
    if (YAMLattributes.count(attname)) {
      // LogContext lg3(std::string("Checking known attribute: ").append(attname));
      auto att     = atts[attname];
      auto YAMLatt = YAMLattributes.at(attname);

      // Type check
      Log::log(Severity::Trace, res) << "TODO: Implement type check.\n";

      // Dimensions check
      {
        auto attdims                = att.getDimensions();
        auto YAMLdimensionality     = YAMLatt.dimensionality.value();
        auto YAMLdims               = YAMLatt.dimensions.value();
        ioda::Dimensions_t dimensionality = -1;
        if (YAMLdimensionality || YAMLdims) {
          if (YAMLdimensionality && YAMLdims) {
            if (*YAMLdimensionality != YAMLdims->size()) {
              Log::log(params.policies.value().AttributeHasCorrectDims.value(), res)
                << "Attribute '" << attname
                << "': YAML spec has inconsistent dimensions / dimensionality parameters.\n";
            } else {
              dimensionality = *YAMLdimensionality;
            }
          } else {
            dimensionality = static_cast<ioda::Dimensions_t>((YAMLdimensionality)
                                                              ? *YAMLdimensionality
                                                              : YAMLdims->size());
          }
          if (attdims.dimensionality != dimensionality) {
            Log::log(params.policies.value().AttributeHasCorrectDims.value(), res)
              << "Attribute '" << attname << "' has the wrong dimensionality.\n";
          } else {
            Log::log(Severity::Debug, res)
              << "Attribute '" << attname << "' has the correct dimensionality.\n";
          }
        } else {
          Log::log(Severity::Debug, res) << "Attribute '" << attname
                                    << "': skipping dimension checks since dimension information "
                                       "is unspecified in the YAML file.\n";
        }
        if (YAMLdims && (dimensionality > 0)) {
          // Check that the dimension sizes match. There might have been a mismatch in
          // the dimensionality specified in the YAML compared to the dimensionality of
          // the attribute. To cover that case, just check dimension sizes up to the
          // minimum rank (dimensionality) of the YAML and attribute.
          bool haserror = false;
          size_t minDimensionality = ((dimensionality < attdims.dimensionality) ?
                                         dimensionality : attdims.dimensionality);
          for (size_t i = 0; i < minDimensionality; ++i) {
            if (attdims.dimsCur[i] != static_cast<ioda::Dimensions_t>((*YAMLdims).at(i))) {
              Log::log(params.policies.value().AttributeHasCorrectDims.value(), res)
                << "Attribute '" << attname << "' has the wrong dimensions at index " << i
                << " ( file: " << attdims.dimsCur[i] << " yaml: " << (*YAMLdims).at(i) << " ).\n";
              haserror = true;
            }
          }
          if (!haserror)
            Log::log(Severity::Debug, res) << "Attribute '"
              << attname << "' has correct dimensions.\n";
        }
      }
    } else {
      Log::log(params.policies.value().GroupHasKnownAttributes.value(), res)
        << "Attribute '" << attname << "' does not have a YAML spec.\n";
    }
  }
}

}  // end namespace ioda_validate
