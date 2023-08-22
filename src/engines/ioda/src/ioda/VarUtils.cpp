/*
 * (C) Copyright 2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include "ioda/Variables/VarUtils.h"

#include <set>

#include "ioda/Attributes/AttrUtils.h"
#include "ioda/Group.h"
#include "ioda/Misc/DimensionScales.h"

#include "oops/util/Logger.h"
#include "oops/util/missingValues.h"

namespace ioda {
namespace VarUtils {

/// @brief Convenience set of "nlocs" & "Location", used by other functions in this namespace.
const std::set<std::string>& LocationVarNames() {
  static const std::set<std::string> names{"nlocs", "Location"};
  return names;
}

bool isPossiblyScale(const std::string& name)
{
  return (std::string::npos == name.find('@'))
    && (std::string::npos == name.find('/')) ? true : false;
}

std::list<std::string> preferentialSortVariableNames(const std::vector<std::string>& allVars) {
  std::list<std::string> sortedAllVars;
  for (const auto& name : allVars) {
    if (sortedAllVars.empty()) {
      sortedAllVars.push_back(name);
    } else {
      if (isPossiblyScale(name)) {
        auto second = sortedAllVars.begin();
        second++;
        if (LocationVarNames().count(sortedAllVars.front())) {
          sortedAllVars.insert(second, name);
        } else {
          sortedAllVars.push_front(name);
        }
      } else {
        sortedAllVars.push_back(name);
      }
    }
  }
  return sortedAllVars;
}

void collectVarDimInfo(const ioda::Group& obsGroup, Vec_Named_Variable& varList,
                       Vec_Named_Variable& dimVarList, VarDimMap& dimsAttachedToVars,
                       ioda::Dimensions_t& maxVarSize0) {
  using namespace ioda;
  // We really want to maximize performance here and avoid excessive variable
  // re-opens and closures that would kill the HDF5 backend.
  // We want to:
  // 1) separate the dimension scales from the regular variables.
  // 2) determine the maximum size along the 0-th dimension.
  // 3) determine which dimensions are attached to which variable axes.
 
  // Retrieve all variable names from the input file. Argument to listObjects is bool
  // and when true will cause listObjects to recurse through the entire Group hierarchy.
  std::vector<std::string> allVars = obsGroup.listObjects<ObjectType::Variable>(true);

  // A sorted list of all variable names that will help optimize the actual processing.
  std::list<std::string> sortedAllVars = preferentialSortVariableNames(allVars);
  
  // TODO(ryan): refactor
  // GeoVaLs fix: all variables appear at the same level, and this is problematic.
  // Detect these files and do some extra sorting.
  if (obsGroup.list().empty()) { // No Groups under the ObsGroup
    std::list<std::string> fix_known_scales, fix_known_nonscales;
    for (const auto& vname : sortedAllVars) {
      Named_Variable v{vname, obsGroup.vars.open(vname)};
      if (v.var.isDimensionScale()) {
        (LocationVarNames().count(v.name))  // true / false ternary
          ? fix_known_scales.push_front(v.name)
          : fix_known_scales.push_back(v.name);
      } else
        fix_known_nonscales.push_back(v.name);
    }
    sortedAllVars.clear();
    for (const auto& e : fix_known_scales) sortedAllVars.push_back(e);
    for (const auto& e : fix_known_nonscales) sortedAllVars.push_back(e);
  }

  // Now for the main processing loop.
  // We separate dimension scales from non-dimension scale variables.
  // We record the maximum sizes of variables.
  // We construct the in-memory mapping of dimension scales and variable axes.
  // Keep track of these to avoid re-opening the scales repeatedly.
  std::list<Named_Variable> dimension_scales;

  varList.reserve(allVars.size());
  dimVarList.reserve(allVars.size());
  maxVarSize0 = 0;
  for (const auto& vname : sortedAllVars) {
    Named_Variable v{vname, obsGroup.vars.open(vname)};
    const auto dims = v.var.getDimensions();
    if (dims.dimensionality >= 1) {
      maxVarSize0 = std::max(maxVarSize0, dims.dimsCur[0]);
    }

    // Expensive function call.
    // Only 1-D variables can be scales. Also pre-filter based on name.
    if (dims.dimensionality == 1 && isPossiblyScale(vname)) {
      if (v.var.isDimensionScale()) {
        (LocationVarNames().count(v.name))  // true / false ternary
          ? dimension_scales.push_front(v)
          : dimension_scales.push_back(v);
        dimVarList.push_back(v);

        //std::cout << "Dimension: " << v.name << " - size " << dims.numElements << "\n";
        continue;  // Move on to next variable in the for loop.
      }
    }

    // See above block. By this point in execution, we know that this variable
    // is not a dimension scale.
    varList.push_back(v);

    // Let's figure out which scales are attached to which dimensions.
    auto attached_dimensions = v.var.getDimensionScaleMappings(dimension_scales);
    std::vector<Named_Variable> dimVars;
    dimVars.reserve(dims.dimensionality);
    for (const auto& dim_scales_along_axis : attached_dimensions) {
      if (dim_scales_along_axis.empty()) {
        throw Exception("Unexpected size of dim_scales_along_axis", ioda_Here());
      }
      dimVars.push_back(dim_scales_along_axis[0]);
    }
    //std::cout << "\nVar " << v.name << ": |";
    //  for (const auto& i : dimVars) std::cout << " " << i.name << " |";

    dimsAttachedToVars.emplace(v, dimVars);
  }
  //std::cout << std::endl;
}

//-----------------------------------------------------------------------------------
void listDimensionsAsYaml(const Vec_Named_Variable & dimVarList,
                          const std::string & indent, std::stringstream & yamlStream) {

    for (const auto & dim: dimVarList) {
        // write name into the string stream
        yamlStream << indent << "- dimension:" << std::endl
                   << indent << constants::indent4 << "name: " << dim.name << std::endl;

        // write out the dimension data type in the stream
        VarUtils::switchOnSupportedVariableType(
            dim.var,
            [&](int) { yamlStream << indent << constants::indent4
                                  << "data type: int" << std::endl; },
            [&](int64_t) { yamlStream << indent << constants::indent4
                                      << "data type: int64" << std::endl; },
            [&](float) { yamlStream << indent << constants::indent4
                                    << "data type: float" << std::endl; },
            [&](std::string) { yamlStream << indent << constants::indent4
                                          << "data type: string" << std::endl; },
            [&](char) { yamlStream << indent << constants::indent4
                                   << "data type: char" << std::endl; },
            VarUtils::ThrowIfVariableIsOfUnsupportedType(dim.name));

        // write out dimension size (should always be 1D for dimensions)
        // write out an alias (*numLocations) for the Locations dimension since
        // that can change on an MPI task-by-task basis.
        ioda::Dimensions_t dimSize = dim.var.getDimensions().dimsCur[0]; 
        if (dim.name == "Location") {
            yamlStream << indent << constants::indent4 << "size: *numLocations" << std::endl;
        } else {
            yamlStream << indent << constants::indent4 << "size: " << dimSize << std::endl;
        }

        // write out dimension attributes
        AttrUtils::listAttributesAsYaml(dim.var.atts, constants::indent8, yamlStream);
    }
}

//-----------------------------------------------------------------------------------
void listVariablesAsYaml(const Vec_Named_Variable & regularVarList,
                         const VarDimMap & dimsAttachedToVars,
                         const std::string & indent, std::stringstream & yamlStream) {

    // First put in the MetaData/dateTime (epoch) style variable. This is done here
    // since the input file could contain both the offset and string format variables
    // and we want to just refer to it once.
    yamlStream << indent << "- variable:" << std::endl
               << indent << constants::indent4 << "name: MetaData/dateTime" << std::endl
               << indent << constants::indent4 << "data type: int64" << std::endl
               << indent << constants::indent4 << "dimensions: [ Location ]" << std::endl
               << indent << constants::indent4 << "attributes:" << std::endl
               << indent << constants::indent8 << "- attribute:" << std::endl
               << indent << constants::indent12 << "name: units" << std::endl
               << indent << constants::indent12 << "data type: string" << std::endl
               << indent << constants::indent12 << "value: *dtimeEpoch" << std::endl;

    // Walk through the list of regular variables and write out YAML showing
    // their name, data type, and dimension list.
    for (const auto & regVar: regularVarList) {
        // Skip over the date time variables. The current format (epoch) is included above
        // and the old datetime formats will be converted to the new epoch style variable.
        // So we always end up with MetaData/dateTime regarless of what was in the input
        // file.
        if ((regVar.name == "MetaData/time") || (regVar.name == "MetaData/datetime") ||
            (regVar.name == "MetaData/dateTime")) {
            continue;
        }

        // write name into the string stream
        yamlStream << indent << "- variable:" << std::endl
                   << indent << constants::indent4 << "name: " << regVar.name << std::endl;

        // write out the dimension data type in the stream
        VarUtils::switchOnSupportedVariableType(
            regVar.var,
            [&](int) { yamlStream << indent << constants::indent4
                                  << "data type: int" << std::endl; },
            [&](int64_t) { yamlStream << indent << constants::indent4
                                      << "data type: int64" << std::endl; },
            [&](float) { yamlStream << indent << constants::indent4
                                    << "data type: float" << std::endl; },
            [&](std::string) { yamlStream << indent << constants::indent4
                                          << "data type: string" << std::endl; },
            [&](char) { yamlStream << indent << constants::indent4
                                   << "data type: char" << std::endl; },
            VarUtils::ThrowIfVariableIsOfUnsupportedType(regVar.name));

        // write out dimension list for the variable
        const Vec_Named_Variable listOfDims = dimsAttachedToVars.at(regVar);
        yamlStream << indent << constants::indent4 << "dimensions: [ ";
        for (std::size_t i = 0; i < listOfDims.size(); ++i) {
            if (i == 0) {
                yamlStream << listOfDims[i].name;
            } else {
                yamlStream << ", " << listOfDims[i].name;
            }
        }
        yamlStream << " ]" << std::endl;

        // write out dimension attributes
        AttrUtils::listAttributesAsYaml(regVar.var.atts, constants::indent8, yamlStream);
    }
}

//--------------------------------------------------------------------------------
template <typename VarType>
void setVarCreateParamsForMem(ioda::VariableCreationParameters & params) {
    // Use the ioda engines default settings, plus shut off compression. We can visit
    // later if compression is desireable for memory, but the thinking is that we
    // are chopping up the data into small pieces for each MPI task, and enabling
    // compression will just have to do compress/decompress functions on every variable
    // access.
    params = VariableCreationParameters::defaults<VarType>();
    params.setFillValue<VarType>(util::missingValue<VarType>());

    // Don't want compression in memory (for now)
    params.noCompress();
}

//--------------------------------------------------------------------------------
void createDimensionsFromConfig(ioda::Has_Variables & vars,
                                const std::vector<eckit::LocalConfiguration> & dimConfigs) {
    // Walk through the list of dimensions and create them as you go
    // This function assumes that the attributes are scalar.
    for (size_t i = 0; i < dimConfigs.size(); ++i) {
        const std::string dimName = dimConfigs[i].getString("dimension.name");
        const ioda::Dimensions_t dimSize = dimConfigs[i].getLong("dimension.size");
        const std::string dimDataType = dimConfigs[i].getString("dimension.data type");
        oops::Log::debug() << "createDimensionsFromConfig: dimName: " << dimName << std::endl;

        // For all dimensions other than Location, set the maxDimSize to the dimSize
        // since we don't anticipate those dimensions to change, and setting the
        // maxDimSize to a fixed value helps greatly with runtime performance.
        // For the Location dimension, we eventually want to be able to append more
        // locations so give this dimension unlimited size.
        ioda::Dimensions_t maxDimSize = dimSize;
        if (dimName == "Location") {
            maxDimSize = ioda::Unlimited;
        }
        
        ioda::Variable dimVar;
        ioda::VariableCreationParameters params;
        if (dimDataType == "int") {
            setVarCreateParamsForMem<int>(params);
            dimVar = vars.create<int>(dimName, { dimSize }, { maxDimSize }, params);
        } else if (dimDataType == "int64") {
            setVarCreateParamsForMem<int64_t>(params);
            dimVar = vars.create<int64_t>(dimName, { dimSize }, { maxDimSize }, params);
        } else if (dimDataType == "float") {
            setVarCreateParamsForMem<float>(params);
            dimVar = vars.create<float>(dimName, { dimSize }, { maxDimSize }, params);
        } else if (dimDataType == "string") {
            setVarCreateParamsForMem<std::string>(params);
            dimVar = vars.create<std::string>(dimName, { dimSize }, { maxDimSize }, params);
        } else if (dimDataType == "char") {
            setVarCreateParamsForMem<char>(params);
            dimVar = vars.create<char>(dimName, { dimSize }, { maxDimSize }, params);
        }
        dimVar.setIsDimensionScale(dimName);

        // create the attributes for this variable
        std::vector<eckit::LocalConfiguration> attrConfigs;
        dimConfigs[i].get("dimension.attributes", attrConfigs);
        AttrUtils::createAttributesFromConfig(dimVar.atts, attrConfigs);
    }
}

//--------------------------------------------------------------------------------
void createVariablesFromConfig(ioda::Has_Variables & vars,
                               const std::vector<eckit::LocalConfiguration> & varConfigs) {
    // Walk through the list of variables and create them as you go
    // This function assumes that the attributes are scalar.
    for (size_t i = 0; i < varConfigs.size(); ++i) {
        const std::string varName = varConfigs[i].getString("variable.name");
        const std::string varDataType = varConfigs[i].getString("variable.data type");
        const std::vector<std::string> varDimNames =
            varConfigs[i].getStringVector("variable.dimensions");
        oops::Log::debug() << "createVariablesFromConfig: varName: " << varName << std::endl;

        // Create a vector of variables from the vars container.
        std::vector<Variable> varDims(varDimNames.size());
        for (std::size_t i = 0; i < varDimNames.size(); ++i) {
            varDims[i] = vars.open(varDimNames[i]);
        }

        ioda::Variable memVar;
        ioda::VariableCreationParameters params;
        if (varDataType == "int") {
            setVarCreateParamsForMem<int>(params);
            memVar = vars.createWithScales<int>(varName, varDims, params);
        } else if (varDataType == "int64") {
            setVarCreateParamsForMem<int64_t>(params);
            memVar = vars.createWithScales<int64_t>(varName, varDims, params);
        } else if (varDataType == "float") {
            setVarCreateParamsForMem<float>(params);
            memVar = vars.createWithScales<float>(varName, varDims, params);
        } else if (varDataType == "string") {
            setVarCreateParamsForMem<std::string>(params);
            memVar = vars.createWithScales<std::string>(varName, varDims, params);
        } else if (varDataType == "char") {
            setVarCreateParamsForMem<char>(params);
            memVar = vars.createWithScales<char>(varName, varDims, params);
        }

        // create the attributes for this variable
        std::vector<eckit::LocalConfiguration> attrConfigs;
        varConfigs[i].get("variable.attributes", attrConfigs);
        AttrUtils::createAttributesFromConfig(memVar.atts, attrConfigs);
    }
}

}  // end namespace VarUtils
}  // end namespace ioda
