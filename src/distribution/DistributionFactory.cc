/*
 * (C) Copyright 2017 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "eckit/config/Configuration.h"
#include "ioda/distribution/DistributionFactory.h"
#include "oops/util/Logger.h"

namespace ioda {

// -----------------------------------------------------------------------------

DistributionFactory::DistributionFactory(const std::string & name) {
  if (getMakers().find(name) != getMakers().end())
    throw std::runtime_error(name + " already registered in the distribution factory");
  getMakers()[name] = this;
}

// -----------------------------------------------------------------------------

std::unique_ptr<Distribution> DistributionFactory::create(const eckit::mpi::Comm & comm,
                                                          const eckit::Configuration & config) {
  oops::Log::trace() << "Distribution::create starting" << std::endl;
  const std::string id = config.getString("distribution", "RoundRobin");
  typename std::map<std::string, DistributionFactory*>::iterator it = getMakers().find(id);
  if (it == getMakers().end())
    throw std::runtime_error(id + " does not exist in the distribution factory");
  std::unique_ptr<Distribution> distribution = it->second->make(comm, config);
  oops::Log::trace() << "Distribution::create done" << std::endl;
  return distribution;
}

// -----------------------------------------------------------------------------

}  // namespace ioda
