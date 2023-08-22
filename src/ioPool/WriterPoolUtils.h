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
#include <memory>
#include <set>
#include <utility>
#include <vector>

#include "ioda/defs.h"

#include <gsl/gsl-lite.hpp>

namespace ioda {

class Group;

namespace IoPool {

class WriterPoolBase;

/// @brief Transfer group contents from in memory group to a file group using an io pool
/// @param ioPool ioda WriterPoolBase object
/// @param memGroup is the source in memory group
/// @param fileGroup is the destination file group
/// @param isParallelIo true if writing the output file in parallel IO mode
IODA_DL void ioWriteGroup(const WriterPoolBase & ioPool, const ioda::Group& memGroup,
                          ioda::Group& fileGroup, const bool isParallelIo);

}  // namespace IoPool
}  // namespace ioda
