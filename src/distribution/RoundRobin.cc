/*
 * (C) Copyright 2017-2019 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include <algorithm>
#include <iostream>

#include "distribution/RoundRobin.h"

#include "oops/util/Logger.h"

namespace ioda {
// -----------------------------------------------------------------------------

RoundRobin::RoundRobin(const eckit::mpi::Comm & Comm, const std::size_t Nlocs) :
      Distribution(Comm, Nlocs) {
  oops::Log::trace() << "RoundRobin constructed" << std::endl;
}

// -----------------------------------------------------------------------------

RoundRobin::~RoundRobin() {
  oops::Log::trace() << "RoundRobin destructed" << std::endl;
}

// -----------------------------------------------------------------------------
/*!
 * \brief Round-robin distribution
 *
 * \details This method distributes observations according to a round-robin scheme.
 *          The round-robin scheme simply selects all locations where the modulus of
 *          the locations index relative to the number of process elements equals
 *          the rank of the process element we are running on. This does a good job
 *          of distributing the observations evenly across processors which optimizes
 *          the load balancing.
 */
void RoundRobin::distribution() {
    // Round-Robin distributing the global total locations among comm.
    std::size_t nproc = comm_.size();
    std::size_t myproc = comm_.rank();
    for (std::size_t ii = 0; ii < nlocs_; ++ii)
        if (ii % nproc == myproc) {
            indx_.push_back(ii);
        }

    oops::Log::debug() << __func__ << " : " << indx_.size() <<
        " locations being allocated to processor with round-robin method : " << myproc<< std::endl;
}

// -----------------------------------------------------------------------------

}  // namespace ioda
