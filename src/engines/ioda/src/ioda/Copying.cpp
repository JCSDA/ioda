/*
 * (C) Copyright 2022 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// \file Copying.cpp
/// \brief Generic copying facility

#include <functional>
#include <numeric>
#include <unordered_set>

#include "eckit/mpi/Comm.h"

#include "ioda/Attributes/Attribute.h"
#include "ioda/Attributes/AttrUtils.h"
#include "ioda/Copying.h"
#include "ioda/Exception.h"
#include "ioda/Group.h"
#include "ioda/Variables/Has_Variables.h"
#include "ioda/Variables/Variable.h"
#include "ioda/Variables/VarUtils.h"

namespace ioda {

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

void copyAttributes(const ioda::Has_Attributes& src, ioda::Has_Attributes& dest) {
  using namespace ioda;
  using namespace std;
  vector<pair<string, Attribute>> srcAtts = src.openAll();

  for (const auto &s : srcAtts) {
    // Some attributes need to be ignored (such as netcdf special attributes and
    // attributes holding information about dimensions) instead of copied.
    if (AttrUtils::ignoreThisAttribute(s.first)) continue;
    if (dest.exists(s.first)) continue;

    AttrUtils::forAnySupportedAttributeType(
        s.second,
        [&](auto typeDiscriminator) {
            typedef decltype(typeDiscriminator) T;
            transferAttribute<T>(s.first, s.second, dest);
        },
        AttrUtils::ThrowIfAttributeIsOfUnsupportedType(s.first));
  }
}

template<typename VarType>
void makeVariable(const std::string & varName, const Variable & srcVar,
                  Has_Variables & destVars, Variable & destVar) {
    VariableCreationParameters params = srcVar.getCreationParameters(false, false);
    Dimensions varDims = srcVar.getDimensions();
    destVar = destVars.create<VarType>(varName, varDims, params);
    copyAttributes(srcVar.atts, destVar.atts);
}

// specialization for string
template<>
void makeVariable<std::string>(const std::string & varName, const Variable & srcVar,
                  Has_Variables & destVars, Variable & destVar) {
    // If the source variable has an "_orig_fill_value" attribute, then
    // override the destination fill value with this value.
    VariableCreationParameters params = srcVar.getCreationParameters(false, false);
    if (srcVar.atts.exists("_orig_fill_value")) {
        std::string fillValue;
        srcVar.atts.open("_orig_fill_value").read<std::string>(fillValue);
        params.setFillValue<std::string>(fillValue);
    }

    Dimensions varDims = srcVar.getDimensions();
    destVar = destVars.create<std::string>(varName, varDims, params);
    copyAttributes(srcVar.atts, destVar.atts);
}

template <typename VarType>
void copyVariableData(const Variable & srcVar, Variable & destVar) {
    std::vector<VarType> varData;
    srcVar.read<VarType>(varData);
    destVar.write<VarType>(varData);
}

void createAndCopyVariable(const std::string & varName, const Variable & srcVar,
                           Has_Variables & destVars, Variable & destVar) {
    // Make the variable
    VarUtils::forAnySupportedVariableType(
        srcVar,
        [&](auto typeDiscriminator) {
            typedef decltype(typeDiscriminator) T;
            makeVariable<T>(varName, srcVar, destVars, destVar);
         },
         VarUtils::ThrowIfVariableIsOfUnsupportedType(varName));

    // transfer the variable data
    VarUtils::forAnySupportedVariableType(
        srcVar,
        [&](auto typeDiscriminator) {
            typedef decltype(typeDiscriminator) T;
            copyVariableData<T>(srcVar, destVar);
         },
         VarUtils::ThrowIfVariableIsOfUnsupportedType(varName));
}

void copyGroup(const ioda::Group & src, ioda::Group & dest) {
    // Copy this group and all child groups
    copyAttributes(src.atts, dest.atts);
    for (auto & childGroupName : src.listObjects<ObjectType::Group>(true)) {
        Group destGroup = dest.create(childGroupName);
        Group srcGroup = src.open(childGroupName);
        copyAttributes(srcGroup.atts, destGroup.atts);
    }

    // Collect variable, dimension information for the rest of the group contents.
    // collectVarDimInfo is used so that we search only once through the source
    // group for variables and their associated dimensions (which is a known performance
    // bottleneck).
    VarUtils::Vec_Named_Variable varList, dimVarList;
    VarUtils::VarDimMap dimsAttachedToVars;
    Dimensions_t maxVarSize0;
    VarUtils::collectVarDimInfo(src, varList, dimVarList, dimsAttachedToVars, maxVarSize0);

    // Dimension variables
    for (auto & namedVar : dimVarList) {
        std::string varName = namedVar.name;
        Variable srcVar = namedVar.var;

        // copy the variable
        Variable destVar;
        createAndCopyVariable(varName, srcVar, dest.vars, destVar);

        // Mark the destination variable as a dimension scale
        destVar.setIsDimensionScale(srcVar.getDimensionScaleName());
    }

    // Regular variables
    for (auto & namedVar : varList) {
        std::string varName = namedVar.name;
        Variable srcVar = namedVar.var;

        // copy the variable
        Variable destVar;
        createAndCopyVariable(varName, srcVar, dest.vars, destVar);
    }

    // Attach all dimension scales to all variables by copying the pattern
    // collected from the source group.
    // We separate this from the variable creation (above)
    // since we use a collective call for performance.
    std::vector<std::pair<Variable, std::vector<Variable>>> dimsAttachedToNewVars;
    for (const auto &srcAttachment : dimsAttachedToVars) {
      Variable destVar = dest.vars[srcAttachment.first.name];
      std::vector<Variable> newDims;
      for (const auto &srcDim : srcAttachment.second) {
          newDims.push_back(dest.vars[srcDim.name]);
      }
      dimsAttachedToNewVars.push_back(make_pair(destVar, std::move(newDims)));
    }
    dest.vars.attachDimensionScales(dimsAttachedToNewVars);
}

}  // namespace ioda
