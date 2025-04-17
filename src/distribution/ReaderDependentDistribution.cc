/*
 * (C) Crown Copyright 2025 Met Office UK
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/distribution/ReaderDependentDistribution.h"

#include <iostream>

#include "eckit/mpi/Comm.h"
#include "ioda/distribution/DistributionFactory.h"
#include "oops/util/Logger.h"

namespace ioda {

// -----------------------------------------------------------------------------
static DistributionMaker<ReaderDependentDistribution> maker("ReaderDependentDistribution");

// -----------------------------------------------------------------------------
ReaderDependentDistribution::ReaderDependentDistribution(const eckit::mpi::Comm & Comm,
                                         const Parameters_ &)
  : NonoverlappingDistribution(Comm) {
  oops::Log::trace() << "ReaderDependentDistribution constructed" << std::endl;
}

// -----------------------------------------------------------------------------
ReaderDependentDistribution::~ReaderDependentDistribution() {
  oops::Log::trace() << "ReaderDependentDistribution destructed" << std::endl;
}

// -----------------------------------------------------------------------------
std::string ReaderDependentDistribution::name() const {
  return "ReaderDependentDistribution";
}

// -----------------------------------------------------------------------------
bool ReaderDependentDistribution::isMyRecord(std::size_t /*RecNum*/) const {
  return true;
}

// -----------------------------------------------------------------------------

}  // namespace ioda
