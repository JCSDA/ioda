/*
 * (C) Copyright 2017 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef DISTRIBUTION_DISTRIBUTIONFACTORY_H_
#define DISTRIBUTION_DISTRIBUTIONFACTORY_H_

#include <string>
#include <vector>

#include "eckit/config/Configuration.h"
#include "eckit/geometry/Point2.h"
#include "ioda/distribution/Distribution.h"

namespace ioda {

// ---------------------------------------------------------------------
/*!
 * \brief Factory class to instantiate objects of Distribution subclasses
 *
 * \details This class provides a create method to instantiate a Distribution object
 *          containing a method for a particular manner in which to distribute obs
 *          across multiple process elements.
 *
 * \author Xin Zhang (JCSDA)
 */
class DistributionFactory {
 public:
    Distribution * createDistribution(const eckit::mpi::Comm & Comm,
                                      const eckit::Configuration & config,
                                      const std::string & Method);
};

// ---------------------------------------------------------------------

}  // namespace ioda

#endif  // DISTRIBUTION_DISTRIBUTIONFACTORY_H_
