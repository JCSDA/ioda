#pragma once
/*
 * (C) Copyright 2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! @file Params.h
* @brief Parameters definitions for ioda ObsSpace validation.
*/

#include "oops/util/parameters/ConfigurationParameter.h"
#include "oops/util/parameters/HasParameters_.h"
#include "oops/util/parameters/IgnoreOtherParameters.h"
#include "oops/util/parameters/NumericConstraints.h"
#include "oops/util/parameters/OptionalParameter.h"
#include "oops/util/parameters/OptionalPolymorphicParameter.h"
#include "oops/util/parameters/Parameter.h"
#include "oops/util/parameters/ParameterTraits.h"
#include "oops/util/parameters/ParameterTraitsAnyOf.h"
#include "oops/util/parameters/ParameterTraitsScalarOrMap.h"
#include "oops/util/parameters/Parameters.h"
#include "oops/util/parameters/PolymorphicParameter.h"
#include "oops/util/parameters/PropertyJsonSchema.h"
#include "oops/util/parameters/RequiredParameter.h"
#include "oops/util/parameters/RequiredPolymorphicParameter.h"

namespace ioda_validate {

enum class Severity { Trace, Debug, Info, Warn, Error };

struct SeverityParameterTraitsHelper {
  typedef Severity EnumType;
  static const char enumTypeName[];
  static const util::NamedEnumerator<Severity> namedValues[5];
};

}  // end namespace ioda_validate

template <>
struct oops::ParameterTraits<ioda_validate::Severity>
    : public oops::EnumParameterTraits<ioda_validate::SeverityParameterTraitsHelper> {};

namespace ioda_validate {
class PolicyParameters : public oops::Parameters {
  OOPS_CONCRETE_PARAMETERS(PolicyParameters, Parameters)

 public:
  oops::Parameter<Severity> GroupsKnown{"KnownGroupNames", Severity::Warn, this};
  oops::Parameter<Severity> RequiredGroups{"RequiredGroups", Severity::Error, this};
  oops::Parameter<Severity> GroupHasRequiredAttributes{"GroupHasRequiredAttributes",
                                                       Severity::Error, this};
  oops::Parameter<Severity> GroupHasKnownAttributes{"GroupHasKnownAttributes", Severity::Warn,
                                                    this};
  oops::Parameter<Severity> GroupAllowsVariables{"GroupAllowsVariables", Severity::Error, this};
  oops::Parameter<Severity> DimensionsKnown{"KnownDimensionNames", Severity::Warn, this};
  oops::Parameter<Severity> DimensionsUseNewName{"PreferredDimensionNames", Severity::Warn, this};
  oops::Parameter<Severity> GeneralDimensionsChecks{"GeneralDimensionsChecks", Severity::Error,
                                                    this};
  oops::Parameter<Severity> RequiredDimensions{"RequiredDimensions", Severity::Error, this};
  oops::Parameter<Severity> RequiredVariables{"RequiredVariables", Severity::Error, this};
  oops::Parameter<Severity> KnownVariableNames{"KnownVariableNames", Severity::Warn, this};
  oops::Parameter<Severity> VariableUseNewName{"PreferredVariableNames", Severity::Warn, this};
  oops::Parameter<Severity> VariableCanBeMetadata{"VariableCanBeMetadata", Severity::Warn, this};
  oops::Parameter<Severity> VariableTypeCheck{"VariableTypeCheck", Severity::Trace, this};
  oops::Parameter<Severity> VariableDimensionCheck{"VariableDimensionCheck", Severity::Warn, this};
  oops::Parameter<Severity> VariableHasReqAtts{"VariableHasReqAtts", Severity::Error, this};
  oops::Parameter<Severity> VariableKnownAtts{"VariableHasKnownAtts", Severity::Warn, this};
  oops::Parameter<Severity> VariableHasValidUnits{"VariableHasValidUnits", Severity::Error, this};
  oops::Parameter<Severity> VariableHasConvertibleUnits{"VariableHasConvertibleUnits",
                                                        Severity::Error, this};
  oops::Parameter<Severity> VariableHasExactUnits{"VariableHasExactUnits", Severity::Warn, this};
  oops::Parameter<Severity> VariableOutOfExpectedRange{"VariableOutOfExpectedRange", Severity::Warn,
                                                       this};
  oops::Parameter<Severity> AttributeHasCorrectDims{"AttributeHasCorrectDims", Severity::Warn,
                                                    this};
};

}  // end namespace ioda_validate

namespace ioda_validate {

enum class Type {
  Unspecified,
  SameAsVariable,
  Float,
  Double,
  Int8,
  Int16,
  Int32,
  Int64,
  UInt8,
  UInt16,
  UInt32,
  UInt64,
  StringVLen,
  StringFixedLen,
  Datetime,
  Char,
  SChar,
  Enum
};

struct TypeParameterTraitsHelper {
  typedef Type EnumType;
  static const char enumTypeName[];
  static const util::NamedEnumerator<Type> namedValues[18];
};

}  // end namespace ioda_validate

template <>
struct oops::ParameterTraits<ioda_validate::Type>
    : public oops::EnumParameterTraits<ioda_validate::TypeParameterTraitsHelper> {};

namespace ioda_validate {
class TypeParameters : public oops::Parameters {
  OOPS_CONCRETE_PARAMETERS(TypeParameters, Parameters)
 public:
  oops::OptionalParameter<std::string> name{"Name", this};
  oops::OptionalParameter<Type> type{"Type", this};
  oops::OptionalParameter<int> length{"Length", this};
};

}  // namespace ioda_validate

namespace ioda_validate {

class AttributeParameters : public oops::Parameters {
  OOPS_CONCRETE_PARAMETERS(AttributeParameters, Parameters)
 public:
  typedef util::AnyOf<Type, TypeParameters> TypeType_;
  oops::RequiredParameter<std::vector<std::string>> attname{"Attribute", this};
  oops::OptionalParameter<TypeType_> type{"Type", this};
  oops::OptionalParameter<int> dimensionality{"Dimensionality", this};
  oops::OptionalParameter<std::vector<int>> dimensions{"Dimensions", this};
  oops::OptionalParameter<std::map<std::string, std::string>> misc{"Misc", this};
  oops::Parameter<bool> deprecated{"Deprecated", false, this};
  oops::Parameter<bool> remove{"Remove", false, this};
};

class ListReqOptionalParameters : public oops::Parameters {
  OOPS_CONCRETE_PARAMETERS(ListReqOptionalParameters, Parameters)
 public:
  oops::Parameter<std::vector<std::string>> required{"Required", {}, this};
  oops::Parameter<std::vector<std::string>> optional{"Optional", {}, this};
};

class AttributeListReqOptionalParameters : public oops::Parameters {
  OOPS_CONCRETE_PARAMETERS(AttributeListReqOptionalParameters, Parameters)
 public:
  ListReqOptionalParameters base{this};
  oops::Parameter<std::vector<std::string>> requiredNotEnum{"RequiredNotEnum", {}, this};
};

class GroupParameters : public oops::Parameters {
  OOPS_CONCRETE_PARAMETERS(GroupParameters, Parameters)
 public:
  typedef util::AnyOf<Type, TypeParameters> TypeType_;
  oops::RequiredParameter<std::vector<std::string>> grpname{"Group", this};
  oops::Parameter<ListReqOptionalParameters> atts{"Valid Attributes", ListReqOptionalParameters(),
                                                  this};
  oops::OptionalParameter<TypeType_> type{"OverrideType", this};
  oops::Parameter<bool> required{"Required", false, this};
  oops::Parameter<bool> dimensionsAllowed{"Dimension Scale Variables Allowed", false, this};
  oops::Parameter<bool> regularVariablesAllowed{"Non Dimension Scale Variables Allowed", true,
                                                this};
  oops::OptionalParameter<std::vector<std::string>> requiredvars{"Required Variables", this};
  oops::OptionalParameter<std::string> remove{"Remove", this};
};

class DimensionParameters : public oops::Parameters {
  OOPS_CONCRETE_PARAMETERS(DimensionParameters, Parameters)
 public:
  typedef util::AnyOf<Type, TypeParameters> TypeType_;
  oops::RequiredParameter<std::vector<std::string>> dimname{"Dimension", this};
  oops::Parameter<bool> required{"Required", false, this};
  oops::Parameter<bool> remove{"Remove", false, this};
  oops::OptionalParameter<TypeType_> type{"Type", this};
};

class VariableOrDefaultVarParameters : public oops::Parameters {
  OOPS_CONCRETE_PARAMETERS(VariableOrDefaultVarParameters, Parameters)
 public:
  typedef util::AnyOf<Type, TypeParameters> TypeType_;
  oops::OptionalParameter<std::vector<std::string>> dimNames{"Dimensions", this};
  oops::OptionalParameter<TypeType_> type{"Type", this};
  oops::Parameter<bool> canBeMetadata{"Metadata", false, this};
  oops::OptionalParameter<AttributeListReqOptionalParameters> atts{"Valid Attributes", this};
};

class VariableParameters : public oops::Parameters {
  OOPS_CONCRETE_PARAMETERS(VariableParameters, Parameters)
 public:
  typedef util::AnyOf<std::string, std::vector<std::string>> VarNameType_;
  oops::RequiredParameter<VarNameType_> varname{"Variable", this};
  VariableOrDefaultVarParameters base{this};
  oops::OptionalParameter<bool> forceunits{"Force Units", this};
  // oops::OptionalParameter<std::string> units{"Units", this};
  oops::Parameter<bool> remove{"Remove", false, this};
  oops::Parameter<bool> checkExactUnits{"Check Exact Units", true, this};
  oops::Parameter<std::map<std::string, std::string>> attributes{
    "Attributes", std::map<std::string, std::string>(), this};
};

class IODAvalidateParameters : public oops::Parameters {
  OOPS_CONCRETE_PARAMETERS(IODAvalidateParameters, Parameters)
 public:
  oops::Parameter<PolicyParameters> policies{"Policies", PolicyParameters{}, this};
  oops::Parameter<std::vector<AttributeParameters>> attributes{
    "Attributes", std::vector<AttributeParameters>{}, this};
  oops::Parameter<std::vector<GroupParameters>> groups{"Groups", std::vector<GroupParameters>{},
                                                       this};
  oops::Parameter<std::vector<DimensionParameters>> dimensions{
    "Dimensions", std::vector<DimensionParameters>{}, this};
  oops::Parameter<VariableOrDefaultVarParameters> vardefaults{
    "Variable Defaults", VariableOrDefaultVarParameters{}, this};
  oops::Parameter<std::vector<VariableParameters>> variables{
    "Variables", std::vector<VariableParameters>{}, this};
};

}  // end namespace ioda_validate
