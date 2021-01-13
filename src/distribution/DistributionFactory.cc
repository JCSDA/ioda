/*
 * (C) Copyright 2017 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "eckit/config/Configuration.h"

#include "ioda/distribution/DistributionFactory.h"

#include "ioda/distribution/Halo.h"
#include "ioda/distribution/InefficientDistribution.h"
#include "ioda/distribution/RoundRobin.h"

namespace ioda {
// -----------------------------------------------------------------------------
/*!
 * \brief create a Distribution object
 *
 * \details This method creates a Distribution object from a specified subclass
 *          of the Distribution base class. The purpose of instantiating a subclass
 *          is to get access to a particular method of distributing obs across
 *          multiple process elements.
 *
 * \param[in] Comm Local MPI communicator
 * \param[in] Method Name of the method of distribution of obs.
 */
Distribution * DistributionFactory::createDistribution(const eckit::mpi::Comm & Comm,
                                    const eckit::Configuration & config,
                                    const std::string & Method) {
  if (Method == "RoundRobin") {
    return new RoundRobin(Comm, config);
  } else if (Method == "InefficientDistribution") {
    return new InefficientDistribution(Comm, config);
  } else if (Method == "Halo") {
    return new Halo(Comm, config);
  } else {
    return NULL;
  }
}

// -----------------------------------------------------------------------------

}  // namespace ioda
