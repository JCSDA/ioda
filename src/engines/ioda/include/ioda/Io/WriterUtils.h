#pragma once
/*
 * (C) Copyright 2022 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// \file WriterUtils.h
/// \brief Utilities for a ioda io writer backend

#include <algorithm>
#include <gsl/gsl-lite.hpp>
#include <memory>
#include <set>
#include <utility>
#include <vector>

#include "ioda/defs.h"

namespace ioda {
    class Group;
    class WriterPool;

/// @brief Transfer group contents from in memory group to a file group using an io pool
/// @param ioPool ioda WriterPool object
/// @param memGroup is the source in memory group
/// @param fileGroup is the destination file group
/// @param isParallelIo true if writing the output file in parallel IO mode
IODA_DL void ioWriteGroup(const ioda::WriterPool & ioPool, const ioda::Group& memGroup,
                          ioda::Group& fileGroup, const bool isParallelIo);

}  // namespace ioda
