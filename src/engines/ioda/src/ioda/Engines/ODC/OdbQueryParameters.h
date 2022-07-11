#pragma once
/*
 * (C) Crown Copyright Met Office 2021
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \addtogroup ioda_cxx_engines_pub_ODC
 *
 * @{
 * \file OdbQueryParameters.h
 * \brief ODB / ODC engine bindings
 */

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
namespace Engines {
namespace ODC {

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

}  // namespace ODC
}  // namespace Engines
}  // namespace ioda

namespace oops {

template<>
struct ParameterTraits<ioda::Engines::ODC::StarParameter> :
    public EnumParameterTraits<ioda::Engines::ODC::StarParameterTraitsHelper>
{};

}  // namespace oops

namespace ioda {
namespace Engines {
namespace ODC {

class OdbVariableParameters : public oops::Parameters {
  OOPS_CONCRETE_PARAMETERS(OdbVariableParameters, Parameters)

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
  /// Optional free-form query
  oops::Parameter<std::string> query{
    "query", "", this};
};

class OdbVariableCreationParameters : public oops::Parameters {
  OOPS_CONCRETE_PARAMETERS(OdbVariableCreationParameters, Parameters)

 public:
  /// The epoch to use for DateTime variables
  oops::Parameter<std::string> epoch{"epoch",
                                     "seconds since 1970-01-01T00:00:00Z", this};

  /// The int64 missing value
  oops::Parameter<int64_t> missingInt64{"missingInt64",
                                        -9223372036854775806, this};

};

class OdbQueryParameters : public oops::Parameters {
  OOPS_CONCRETE_PARAMETERS(OdbQueryParameters, Parameters)

 public:
  /// Variables to select
  oops::Parameter<std::vector<OdbVariableParameters>> variables{ "variables", {}, this };

  /// Selection criteria
  oops::RequiredParameter<OdbWhereParameters> where{"where", this};

  /// Parameters related to variable creation
  OdbVariableCreationParameters variableCreation{this};

  /// Truncate multi-level profiles to the `numlev` parameter.
  /// This parameter only has an effect for sonde and oceansound data.
  oops::Parameter<bool> truncateProfilesToNumLev{"truncate profiles to numlev", false, this};
};

}  // namespace ODC
}  // namespace Engines
}  // namespace ioda
