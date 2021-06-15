/*
 * (C) Crown Copyright Met Office 2021
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef ODBQUERYPARAMETERS_H_
#define ODBQUERYPARAMETERS_H_

#include <string>
#include <utility>
#include <vector>

#include "ioda/Variables/Variable.h"
#include "oops/util/AnyOf.h"
#include "oops/util/parameters/OptionalParameter.h"
#include "oops/util/parameters/Parameter.h"
#include "oops/util/parameters/Parameters.h"
#include "oops/util/parameters/ParameterTraitsAnyOf.h"
#include "oops/util/parameters/RequiredParameter.h"

namespace eckit {
  class Configuration;
}

namespace ioda {

enum class StarParameter {
  ALL
};

struct StarParameterTraitsHelper{
  typedef StarParameter EnumType;
  static constexpr char enumTypeName[] = "StarParameter";
  static constexpr util::NamedEnumerator<StarParameter> namedValues[] = {
    { StarParameter::ALL, "ALL" }
  };
};

}  // namespace ioda

namespace oops {

template<>
struct ParameterTraits<ioda::StarParameter> :
    public EnumParameterTraits<ioda::StarParameterTraitsHelper>
{};

}  // namespace oops

namespace ioda {

class ODBVariableParameters : public oops::Parameters {
  OOPS_CONCRETE_PARAMETERS(ODBVariableParameters, Parameters)

 public:
  /// The column to use to match the conditions
  oops::RequiredParameter<std::string> name{"name", this};

  /// Select locations at which the condition variable is greater than or equal to the specified
  /// value. Can be set to an int, float or datetime in the ISO 8601 format (if any datetime
  /// components are zero, they are ignored).
  oops::OptionalParameter<util::AnyOf<int, float, util::PartialDateTime>> minvalue{
    "min value", this
  };

  /// Select locations at which the condition variable is less than or equal to the specified
  /// value. Can be set to an int, float or datetime in the ISO 8601 format (if any datetime
  /// components are zero, they are ignored).
  oops::OptionalParameter<util::AnyOf<int, float, util::PartialDateTime>> maxvalue{
    "max value", this};

  /// Select locations at which the condition variable is not set to the missing value indicator.
  oops::OptionalParameter<bool> isDefined{"is defined", this};
};

class OdbWhereParameters : public oops::Parameters {
  OOPS_CONCRETE_PARAMETERS(OdbWhereParameters, Parameters)

 public:
  /// The varnos to query data from
  oops::RequiredParameter<util::AnyOf<StarParameter, std::vector<int>>> varno{
      "varno", this};

  /// Variables to use to filter data from queried varnos
  oops::OptionalParameter<std::vector<ODBVariableParameters>> variable{ "variable", this };
};

}  // namespace ioda

#endif  // ODBQUERYPARAMETERS_H_
