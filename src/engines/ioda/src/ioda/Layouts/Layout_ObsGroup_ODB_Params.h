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

/// Defines the mapping between a ioda variable and an ODB column storing values dependent
/// on the observation location, but not on the observed variable (varno), like most metadata.
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
  /// \p bitIndex can be used to specify the index of a bit within a bitfield that should
  /// store the value of a Boolean variable when writing an ODB file. Currently not used;
  /// will be used by the ODB writer.
  oops::OptionalParameter<int> bitIndex {"bit index", this};
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

/// Maps a varno to a ioda variable name (without group).
class VarnoToVariableNameMappingParameters : public oops::Parameters {
OOPS_CONCRETE_PARAMETERS(VarnoToVariableNameMappingParameters, Parameters)
 public:
  /// ioda variable name. Example: \c brightness_temperature.
  oops::RequiredParameter<std::string> name {"name", this};
  /// ODB identifier of an observed variable. Example: \c 119.
  oops::RequiredParameter<int> varno {"varno", this};
  /// (Optional) The non-SI unit in which the variable values are expressed in the ODB
  /// file. These values will be converted to SI units before storing in the ioda variable.
  oops::OptionalParameter<std::string> unit {"unit", this};
};

/// Defines the mapping between a set of ioda variables and an ODB column storing values dependent
/// not just on the observation location, like most metadata, but also on the observed
/// variable (varno), like obs values, obs errors, QC flags and diagnostic flags.
class VarnoDependentColumnParameters : public oops::Parameters {
OOPS_CONCRETE_PARAMETERS(VarnoDependentColumnParameters, Parameters)
 public:
  /// ODB column name. Example: \c initial_obsvalue.
  oops::RequiredParameter<std::string> source {"source", this};
  /// Name of the ioda group containing the variables storing restrictions
  /// of the ODB column `source` to individual varnos. Example: \c ObsValue.
  oops::RequiredParameter<std::string> groupName {"group name", this};
  /// Specified the index of a bit within a bitfield that should
  /// store the value of a Boolean variable when writing an ODB file. Currently not used;
  /// will be used by the ODB writer.
  oops::OptionalParameter<int> bitIndex {"bit index", this};
  /// Maps varnos to names of variables storing restrictions of the ODB column `source` to
  /// these varnos.
  oops::Parameter<std::vector<VarnoToVariableNameMappingParameters>> mappings {
    "varno-to-variable-name mapping", {}, this};
};

class ODBLayoutParameters : public oops::Parameters {
OOPS_CONCRETE_PARAMETERS(ODBLayoutParameters, Parameters)
 public:
  oops::Parameter<std::vector<VariableParameters>> variables {"varno-independent columns", {}, this};
  oops::Parameter<std::vector<ComplementaryVariablesParameters>> complementaryVariables {
    "complementary variables", {}, this};
  oops::Parameter<std::vector<VarnoDependentColumnParameters>> varnoDependentColumns {
    "varno-dependent columns", {}, this};
};

}  // namespace detail
}  // namespace ioda
