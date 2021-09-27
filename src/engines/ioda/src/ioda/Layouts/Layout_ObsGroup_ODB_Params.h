#pragma once
/*
 * (C) Copyright 2021 Met Office UK
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \defgroup ioda_cxx_layout_internal Internal Groups and Data Layout
 * \brief Private API
 * \ingroup ioda_internals
 *
 * @{
 * \file Layout_ObsGroup_ODB_Params.h
 * \brief Defines all of the information which should be stored in the YAML mapping file.
 */

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
  /// \p name is what the variable should be referred to as in ioda, including the full group
  /// hierarchy.
  oops::RequiredParameter<std::string> name {"name", this};
  /// \p source is the variable's name in the input file
  oops::RequiredParameter<std::string> source {"source", this};
  /// \p unit is the variable's unit type, for conversion to SI units. The data values will be
  /// changed according to the arithmetic conversion function if function is available.
  oops::OptionalParameter<std::string> unit {"unit", this};
};

class ComplementaryVariablesParameters : public oops::Parameters {
OOPS_CONCRETE_PARAMETERS(ComplementaryVariablesParameters, Parameters)
 public:
  /// \p outputName is the variable's name as it should be found in IODA. The full group
  /// hierarchy should be included.
  oops::RequiredParameter<std::string> outputName {"output name", this};
  /// \p outputVariableDataType is the output variable's data type. Strings are currently
  /// the only supported type.
  oops::Parameter<std::string> outputVariableDataType {
    "output variable data type", "string", this};
  /// \p inputNames are the variable names as they should be found prior to the merge.
  oops::RequiredParameter<std::vector<std::string>> inputNames {"input names", this};
  /// \p mergeMethod is the method which should be used to combine the input variables.
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
