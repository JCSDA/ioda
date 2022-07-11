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
    class IoPool;

/// @brief Copy attributes from src to dest. Ignore duplicates, dimension scales, and NetCDF crud.
/// @param src is the source.
/// @param dest is the destination.
IODA_DL void copyAttributes(const ioda::Has_Attributes& src, ioda::Has_Attributes& dest);

/// @brief Transfer group contents from in memory group to a file group using an io pool
/// @param ioPool ioda IoPool object
/// @param memGroup is the source in memory group
/// @param fileGroup is the destination file group
IODA_DL void ioWriteGroup(const ioda::IoPool & ioPool, const ioda::Group& memGroup,
                          ioda::Group& fileGroup);

}  // namespace ioda
