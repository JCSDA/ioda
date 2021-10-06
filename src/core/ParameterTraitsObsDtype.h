/*
 * (C) Copyright 2021 Met Office UK
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef CORE_PARAMETERTRAITSOBSDTYPE_H_
#define CORE_PARAMETERTRAITSOBSDTYPE_H_

#include <string>
#include <utility>
#include <vector>

#include "ioda/ObsSpace.h"

#include "oops/util/parameters/ParameterTraits.h"

namespace ioda {

/// Helps with the conversion of ObsDtype values to/from strings.
struct ObsDtypeParameterTraitsHelper {
  typedef ObsDtype EnumType;
  static constexpr char enumTypeName[] = "ObsDtype";
  static constexpr util::NamedEnumerator<EnumType> namedValues[] = {
    { EnumType::Float, "float" },
    { EnumType::Integer, "int" },
    { EnumType::String, "string" },
    { EnumType::DateTime, "datetime" },
    { EnumType::Bool, "bool" }
  };
};

}  // namespace ioda

namespace oops {

/// Specialization of ParameterTraits for ObsDtype.
template <>
struct ParameterTraits<ioda::ObsDtype> :
    public EnumParameterTraits<ioda::ObsDtypeParameterTraitsHelper>
{};

}  // namespace oops

#endif  // CORE_PARAMETERTRAITSOBSDTYPE_H_
