/*
 * (C) Copyright 2017 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include <algorithm>
#include <iostream>

#include "distribution/RoundRobin.h"

namespace ioda {
// -----------------------------------------------------------------------------

RoundRobin::~RoundRobin() {}

// -----------------------------------------------------------------------------
void RoundRobin::distribution(const eckit::mpi::Comm & comm, const std::size_t gnlocs) {
    // Round-Robin distributing the global total locations among comm.
    std::size_t nproc = comm.size();
    std::size_t myproc = comm.rank();
    for (std::size_t ii = 0; ii < gnlocs; ++ii)
        if (ii % nproc == myproc) {
            indx_.push_back(ii);
        }

    oops::Log::debug() << __func__ << " : " << indx_.size() <<
        " locations being allocated to processor with round-robin method : " << myproc<< std::endl;
}

// -----------------------------------------------------------------------------

}  // namespace ioda
