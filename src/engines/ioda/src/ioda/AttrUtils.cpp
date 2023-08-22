/*
 * (C) Copyright 2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include "ioda/Attributes/AttrUtils.h"

#include <set>

#include "ioda/Attributes/Has_Attributes.h"

#include "oops/util/Logger.h"

namespace ioda {
namespace AttrUtils {

/// \brief true if attribute is to be ignored
bool ignoreThisAttribute(const std::string & attrName) {
    // This set contains the names of attributes that need to be stripped off of
    // variables coming from the input file. The items in the list are related to
    // dimension scales. In general, when copying attributes, the dimension
    // associations in the output file need to be re-created since they are encoded
    // as object references.
    static const std::set<std::string> ignored_names{
        "CLASS",
        "DIMENSION_LIST",
        "NAME",
        "REFERENCE_LIST",
        "_FillValue",
        "_NCProperties",
        "_Netcdf4Coordinates",
        "_Netcdf4Dimid",
        "_nc3_strict",
        "_orig_fill_value",
        "suggested_chunk_dim"
        };

    return (ignored_names.count(attrName) != 0); 
}

//--------------------------------------------------------------------------------
template <typename AttrType>
void streamAttrValueAsYaml(const AttrType & attrValue, const std::string & indent,
                           std::stringstream & yamlStream) {
    yamlStream << indent << constants::indent8 << "value: "
               << attrValue << std::endl;
}

// string specialization - need to put in quotes around the string value to handle
// complex string values (such as the history attribute from NCO tools)
template <>
void streamAttrValueAsYaml(const std::string & attrValue, const std::string & indent,
                           std::stringstream & yamlStream) {
    yamlStream << indent << constants::indent8 << "value: \""
               << attrValue << "\"" << std::endl;
}

//--------------------------------------------------------------------------------
void listAttributesAsYaml(const ioda::Has_Attributes& atts, const std::string & indent,
                          std::stringstream & yamlStream) {
    // Walk through the list of attributes and dump out in YAML format. Use
    // the indent parameter to get the correct indentation level.
    std::vector<std::pair<std::string, Attribute>> attributes = atts.openAll();
    bool firstTime = true;
    for (const auto & attr : attributes) {
        // Some attributes should be ingored
        if (AttrUtils::ignoreThisAttribute(attr.first)) continue;

        if (firstTime) {
            yamlStream << indent << "attributes:" << std::endl;
            firstTime = false;
        }

        // write name into the string stream
        yamlStream << indent << constants::indent4 << "- attribute:" << std::endl
                   << indent << constants::indent8 << "name: " << attr.first << std::endl;

        // write attribute data type into the stream
        AttrUtils::switchOnSupportedAttributeType(
            attr.second,
            [&](int) { yamlStream << indent << constants::indent8
                                  << "data type: int" << std::endl; },
            [&](long) { yamlStream << indent << constants::indent8   // NOLINT
                                   << "data type: long" << std::endl; },
            [&](float) { yamlStream << indent << constants::indent8
                                    << "data type: float" << std::endl; },
            [&](double) { yamlStream << indent << constants::indent8
                                     << "data type: double" << std::endl; },
            [&](std::string) { yamlStream << indent << constants::indent8
                                          << "data type: string" << std::endl; },
            [&](char) { yamlStream << indent << constants::indent8
                                   << "data type: char" << std::endl; },
            AttrUtils::ThrowIfAttributeIsOfUnsupportedType(attr.first));

        // write attribute value into the stream
        AttrUtils::forAnySupportedAttributeType(
            attr.second,
            [&](auto typeDiscriminator) {
                typedef decltype(typeDiscriminator) T;
                T attrValue;
                attr.second.read<T>(attrValue);
                streamAttrValueAsYaml<T>(attrValue, indent, yamlStream);
            },
            AttrUtils::ThrowIfAttributeIsOfUnsupportedType(attr.first));
    }
}

//--------------------------------------------------------------------------------
void createAttributesFromConfig(ioda::Has_Attributes & atts,
                                const std::vector<eckit::LocalConfiguration> & attsConfig) {
    // Walk through the list of attributes and create them as you go
    // This function assumes that the attributes are scalar.
    for (size_t i = 0; i < attsConfig.size(); ++i) {
        std::string attrName = attsConfig[i].getString("attribute.name");
        std::string attrDataType = attsConfig[i].getString("attribute.data type");
        if (attrDataType == "int") {
            int attrValue = attsConfig[i].getInt("attribute.value");
            atts.add<int>(attrName, attrValue);
        } else if (attrDataType == "long") {                               // NOLINT
            long attrValue = attsConfig[i].getLong("attribute.value");     // NOLINT
            atts.add<long>(attrName, attrValue);                           // NOLINT
        } else if (attrDataType == "float") {
            float attrValue = attsConfig[i].getFloat("attribute.value");
            atts.add<float>(attrName, attrValue);
        } else if (attrDataType == "double") {
            double attrValue = attsConfig[i].getDouble("attribute.value");
            atts.add<double>(attrName, attrValue);
        } else if (attrDataType == "string") {
            std::string attrValue = attsConfig[i].getString("attribute.value");
            atts.add<std::string>(attrName, attrValue);
        } else if (attrDataType == "char") {
            // Don't have a char type in eckit::LocalConfiguration so use string
            std::string attrValue = attsConfig[i].getString("attribute.value");
            atts.add<char>(attrName, attrValue[0]);
        }
    }
}

}  // end namespace AttrUtils
}  // end namespace ioda
