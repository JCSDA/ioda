/*
 * (C) Copyright 2017 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include <algorithm>
#include <iostream>

#include "ioda/Distribution.h"

namespace ioda {

// -----------------------------------------------------------------------------

void Distribution::round_robin_distribution(const eckit::mpi::Comm & comm, const int & gnlocs) {
    nproc_ = comm.size();
    myproc_ = comm.rank();

    // Randomly distributing the locations among comm.
    for (std::size_t ii = 0; ii < gnlocs; ++ii)
        if (ii % nproc_ == myproc_) {
            indx_.push_back(ii);
        }

    oops::Log::debug() << __func__ << " : " << indx_.size() <<
        " locations being randomly allocated to processor : " << myproc_ << std::endl;
}

// -----------------------------------------------------------------------------

void Distribution::erase(const std::size_t & index) {
    auto spos = std::find(indx_.begin(), indx_.end(), index);
    if (spos != indx_.end())
      indx_.erase(spos);
}

// -----------------------------------------------------------------------------

}  // namespace ioda
