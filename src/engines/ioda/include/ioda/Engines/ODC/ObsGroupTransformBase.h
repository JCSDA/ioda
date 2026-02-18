#pragma once
/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include <string>

#include "oops/util/parameters/Parameters.h"
#include "oops/util/parameters/RequiredParameter.h"

namespace ioda {

namespace Engines {

class ContainerFacade;

namespace ODC {

/// \brief Parameters controlling the behavior of a subclass of ObsGroupTransformBase.
class ObsGroupTransformParametersBase : public oops::Parameters {
  OOPS_ABSTRACT_PARAMETERS(ObsGroupTransformParametersBase, Parameters)

 public:
  /// \brief A string identifying the ObsGroup transform.
  oops::RequiredParameter<std::string> name{"name", this};
};

// -----------------------------------------------------------------------------

/// \brief A concrete subclass of ObsGroupTransformParametersBase with no extra parameters.
class EmptyObsGroupTransformParameters : public ObsGroupTransformParametersBase {
  OOPS_CONCRETE_PARAMETERS(EmptyObsGroupTransformParameters, ObsGroupTransformParametersBase)
};

// -----------------------------------------------------------------------------

/// \brief Applies a certain transformation to an ObsGroup or another container hidden by a
/// ContainerFacade.
///
/// Each subclass needs to typedef `Parameters_` to the type of a subclass of
/// ObsGroupTransformParametersBase storing its configuration options, and to provide a constructor
/// with the following signature:
///
///     SubclassName(const Parameters_ &transformParameters,
///                  const ODC_Parameters &odcParameters,
///                  const OdbVariableCreationParameters &variableCreationParameters);
///
/// where `transformParameters` are the transform's configuration options, `odcParameters`, the
/// ODB reader's configuration options, and `variableCreationParameters`, the configuration options
/// influencing the process of converting ODB query results into ioda variables.
///
/// A common application of ObsGroupTransforms is the construction of variables composed of data
/// held in more than one ODB column, such as datetimes or station identifiers.
class ObsGroupTransformBase {
public:
  virtual ~ObsGroupTransformBase() {}

  /// \brief Transform a given container (in-place).
  virtual void transform(ContainerFacade &container) const = 0;
};

}  // namespace ODC
}  // namespace Engines
}  // namespace ioda
