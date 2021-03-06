/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \addtogroup ioda_internals_engines_obsstore
 *
 * @{
 * \file ObsStore-selection.h
 * \brief Functions for transfering ioda::Selection to ObsStore
 */
#pragma once

#include <vector>

#include "./Selection.hpp"
#include "ioda/Variables/Selection.h"
#include "ioda/defs.h"

namespace ioda {
namespace Engines {
namespace ObsStore {
/// \brief translate a ioda::Selection to and ObsStore Selection
/// \ingroup ioda_internals_engines_obsstore
ioda::ObsStore::Selection createObsStoreSelection(const ioda::Selection& selection,
                                                  const std::vector<Dimensions_t>& dim_sizes);

/// \brief generate the dimension selection structure from hyperslab specs
/// \ingroup ioda_internals_engines_obsstore
void genDimSelects(const Selection::VecDimensions_t& start, const Selection::VecDimensions_t& count,
                   const Selection::VecDimensions_t& stride,
                   const Selection::VecDimensions_t& block,
                   std::vector<ioda::ObsStore::SelectSpecs>& selects);

/// \brief generate the dimension selection structure from point specs
/// \ingroup ioda_internals_engines_obsstore
void genDimSelects(const std::vector<Selection::VecDimensions_t>& points,
                   std::vector<ioda::ObsStore::SelectSpecs>& selects);

/// \brief generate the dimension selection structure from dimension index specs
/// \ingroup ioda_internals_engines_obsstore
void genDimSelects(const std::vector<Selection::SingleSelection>& actions,
                   const std::vector<Dimensions_t>& dim_sizes,
                   std::vector<ioda::ObsStore::SelectSpecs>& selects);
}  // namespace ObsStore
}  // namespace Engines
}  // namespace ioda

/// @}
