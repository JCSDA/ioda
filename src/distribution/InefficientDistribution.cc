/*
 * (C) Copyright 2017-2019 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include <algorithm>
#include <iostream>

#include "distribution/InefficientDistribution.h"

namespace ioda {
// -----------------------------------------------------------------------------

InefficientDistribution::~InefficientDistribution() {}

// -----------------------------------------------------------------------------
/*!
 * \brief Inefficient distribution
 *
 * \details This method distributes all observations to all processes.
 *
 * \param[in] comm The eckit MPI communicator object for this run
 * \param[in] gnlocs The total number of locations from the input obs file
 */
void InefficientDistribution::distribution(const eckit::mpi::Comm & comm,
                                           const std::size_t gnlocs) {
  indx_.resize(gnlocs);
  std::iota(indx_.begin(), indx_.end(), 0);
}

// -----------------------------------------------------------------------------

}  // namespace ioda
