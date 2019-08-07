/*
 * (C) Copyright 2017-2019 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include <algorithm>
#include <iostream>
#include <numeric>

#include "distribution/RoundRobin.h"

#include "oops/util/Logger.h"

namespace ioda {
// -----------------------------------------------------------------------------

// Constructor with default obs grouping
RoundRobin::RoundRobin(const eckit::mpi::Comm & Comm, const std::size_t Nlocs) :
      Distribution(Comm, Nlocs) {
  // This constructor treats every location as a separate group (ie, no grouping).
  // To accomplish this, fill the group_number vector with the values 0..(Nlocs-1).
  group_numbers_.resize(nlocs_);
  std::iota(group_numbers_.begin(), group_numbers_.end(), 0);

  oops::Log::trace() << "RoundRobin(comm, nlocs) constructed" << std::endl;
}

// Constructor with specified obs grouping
RoundRobin::RoundRobin(const eckit::mpi::Comm & Comm, const std::size_t Nlocs,
                       const std::vector<std::size_t> & Groups) :
      Distribution(Comm, Nlocs), group_numbers_(Groups) {
  oops::Log::trace() << "RoundRobin(comm, nlocs, groups) constructed" << std::endl;
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
    for (std::size_t ii = 0; ii < nlocs_; ++ii) {
        if (group_numbers_[ii] % nproc == myproc) {
            indx_.push_back(ii);
        }
    }

    oops::Log::debug() << __func__ << " : " << indx_.size() <<
        " locations being allocated to processor with round-robin method : " << myproc<< std::endl;
}

// -----------------------------------------------------------------------------

}  // namespace ioda
