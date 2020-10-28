/*
 * (C) Copyright 2017 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include <algorithm>

#include "ioda/distribution/Distribution.h"

#include "oops/util/Logger.h"

namespace ioda {
// -----------------------------------------------------------------------------

Distribution::Distribution(const eckit::mpi::Comm & Comm) : comm_(Comm) {
  oops::Log::trace() << "Distribution constructed" << std::endl;
}

// -----------------------------------------------------------------------------

Distribution::~Distribution() {
  oops::Log::trace() << "Distribtion destructed" << std::endl;
}

// -----------------------------------------------------------------------------

}  // namespace ioda
