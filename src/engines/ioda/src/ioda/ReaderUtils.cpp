/*
 * (C) Copyright 2022 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// \file ReaderUtils.cpp
/// \brief Utilities for a ioda io reader backend

#include "ioda/Io/ReaderUtils.h"

#include "eckit/mpi/Comm.h"

#include "ioda/Group.h"
#include "ioda/Io/ReaderPool.h"

namespace ioda {

void ioReadGroup(const ioda::ReaderPool & ioPool, const ioda::Group& fileGroup,
                 ioda::Group& memGroup, const bool isParallelIo) {
}

}  // namespace ioda
