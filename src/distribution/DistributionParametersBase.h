/*
 * (C) Copyright 2021- UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef IODA_DISTRIBUTION_DISTRIBUTIONPARAMETERSBASE_H_
#define IODA_DISTRIBUTION_DISTRIBUTIONPARAMETERSBASE_H_

#include <string>

#include "oops/util/parameters/Parameters.h"
#include "oops/util/parameters/RequiredParameter.h"

namespace ioda {

// -----------------------------------------------------------------------------
/// \brief Base class of classes storing configuration parameters of specific observation
/// distributions.
class DistributionParametersBase : public oops::Parameters {
  OOPS_ABSTRACT_PARAMETERS(DistributionParametersBase, Parameters)

 public:
  oops::RequiredParameter<std::string> name{"name", "type of the observation MPI distribution",
                                            this};
};

// -----------------------------------------------------------------------------

/// \brief A subclass of DistributionParametersBase storing no options.
/// It cannot be used for distributions that require configuration options other than "name",
/// and cannot be used to create a distribution without first setting the "name" parameter.
/// The function createBaseDistributionParams below can be used to create an instance of this class
/// with the "name" parameter set to a specified value.
class EmptyDistributionParameters : public DistributionParametersBase {
  OOPS_CONCRETE_PARAMETERS(EmptyDistributionParameters, DistributionParametersBase)
 public:
};

inline std::unique_ptr<DistributionParametersBase> createBaseDistributionParams
                                                                  (const std::string name) {
  std::unique_ptr<EmptyDistributionParameters> params =
                                                    std::make_unique<EmptyDistributionParameters>();
  eckit::LocalConfiguration config;
  config.set("name", name);
  params->deserialize(config);
  return params;
}


// -----------------------------------------------------------------------------

}  // namespace ioda

#endif  // IODA_DISTRIBUTION_DISTRIBUTIONPARAMETERSBASE_H_
