/*
 * (C) Copyright 2017 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include <algorithm>

#include "distribution/Distribution.h"

#include "oops/util/Logger.h"

namespace ioda {
// -----------------------------------------------------------------------------

Distribution::Distribution(const eckit::mpi::Comm & Comm, const std::size_t Gnlocs) :
    comm_(Comm), gnlocs_(Gnlocs), nlocs_(0), nrecs_(0) {
  // clear out the index vector
  indx_.clear();

  oops::Log::trace() << "Distribution constructed" << std::endl;
}

// -----------------------------------------------------------------------------

Distribution::~Distribution() {
  oops::Log::trace() << "Distribtion destructed" << std::endl;
}

// -----------------------------------------------------------------------------

}  // namespace ioda
