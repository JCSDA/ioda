/*
 * (C) Crown copyright 2021, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/distribution/ReplicaOfNonoverlappingDistribution.h"

#include "oops/util/Logger.h"

namespace ioda {

// -----------------------------------------------------------------------------
// Note: we don't declare an instance of DistributionMaker<ReplicaOfNonoverlappingDistribution>,
// since this distribution must be created programmatically (not from YAML).

// -----------------------------------------------------------------------------
ReplicaOfNonoverlappingDistribution::ReplicaOfNonoverlappingDistribution(
    const eckit::mpi::Comm &comm,
    std::shared_ptr<const Distribution> master)
  : NonoverlappingDistribution(comm),
    master_(std::move(master)) {
  oops::Log::trace() << "ReplicaOfNonoverlappingDistribution constructed" << std::endl;
}

// -----------------------------------------------------------------------------
ReplicaOfNonoverlappingDistribution::~ReplicaOfNonoverlappingDistribution() {
  oops::Log::trace() << "ReplicaOfNonoverlappingDistribution destructed" << std::endl;
}

// -----------------------------------------------------------------------------
bool ReplicaOfNonoverlappingDistribution::isMyRecord(std::size_t RecNum) const {
  return master_->isMyRecord(RecNum);
}

// -----------------------------------------------------------------------------

}  // namespace ioda
