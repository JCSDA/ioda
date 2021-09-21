/*
 * (C) Crown copyright 2021, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef CORE_PARAMETERTRAITSFILEFORMAT_H_
#define CORE_PARAMETERTRAITSFILEFORMAT_H_

#include <string>
#include <utility>
#include <vector>

#include "ioda/core/FileFormat.h"

#include "oops/util/parameters/ParameterTraits.h"

namespace ioda {

/// Helps with the conversion of FileFormat values to/from strings.
struct FileFormatParameterTraitsHelper {
  typedef FileFormat EnumType;
  static constexpr char enumTypeName[] = "FileFormat";
  static constexpr util::NamedEnumerator<EnumType> namedValues[] = {
    { EnumType::AUTO, "auto" },
    { EnumType::HDF5, "hdf5" },
    { EnumType::ODB, "odb" }
  };
};

}  // namespace ioda

namespace oops {

/// Specialization of ParameterTraits for FileFormat.
template <>
struct ParameterTraits<ioda::FileFormat> :
    public EnumParameterTraits<ioda::FileFormatParameterTraitsHelper>
{};

}  // namespace oops

#endif  // CORE_PARAMETERTRAITSFILEFORMAT_H_
