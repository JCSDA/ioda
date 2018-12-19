/*
 * (C) Copyright 2017 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef DISTRIBUTION_DISTRIBUTIONFACTORY_H_
#define DISTRIBUTION_DISTRIBUTIONFACTORY_H_

#include <string>

#include "distribution/Distribution.h"

namespace ioda {

// ---------------------------------------------------------------------

class DistributionFactory {
 public:
    Distribution * createDistribution(const std::string & method);
};

// ---------------------------------------------------------------------

}  // namespace ioda

#endif  // DISTRIBUTION_DISTRIBUTIONFACTORY_H_
