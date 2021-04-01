#pragma once
/*
 * (C) Copyright 2021 Met Office UK
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// \file Layout_ObsGroup_ODB_Params.h
/// \brief Defines all of the information which should be stored in the YAML mapping file.

#include <string>
#include <vector>

#include "oops/util/parameters/OptionalParameter.h"
#include "oops/util/parameters/Parameter.h"
#include "oops/util/parameters/Parameters.h"
#include "oops/util/parameters/RequiredParameter.h"

namespace ioda {
namespace detail {

class VariableParameters : public oops::Parameters {
OOPS_CONCRETE_PARAMETERS(VariableParameters, Parameters)
 public:
  oops::RequiredParameter<std::string> name {"name", this};
  oops::RequiredParameter<std::string> source {"source", this};
};

class ComplementaryVariablesParameters : public oops::Parameters {
OOPS_CONCRETE_PARAMETERS(ComplementaryVariablesParameters, Parameters)
 public:
  oops::RequiredParameter<std::string> outputName {"output name", this};
  oops::Parameter<std::string> outputVariableDataType {
    "output variable data type", "string", this};
  oops::RequiredParameter<std::vector<std::string>> inputNames {"input names", this};
  oops::Parameter<std::string> mergeMethod {"merge method", "concat", this};
};

class ODBLayoutParameters : public oops::Parameters {
OOPS_CONCRETE_PARAMETERS(ODBLayoutParameters, Parameters)

 public:
  oops::OptionalParameter<std::vector<VariableParameters>> variables {"variables", this};
  oops::OptionalParameter<std::vector<ComplementaryVariablesParameters>>
  complementaryVariables {"complementary variables", this};
};

}  // namespace detail
}  // namespace ioda
