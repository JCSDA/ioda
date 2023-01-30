#pragma once
/*
 * (C) Copyright 2022 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// \file ReaderUtils.h
/// \brief Utilities for a ioda io reader backend

#include "ioda/defs.h"

namespace ioda {
    class Group;
    class ReaderPool;

/// @brief Transfer group contents from a file group to a memory group using an io pool
/// @param ioPool ioda ReaderPool object
/// @param fileGroup is the source file group
/// @param memGroup is the destination memory group
/// @param isParallelIo true if reading the input file in parallel IO mode
IODA_DL void ioReadGroup(const ioda::ReaderPool & ioPool, const ioda::Group& fileGroup,
                         ioda::Group& memGroup, const bool isParallelIo);

}  // namespace ioda
