/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include "ioda/Misc/DimensionScales.h"

#include <map>

#include "ioda/Exception.h"
#include "ioda/Types/Type.h"
#include "ioda/Variables/Variable.h"
#include "ioda/defs.h"

namespace ioda {
std::shared_ptr<NewDimensionScale_Base> NewDimensionScale(const std::string& name,
                                                                 const Type& t,
                                                          Dimensions_t size, Dimensions_t maxSize,
                                                          Dimensions_t chunkingSize) {
  return std::make_shared<NewDimensionScale_Base>(name, t, size, maxSize, chunkingSize);
}

std::shared_ptr<NewDimensionScale_Base> NewDimensionScale(const std::string& name,
                                                          const Variable& scale,
                                                          const ScaleSizes& overrides) {
  Options errOpts;
  try {
    Type typ        = scale.getType();
    const auto dims = scale.getDimensions();
    errOpts
      .add("dims.dimensionality", dims.dimensionality)
      .add("dims.numElements", dims.numElements);
    if (dims.dimensionality != 1) throw Exception("Dimensionality != 1.", ioda_Here(), errOpts);
    Dimensions_t size = (overrides.size_ != Unspecified) ? overrides.size_ : dims.dimsCur[0];
    Dimensions_t maxSize
      = (overrides.maxSize_ != Unspecified) ? overrides.maxSize_ : dims.dimsMax[0];
    Dimensions_t chunkingSize = overrides.chunkingSize_;
    if (chunkingSize == Unspecified) {
      auto chunking = scale.getChunkSizes();
      // If chunking is not declared, hint that it should be found elsewhere.
      chunkingSize = (!chunking.empty()) ? chunking[0] : Unspecified;
    }
    errOpts.add("size", size)
      .add("overrides.size_", overrides.size_)
      .add("dims.dimsCur[0]", dims.dimsCur[0])
      .add("maxSize", maxSize)
      .add("dims.dimsMax[0]", dims.dimsMax[0])
      .add("overrides.maxSize_", overrides.maxSize_)
      .add("chunkingSize", chunkingSize)
      .add("overrides.chunkingSize_", overrides.chunkingSize_)
      ;

    return NewDimensionScale(name, typ, size, maxSize, chunkingSize);
  } catch (...) {
    std::throw_with_nested(Exception("An exception occurred inside ioda.", ioda_Here(), errOpts));
  }
}

// Partially copied from IodaUtils.cpp
std::list<ioda::Named_Variable> identifyDimensionScales(
  const ioda::detail::Has_Variables_Base &hasVars, std::vector<std::string> &allVarNames) {
  // Convenience lambda to hint if a variable is a scale. This is not definitive,
  // but has a high likelihood of being correct. The idea is that dimension scales
  // will have neither "@" nor "/" in their names, whereas most other variables will.
  // This lambda returns true if the name constains neither "@" nor "/".
  auto isPossiblyScale = [](const std::string& name) -> bool {
      return (std::string::npos == name.find('@')) &&
             (std::string::npos == name.find('/')) ? true : false;
  };

  std::list<ioda::Named_Variable> dimensionScales;
  for (const auto& vname : allVarNames) {
    Variable v = hasVars.open(vname);
    const auto dims = v.getDimensions();

    // Expensive function call.
    // Only 1-D variables can be scales. Also pre-filter based on name.
    if (dims.dimensionality == 1 && isPossiblyScale(vname)) {
        if (v.isDimensionScale()) {
            (vname == "nlocs")   // true / false ternary
            ? dimensionScales.push_front(ioda::Named_Variable(vname, v))
            : dimensionScales.push_back(ioda::Named_Variable(vname, v));
        }
    }
  }

  return dimensionScales;
}

}  // namespace ioda
