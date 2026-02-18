/*
 * (C) Crown Copyright 2025 Met Office UK
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/distribution/IdentityDistribution.h"

#include <iostream>

#include "eckit/mpi/Comm.h"
#include "ioda/distribution/DistributionFactory.h"
#include "oops/util/Logger.h"

namespace ioda {

// -----------------------------------------------------------------------------
static DistributionMaker<IdentityDistribution> maker("Identity");

// -----------------------------------------------------------------------------
IdentityDistribution::IdentityDistribution(const eckit::mpi::Comm & Comm,
                                         const Parameters_ &)
  : NonoverlappingDistribution(Comm) {
  oops::Log::trace() << "IdentityDistribution constructed" << std::endl;
}

// -----------------------------------------------------------------------------
IdentityDistribution::~IdentityDistribution() {
  oops::Log::trace() << "IdentityDistribution destructed" << std::endl;
}

// -----------------------------------------------------------------------------
std::string IdentityDistribution::name() const {
  return "Identity";
}

// -----------------------------------------------------------------------------
bool IdentityDistribution::isMyRecord(std::size_t /*RecNum*/) const {
  return true;
}

// -----------------------------------------------------------------------------

}  // namespace ioda
