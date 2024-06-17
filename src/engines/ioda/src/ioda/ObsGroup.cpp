/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include "ioda/ObsGroup.h"

#include "ioda/Engines/HH.h"
#include "ioda/Exception.h"
#include "ioda/Layout.h"
#include "ioda/Variables/VarUtils.h"

namespace ioda {
NewDimensionScale_Base::~NewDimensionScale_Base() = default;

ObsGroup::~ObsGroup() = default;

ObsGroup::ObsGroup() = default;

ObsGroup::ObsGroup(Group g, std::shared_ptr<const detail::DataLayoutPolicy> layout) : Group(g) {
  try {
    if (layout)
      this->setLayout(layout);
    else
      this->setLayout(
        detail::DataLayoutPolicy::generate(detail::DataLayoutPolicy::Policies::ObsGroup));
  } catch (...) {
    std::throw_with_nested(Exception(
      "An exception occurred inside ioda while constructing an ObsGroup.", ioda_Here()));
  }
}

void ObsGroup::setLayout(std::shared_ptr<const detail::DataLayoutPolicy> policy) {
  try {
    layout_ = policy;
    this->vars.setLayout(policy);
  } catch (...) {
    std::throw_with_nested(Exception(
      "An exception occurred inside ioda while setting a layout policy.", ioda_Here()));
  }
}

void ObsGroup::setup(const NewDimensionScales_t& fundamentalDims,
                     std::shared_ptr<const detail::DataLayoutPolicy>) {
  try {
    layout_->initializeStructure(*this);

    for (const auto& f : fundamentalDims) {
      ioda::VariableCreationParameters p;
      p.chunk = true;
      p.fChunkingStrategy
        = [&f](const ioda::Selection::VecDimensions_t&, ioda::Selection::VecDimensions_t& out) {
            out = {f->chunkingSize_};
            return true;
          };
      p.atts.add("suggested_chunk_dim", f->chunkingSize_);

      Type t = (f->dataTypeKnown_.isValid())
                 ? f->dataTypeKnown_
                 : vars.getTypeProvider()->makeFundamentalType(f->dataType_);

      auto newvar = vars.create(f->name_, t, {f->size_}, {f->maxSize_}, p);
      newvar.setIsDimensionScale(f->name_);
      f->writeInitialData(newvar);
    }
  } catch (...) {
    std::throw_with_nested(Exception(
      "An exception occurred inside ioda while building a new ObsGroup.", ioda_Here()));
  }
}

ObsGroup ObsGroup::generate(Group& emptyGroup, const NewDimensionScales_t& fundamentalDims,
                            std::shared_ptr<const detail::DataLayoutPolicy> layout) {
  try {
    ObsGroup res{emptyGroup, layout};
    res.setup(fundamentalDims, layout);

    return res;
  } catch (...) {
    std::throw_with_nested(Exception(
      "An exception occurred inside ioda while building a new ObsGroup.", ioda_Here()));
  }
}

void ObsGroup::resize(const std::vector<std::pair<Variable, ioda::Dimensions_t>>& newDims) {
  try {
    // Resize the dimension variables
    for (std::size_t i = 0; i < newDims.size(); ++i) {
      Variable var = newDims[i].first;
      var.resize({newDims[i].second});
    }
    // Recursively traverse group structure and resize all variables using
    // the given dimensions.
    resizeVars(*this, newDims);
  } catch (...) {
    std::throw_with_nested(Exception(
      "An exception occurred inside ioda while resizing an ObsGroup.", ioda_Here()));
  }
}

void ObsGroup::resizeVars(Group& g,
                          const std::vector<std::pair<Variable, ioda::Dimensions_t>>& newDims)
{
  try {
    // Visit the variables in this group, and resize according to
    // what's in the newDims vector.
    auto groupVars = g.listObjects(ObjectType::Variable, true)[ObjectType::Variable];
    for (std::size_t i = 0; i < groupVars.size(); ++i) {
      // Want to create new sizes for the variable dimesions that
      // consider both the current dimension sizes and the new
      // dimension sizes in newDims. For each dimension position,
      // if a variable in newDims is a Dimension Scale for that position
      // use the new size in newDims, otherwise use the current size.
      //
      // Do not attempt to resize Dimension Scale variables. Those have
      // been taken care of in the ObsGroup::resize function. Plus,
      // Dimension Scale variables cannot have Dimension Scales attached
      // to them, so the isDimensionScaleAttached call will throw an error
      // if attempted to be run on a Dimension Scale variable.
      Variable var = g.vars.open(groupVars[i]);
      if (!var.isDimensionScale()) {
        std::vector<Dimensions_t> varDims = var.getDimensions().dimsCur;
        std::vector<Dimensions_t> varNewDims(varDims);
        for (std::size_t idim = 0; idim < varDims.size(); ++idim) {
          for (std::size_t inewdim = 0; inewdim < newDims.size(); ++inewdim) {
            if (var.isDimensionScaleAttached(gsl::narrow<unsigned>(idim), newDims[inewdim].first)) {
              varNewDims[idim] = newDims[inewdim].second;
            }
          }
        }
        var.resize(varNewDims);
      }
    }
  } catch (...) {
    std::throw_with_nested(Exception(
      "An exception occurred inside ioda while resizing an ObsGroup.", ioda_Here()));
  }
}

//-------------------------------------------------------------------------------
void ObsGroup::append(ObsGroup & appendGroup) {
  // Do a move if this group does not have a backend yet (ie, it is empty).
  // Otherwise, do an append.
  if (this->getBackend() == nullptr) {
    *this = std::move(appendGroup);
  } else {
    // First find out how many locations will exist with the result for
    // the resize command (which opens up the space for the data that is
    // being appended
    Variable destLocVar = this->vars.open("Location");
    Variable srcLocVar = appendGroup.vars.open("Location");
    const Dimensions_t locStart = destLocVar.getDimensions().dimsCur[0];
    const Dimensions_t locCount = srcLocVar.getDimensions().dimsCur[0];
    const std::size_t finalNlocs = locStart + locCount;

    // For now, the assumption is that all of the variables that are not
    // dimensioned by Location are identical in this group and the appendGroup.
    // Same for the channel specs - the same number of channels with the
    // same channel numbering.
    // Resize along the Location dimension to open up space for the data that
    // will be appended
    this->resize({ std::pair<Variable, Dimensions_t>(destLocVar, finalNlocs) });

    // Walk through the appendGroup and for each variable that is
    // dimensioned by Location, read the new values and transfer
    // them to the space opened up by the above resize call.
    for (const auto & varName : appendGroup.listObjects<ObjectType::Variable>(true)) {
      const Variable srcVar = appendGroup.vars.open(varName);

      // skip if a dimension variable other than Location
      if (srcVar.isDimensionScale()&& (varName != "Location")) {
        continue;
      }

      // Process the variables that have Location as their first dimension.
      // Include the variable Location as well.
      if (srcVar.isDimensionScaleAttached(0, srcLocVar) || (varName == "Location")) {
        Variable destVar = this->vars.open(varName);

        // Create a selection object for the source variable. We want to include
        // the entire variable.
        const std::vector<Dimensions_t> srcVarShape = srcVar.getDimensions().dimsCur;
        const std::vector<Dimensions_t> srcCounts = srcVarShape;
        const std::vector<Dimensions_t> srcStarts(srcCounts.size(), 0);
        Selection srcSelect;
        srcSelect.extent(srcVarShape)
                 .select({ SelectionOperator::SET, srcStarts, srcCounts });

        // Create a selection object for the destination variable. We want one
        // block along the Location dimension (which is always the first dimension),
        // that consists of the number of locations that we are appending.
        const std::vector<Dimensions_t> destVarShape = destVar.getDimensions().dimsCur;
        std::vector<Dimensions_t> destCounts = destVarShape;
        destCounts[0] = locCount;
        std::vector<Dimensions_t> destStarts(destCounts.size(), 0);
        destStarts[0] = locStart;
        Selection destSelect;
        destSelect.extent(destVarShape)
                  .select({ SelectionOperator::SET, destStarts, destCounts });

        // Read and append the new data
        VarUtils::forAnySupportedVariableType(
          destVar,
          [&] (auto typeDiscriminator) {
          typedef decltype(typeDiscriminator) T;
            std::vector<T> newValues;
            srcVar.read<T>(newValues);
            destVar.write<T>(newValues, srcSelect, destSelect);
            },
            VarUtils::ThrowIfVariableIsOfUnsupportedType(varName)
          );
      }
    }
  }
}

}  // namespace ioda
