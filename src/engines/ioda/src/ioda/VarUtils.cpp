/*
 * (C) Copyright 2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include <set>

#include "ioda/Variables/VarUtils.h"
#include "ioda/Group.h"
#include "ioda/Variables/Has_Variables.h"

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


}  // end namespace VarUtils
}  // end namespace ioda
