/*
 * (C) Copyright 2021- UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef IODA_DISTRIBUTION_DISTRIBUTIONPARAMETERSBASE_H_
#define IODA_DISTRIBUTION_DISTRIBUTIONPARAMETERSBASE_H_

#include <string>

#include "oops/util/parameters/Parameter.h"
#include "oops/util/parameters/Parameters.h"

namespace ioda {

// -----------------------------------------------------------------------------
/// \brief Base class of classes storing configuration parameters of specific observation
/// distributions.
class DistributionParametersBase : public oops::Parameters {
  OOPS_ABSTRACT_PARAMETERS(DistributionParametersBase, Parameters)

 public:
  oops::Parameter<std::string> name{"name", "type of the observation MPI distribution",
                                    "RoundRobin", this};
};

// -----------------------------------------------------------------------------

/// \brief A subclass of DistributionParametersBase storing no options.
/// It can be used for distributions that do not require configuration options other than
/// "name".
class EmptyDistributionParameters : public DistributionParametersBase {
  OOPS_CONCRETE_PARAMETERS(EmptyDistributionParameters, DistributionParametersBase)
 public:
};

// -----------------------------------------------------------------------------

}  // namespace ioda

#endif  // IODA_DISTRIBUTION_DISTRIBUTIONPARAMETERSBASE_H_
