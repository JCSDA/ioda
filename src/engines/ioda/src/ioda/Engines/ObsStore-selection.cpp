/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include "ioda/Engines/ObsStore/ObsStore-selection.h"

#include <numeric>
#include <set>

#include "ioda/defs.h"

namespace ioda {
namespace Engines {
namespace ObsStore {
ioda::ObsStore::Selection createObsStoreSelection(const ioda::Selection& selection,
                                                  const std::vector<Dimensions_t>& dim_sizes) {
  ioda::ObsStore::SelectionModes mode = ioda::ObsStore::SelectionModes::ALL;
  std::vector<ioda::ObsStore::SelectSpecs> dim_selects;
  std::size_t start = 0;
  std::size_t npoints = 0;

  // If incoming mode is set to select all, then pass that information
  // along to the ObsStore selection object instead of filling up
  // the selection indices.
  //
  // Assumptions:
  //    1. Only one action is specified
  //    2. No offset specs
  if ((selection.default_ == SelectionState::ALL) && (selection.actions_.empty())) {
    // Select all points
    mode = ioda::ObsStore::SelectionModes::ALL;
    start = 0;

    // Count up the number of points
    npoints = 1;
    for (std::size_t idim = 0; idim < dim_sizes.size(); ++idim) {
      npoints *= dim_sizes[idim];
    }
  } else {
    auto first_action = selection.actions_.begin();
    if (!first_action->start_.empty()) {
      // Selection is specified as hyperslab
      mode = ioda::ObsStore::SelectionModes::INTERSECT;
      genDimSelects(first_action->start_, first_action->count_, first_action->stride_, first_action->block_,
                    dim_selects);
    } else if (!first_action->points_.empty()) {
      // Selection is specified as list of points
      mode = ioda::ObsStore::SelectionModes::POINT;
      genDimSelects(first_action->points_, dim_selects);
    } else if (!first_action->dimension_indices_starts_.empty()) {
      // Selection is specified as dimension indices
      mode = ioda::ObsStore::SelectionModes::INTERSECT;
      genDimSelects(selection.actions_, dim_sizes, dim_selects);
    } else {
      throw;  // jedi_throw.add("Reason", "Unrecongnized selection mode");
    }
  }

  if (mode == ioda::ObsStore::SelectionModes::ALL) return ioda::ObsStore::Selection{start, npoints};
  return ioda::ObsStore::Selection{mode, dim_selects, dim_sizes};
}

void genDimSelects(const Selection::VecDimensions_t& start, const Selection::VecDimensions_t& count,
                   const Selection::VecDimensions_t& stride, const Selection::VecDimensions_t& block,
                   std::vector<ioda::ObsStore::SelectSpecs>& selects) {
  // Walk through the start, count, stride, block specs and generate
  // the indices for each dimension.
  std::size_t numDims = start.size();
  selects.resize(numDims);
  for (std::size_t idim = 0; idim < numDims; ++idim) {
    // Get the start, count, stride, and block parameters for this dimension
    std::size_t dim_start = start[idim];
    std::size_t dim_count = count[idim];
    std::size_t dim_stride = 1;
    if (!stride.empty()) {
      dim_stride = stride[idim];
    }
    std::size_t dim_block = 1;
    if (!block.empty()) {
      dim_block = block[idim];
    }

    // Generate the dimension selects
    for (std::size_t i = 0; i < dim_count; ++i) {
      std::size_t block_start = dim_start + (i * dim_stride);
      for (std::size_t j = 0; j < dim_block; ++j) {
        selects[idim].push_back(block_start + j);
      }
    }
  }
}

void genDimSelects(const std::vector<Selection::VecDimensions_t>& points,
                   std::vector<ioda::ObsStore::SelectSpecs>& selects) {
  // points[0] holds indices of first point
  // points[1] holds indices of second point, etc.
  // Copy indexes in points directly to select object
  std::size_t numPoints = points.size();
  std::size_t numDims = points[0].size();
  selects.resize(numDims);
  for (std::size_t idim = 0; idim < numDims; ++idim) {
    selects[idim].resize(numPoints);
  }

  for (std::size_t ipnt = 0; ipnt < numPoints; ++ipnt) {
    for (std::size_t idim = 0; idim < numDims; ++idim) {
      selects[idim][ipnt] = points[ipnt][idim];
    }
  }
}

void genDimSelects(const std::vector<Selection::SingleSelection>& actions,
                   const std::vector<Dimensions_t>& dim_sizes,
                   std::vector<ioda::ObsStore::SelectSpecs>& selects) {
  // Initialize selects to the same rank as dim_sizes. Fill in the dimension
  // indices according to the selections actions. Then if any dimensions have
  // not been filled, set their selections to all of the indices in
  // that dimension.
  selects.resize(dim_sizes.size());

  // Each element in actions is a list of indices for a particular dimension
  for (std::size_t iact = 0; iact < actions.size(); ++iact) {
    // Create an ordered set of indices (in case the starts and counts overlap)
    // Then copy the set into the selects structure.
    std::set<std::size_t> dim_indices;
    bool haveCounts = (!actions[iact].dimension_indices_counts_.empty());
    for (std::size_t i = 0; i < actions[iact].dimension_indices_starts_.size(); ++i) {
      std::size_t idx = actions[iact].dimension_indices_starts_[i];
      if (haveCounts) {
        for (std::size_t j = 0; j < actions[iact].dimension_indices_counts_.size(); ++j) {
          dim_indices.insert(idx + j);
        }
      } else {
        dim_indices.insert(idx);
      }
    }

    // copy the set of indices into the selects structure
    std::size_t idim = actions[iact].dimension_;
    selects[idim].resize(dim_indices.size());
    std::size_t iidx = 0;
    for (auto iset = dim_indices.begin(); iset != dim_indices.end(); ++iset) {
      selects[idim][iidx] = *iset;
      iidx++;
    }
  }

  // For any unfilled dimensions, insert all indices.
  for (std::size_t idim = 0; idim < selects.size(); ++idim) {
    if (selects[idim].empty()) {
      selects[idim].resize(dim_sizes[idim]);
      std::iota(selects[idim].begin(), selects[idim].end(), 0);
    }
  }
}
}  // namespace ObsStore
}  // namespace Engines
}  // namespace ioda
