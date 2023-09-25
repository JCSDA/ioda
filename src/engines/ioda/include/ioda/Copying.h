#pragma once
/*
 * (C) Copyright 2022 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// \file Copying.h
/// \brief Generic copying facility

#include <algorithm>
#include <gsl/gsl-lite.hpp>
#include <memory>
#include <set>
#include <utility>
#include <vector>

#include "ioda/defs.h"

namespace ioda {
    class Group;
    class Has_Attributes;

/// @brief Copy attributes from src to dest. Ignore duplicates, dimension scales, and NetCDF crud.
/// @param src is the source.
/// @param dest is the destination.
IODA_DL void copyAttributes(const ioda::Has_Attributes& src, ioda::Has_Attributes& dest);

/// \brief Copy the group structure (subgroups and group attributes) from src to dest
/// \detail Note that this function only copies groups and group attributes (ie, the
/// hierarchical group structure) but does not copy variables.
/// \param src is the source group
/// \param dest is the destination group
IODA_DL void copyGroupStructure(const ioda::Group & src, ioda::Group & dest);

/// \brief Copy the entire contents of group from src to dest
/// \detail Note that this function copies everthing in the src group hierarchical tree
/// including variables
/// \param src is the source group
/// \param dest is the destination group
IODA_DL void copyGroup(const ioda::Group & src, ioda::Group & dest);

}  // namespace ioda
