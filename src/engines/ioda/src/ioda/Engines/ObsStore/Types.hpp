/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \addtogroup ioda_internals_engines_obsstore
 *
 * @{
 * \file Types.hpp
 * \brief Functions for ObsStore type markers
 */
#pragma once
#include <cstring>
#include <gsl/gsl-lite.hpp>
#include <memory>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <vector>

namespace ioda {
namespace ObsStore {
/// \brief ObsStore data type markers
/// \ingroup ioda_internals_engines_obsstore
/// \details ObsStore data type markers are one-for-one with C++ POD types.
///          These are needed for translating the frontend structure that holds
///          POD types to an equivalent in ObsStore. These are primarily used
///          for constructing templated objects that hold data values.
enum class ObsTypes {
  NOTYPE,

  BOOL,

  FLOAT,
  DOUBLE,
  LDOUBLE,

  SCHAR,
  SHORT,
  INT,
  LONG,
  LLONG,

  UCHAR,
  UINT,
  USHORT,
  ULONG,
  ULLONG,

  CHAR,
  WCHAR,
  CHAR16,
  CHAR32,

  STRING
};
}  // namespace ObsStore
}  // namespace ioda

/// @}
