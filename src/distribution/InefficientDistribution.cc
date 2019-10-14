/*
 * (C) Copyright 2017-2019 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "distribution/InefficientDistribution.h"

#include <algorithm>
#include <iostream>
#include <numeric>

#include "oops/util/Logger.h"


namespace ioda {
// -----------------------------------------------------------------------------
InefficientDistribution::InefficientDistribution(const eckit::mpi::Comm & Comm,
                                                 const std::size_t Gnlocs) :
      Distribution(Comm, Gnlocs) {
  oops::Log::trace() << "InefficientDistribution constructed" << std::endl;
}

// -----------------------------------------------------------------------------

InefficientDistribution::~InefficientDistribution() {
  oops::Log::trace() << "InefficientDistribution destructed" << std::endl;
}

// -----------------------------------------------------------------------------
/*!
 * \brief Inefficient distribution
 *
 * \details This method distributes all observations to all processes.
 *
 * \param[in] comm The eckit MPI communicator object for this run
 * \param[in] gnlocs The total number of locations from the input obs file
 */
void InefficientDistribution::distribution() {
  indx_.resize(gnlocs_);
  std::iota(indx_.begin(), indx_.end(), 0);

  recnums_.resize(gnlocs_);
  std::iota(recnums_.begin(), recnums_.end(), 0);

  // The number of locations will equal the number of global locations,
  // and the number of records will equal the number of locations (no grouping)
  nlocs_ = gnlocs_;
  nrecs_ = nlocs_;

  oops::Log::debug() << __func__ << " : " << nlocs_ <<
      " locations being allocated to processor with inefficient-distribution method : "
      << comm_.rank()<< std::endl;
}

// -----------------------------------------------------------------------------

}  // namespace ioda
