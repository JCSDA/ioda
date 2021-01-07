/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include "ioda/ObsGroup.h"

#include "ioda/Engines/HH.h"
#include "ioda/Layout.h"

namespace ioda {
NewDimensionScale_Base::~NewDimensionScale_Base() = default;

ObsGroup::~ObsGroup() = default;

ObsGroup::ObsGroup() = default;

ObsGroup::ObsGroup(Group g, std::shared_ptr<const detail::DataLayoutPolicy> layout) : Group(g) {
  if (layout)
    this->setLayout(layout);
  else
    this->setLayout(detail::DataLayoutPolicy::generate(detail::DataLayoutPolicy::Policies::ObsGroup));
}

void ObsGroup::setLayout(std::shared_ptr<const detail::DataLayoutPolicy> policy) {
  layout_ = policy;
  this->vars.setLayout(policy);
}

void ObsGroup::setup(const NewDimensionScales_t& fundamentalDims,
                     std::shared_ptr<const detail::DataLayoutPolicy>) {
  layout_->initializeStructure(*this);

  for (const auto& f : fundamentalDims) {
    ioda::VariableCreationParameters p;
    p.setIsDimensionScale(f->name_);
    p.chunk = true;
    p.fChunkingStrategy = [&f](const ioda::Selection::VecDimensions_t&,
                               ioda::Selection::VecDimensions_t& out) {
      out = {f->chunkingSize_};
      return true;
    };
    p.atts.add("suggested_chunk_dim", f->chunkingSize_);
    Type t = vars.getTypeProvider()->makeFundamentalType(f->dataType_);
    auto newvar = vars.create(f->name_, t, {f->size_}, {f->maxSize_}, p);

    f->writeInitialData(newvar);
  }
}

ObsGroup ObsGroup::generate(Group& emptyGroup, const NewDimensionScales_t& fundamentalDims,
                            std::shared_ptr<const detail::DataLayoutPolicy> layout) {
  ObsGroup res{emptyGroup, layout};
  res.setup(fundamentalDims, layout);

  return res;
}

void ObsGroup::resize(const std::vector<std::pair<Variable, ioda::Dimensions_t>>& newDims) {
  // Resize the dimension variables
  for (std::size_t i = 0; i < newDims.size(); ++i) {
    Variable var = newDims[i].first;
    var.resize({newDims[i].second});
  }
  // Recursively traverse group structure and resize all variables using
  // the given dimensions.
  resizeVars(*this, newDims);
}

void ObsGroup::resizeVars(Group& g, const std::vector<std::pair<Variable, ioda::Dimensions_t>>& newDims) {
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
}
}  // namespace ioda
