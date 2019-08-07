/*
 * (C) Copyright 2017 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "distribution/DistributionFactory.h"
#include "distribution/InefficientDistribution.h"
#include "distribution/RoundRobin.h"

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
 * \param[in] Gnlocs Total number of locations (before distribution)
 * \param[in] Method Name of the method of distribution of obs.
 * \param[in] Groups Observation grouping by locations.
 */
Distribution * DistributionFactory::createDistribution(const eckit::mpi::Comm & Comm,
                                    const std::size_t Gnlocs, const std::string & Method,
                                    const std::vector<std::size_t> & Groups) {
  if (Method == "RoundRobin") {
    if (Groups.size() == 0) {
      return new RoundRobin(Comm, Gnlocs);
    } else {
      return new RoundRobin(Comm, Gnlocs, Groups);
    }
  } else if (Method == "InefficientDistribution") {
    return new InefficientDistribution(Comm, Gnlocs);
  } else {
    return NULL;
  }
}

// -----------------------------------------------------------------------------

}  // namespace ioda
