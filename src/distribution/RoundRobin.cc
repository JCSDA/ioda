/*
 * (C) Copyright 2017-2019 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/distribution/RoundRobin.h"

#include <algorithm>
#include <iostream>
#include <numeric>
#include <set>

#include "oops/util/Logger.h"

namespace ioda {
// -----------------------------------------------------------------------------
RoundRobin::RoundRobin(const eckit::mpi::Comm & Comm) : Distribution(Comm) {
  oops::Log::trace() << "RoundRobin constructed" << std::endl;
}

// -----------------------------------------------------------------------------
RoundRobin::~RoundRobin() {
  oops::Log::trace() << "RoundRobin destructed" << std::endl;
}

// -----------------------------------------------------------------------------
/*!
 * \brief Round-robin selector
 *
 * \details This method distributes observations according to a round-robin scheme.
 *          The round-robin scheme simply selects all locations where the modulus of
 *          the record number relative to the number of process elements equals
 *          the rank of the process element we are running on. This does a good job
 *          of distributing the observations evenly across processors which optimizes
 *          the load balancing.
 *
 * \param[in] RecNum Record number, checked if belongs on this process element
 */
bool RoundRobin::isMyRecord(std::size_t RecNum) const {
    return (RecNum % comm_.size() == comm_.rank());
}

// -----------------------------------------------------------------------------

}  // namespace ioda
