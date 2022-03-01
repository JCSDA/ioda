/*
 * (C) Copyright 2022 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// \file Copying.cpp
/// \brief Generic copying facility

#include "ioda/Attributes/Attribute.h"
#include "ioda/Copying.h"
#include "ioda/Types/Type.h"
#include "ioda/Types/Type_Provider.h"
#include "ioda/Variables/Variable.h"
#include "ioda/Variables/VarUtils.h"

namespace ioda {

// private functions
template <typename AttrType>
void transferAttribute(const std::string & attrName, const Attribute & srcAttr,
                       Has_Attributes & destAttrs) {
    const std::vector<ioda::Dimensions_t> & attrDims = srcAttr.getDimensions().dimsCur;
    Dimensions_t numElements = srcAttr.getDimensions().numElements;

    // Use span to handle scalar case
    std::vector<AttrType> attrData(numElements);
    gsl::span<AttrType> attrSpan(attrData);
    srcAttr.read<AttrType>(attrSpan);
    destAttrs.add<AttrType>(attrName, attrSpan, attrDims);
}

template <typename VarType>
void transferVariable(const std::string & varName, const Variable & srcVar,
                       Has_Variables & destVars) {
    VariableCreationParameters params = srcVar.getCreationParameters(false, false);
    Dimensions varDims = srcVar.getDimensions();
    Dimensions_t numElements = varDims.numElements;

    // Use span to handle scalar case
    Variable destVar = destVars.create<VarType>(varName, varDims, params);
    std::vector<VarType> varData(numElements);
    gsl::span<VarType> varSpan(varData);
    srcVar.read<VarType>(varSpan);
    destVar.write<VarType>(varSpan);

    copyAttributes(srcVar.atts, destVar.atts);
}

// public functions

void copyAttributes(const ioda::Has_Attributes& src, ioda::Has_Attributes& dest) {
  using namespace ioda;
  using namespace std;
  vector<pair<string, Attribute>> srcAtts = src.openAll();

  for (const auto &s : srcAtts) {
    // This set contains the names of atttributes that need to be stripped off of
    // variables coming from the input file. The items in the list are related to
    // dimension scales. In general, when copying attributes, the dimension
    // associations in the output file need to be re-created since they are encoded
    // as object references.
    const set<string> ignored_names{
        "CLASS",
        "DIMENSION_LIST",
        "NAME",
        "REFERENCE_LIST",
        "_FillValue",
        "_NCProperties",
        "_Netcdf4Coordinates",
        "_Netcdf4Dimid",
        "_nc3_strict",
        "suggested_chunk_dim"
        };
    if (ignored_names.count(s.first)) continue;
    if (dest.exists(s.first)) continue;

    if (s.second.isA<int>()) {
        transferAttribute<int>(s.first, s.second, dest);
    } else if (s.second.isA<long>()) {                       // NOLINT
        transferAttribute<long>(s.first, s.second, dest);    // NOLINT
    } else if (s.second.isA<float>()) {
        transferAttribute<float>(s.first, s.second, dest);
    } else if (s.second.isA<double>()) {
        transferAttribute<double>(s.first, s.second, dest);
    } else if (s.second.isA<std::string>()) {
        transferAttribute<std::string>(s.first, s.second, dest);
    } else if (s.second.isA<char>()) {
        transferAttribute<char>(s.first, s.second, dest);
    } else {
        std::string ErrorMsg = std::string("Attribute '") + s.first +
                               std::string("' is not of any supported type");
        throw Exception(ErrorMsg.c_str(), ioda_Here());
    }
  }
}

void copyGroup(const ioda::Group& src, ioda::Group& dest) {
  using namespace ioda;
  using namespace std;

  // NOTE: This routine does not respect hard links for groups,
  // types, and variables. Once hard link support is added to IODA,
  // we will need an expanded listObjects function that
  // respects references.

  // Get all variable and group names
  const auto objs = src.listObjects(ObjectType::Ignored, true);

  // Make all groups and copy global group attributes.
  copyAttributes(src.atts, dest.atts);
  for (const auto &g_name : objs.at(ObjectType::Group)) {
      Group old_g = src.open(g_name);
      Group new_g = dest.create(g_name);
      copyAttributes(old_g.atts, new_g.atts);
  }

  // Make all variables and copy data and most attributes.
  // Dimension mappings & scales are handled later.
  for (const auto& var_name : objs.at(ObjectType::Variable)) {
    const Variable old_var = src.vars.open(var_name);
    if (old_var.isA<int>()) {
        transferVariable<int>(var_name, old_var, dest.vars);
    } else if (old_var.isA<float>()) {
        transferVariable<float>(var_name, old_var, dest.vars);
    } else if (old_var.isA<int64_t>()) {
        transferVariable<int64_t>(var_name, old_var, dest.vars);
    } else if (old_var.isA<char>()) {
        transferVariable<char>(var_name, old_var, dest.vars);
    } else if (old_var.isA<std::string>()) {
        transferVariable<std::string>(var_name, old_var, dest.vars);
    } else {
        std::string ErrorMsg = std::string("Variable '") + var_name +
                               std::string("' is not of any supported type");
        throw Exception(ErrorMsg.c_str(), ioda_Here());
    }
  }

  // TODO(future): Copy named types

  // TODO(future): Copy soft links and external links

  // Query old data for dimension mappings
  VarUtils::Vec_Named_Variable regularVarList, dimVarList;
  VarUtils::VarDimMap dimsAttachedToVars;
  Dimensions_t maxVarSize0;  // unused in this function
  VarUtils::collectVarDimInfo(src, regularVarList, dimVarList, dimsAttachedToVars, maxVarSize0);

  // Make new dimension scales
  for (auto& dim : dimVarList)
      dest.vars[dim.name].setIsDimensionScale(dim.var.getDimensionScaleName());

  // Attach all dimension scales to all variables.
  // We separate this from the variable creation (above)
  // since we use a collective call for performance.
  vector<pair<Variable, vector<Variable>>> dimsAttachedToNewVars;
  for (const auto &old : dimsAttachedToVars) {
    Variable new_var = dest.vars[old.first.name];
    vector<Variable> new_dims;
    for (const auto &old_dim : old.second) {
        new_dims.push_back(dest.vars[old_dim.name]);
    }
    dimsAttachedToNewVars.push_back(make_pair(new_var, std::move(new_dims)));
  }
  dest.vars.attachDimensionScales(dimsAttachedToNewVars);
}

}  // namespace ioda
