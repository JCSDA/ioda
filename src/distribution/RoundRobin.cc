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
RoundRobin::RoundRobin(const eckit::mpi::Comm & Comm, const std::size_t Gnlocs) :
      Distribution(Comm, Gnlocs) {
  // This constructor treats every location as a separate record (ie, no grouping).
  // To accomplish this, fill the group_number vector with the values 0..(Gnlocs-1).
  record_numbers_.resize(gnlocs_);
  std::iota(record_numbers_.begin(), record_numbers_.end(), 0);
  oops::Log::trace() << "RoundRobin(comm, nlocs) constructed" << std::endl;
}

// Constructor with specified obs grouping
RoundRobin::RoundRobin(const eckit::mpi::Comm & Comm, const std::size_t Gnlocs,
                       const std::vector<std::size_t> & Records) :
      Distribution(Comm, Gnlocs), record_numbers_(Records) {
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
    std::size_t prev_rec_num = -1;
    nrecs_ = 0;
    for (std::size_t ii = 0; ii < gnlocs_; ++ii) {
        if (record_numbers_[ii] % nproc == myproc) {
            indx_.push_back(ii);
            recnums_.push_back(record_numbers_[ii]);
            if (prev_rec_num != record_numbers_[ii]) {
              nrecs_++;
              prev_rec_num = record_numbers_[ii];
            }
        }
    }

    // The number of locations in this distribution will be equal to the size of
    // the indx_ vector.
    nlocs_ = indx_.size();

    oops::Log::debug() << __func__ << " : " << nlocs_ <<
        " locations being allocated to processor with round-robin method : "
        << myproc<< std::endl;
}

// -----------------------------------------------------------------------------

}  // namespace ioda
