#pragma once
/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \addtogroup ioda_cxx_variable
 *
 * @{
 * \file FillPolicy.h
 * \brief Default fill values for ioda files.
 */

#include <gsl/gsl-lite.hpp>
#include <memory>
#include <string>

#include "ioda/Variables/Fill.h"
#include "ioda/defs.h"

namespace ioda {

/// \brief This option describes the default fill values that will be used if the user does not
/// manually specify a fill value.
/// \ingroup ioda_cxx_variable
enum class FillValuePolicy {
  HDF5,    ///< Set all fill values to zero or null strings.
  NETCDF4  ///< Use NetCDF4 default fill values. This is the default option for ioda files.
};

/// \brief Holds the different default fill values used in ioda files produced by different
///   backends.
/// \ingroup ioda_cxx_variable
/// \details This matters for netCDF4 vs HDF5-produced files. They have different default
///   fill values.
namespace FillValuePolicies {
template <class T>
T HDF5_default() {
  return 0;
}
template <>
inline std::string HDF5_default<std::string>() {
  return std::string();
}

/// \ingroup ioda_cxx_variable
/// \see netcdf.h, starting around line 62, for these values
///   netcdf uses "ints" and "shorts", but these are all defined as fixed-width types.
template <class T>
T netCDF4_default() {
  return 0;
}
template <>
inline std::string netCDF4_default<std::string>() {
  return std::string();
}
template <>
inline signed char netCDF4_default<signed char>() {
  return static_cast<signed char>(-127);
}
template <>
inline char netCDF4_default<char>() {
  return static_cast<char>(0);
}
template <>
inline int16_t netCDF4_default<int16_t>() {
  return static_cast<int16_t>(-32767);
}
template <>
inline int32_t netCDF4_default<int32_t>() {
  return -2147483647;
}
template <>
inline float netCDF4_default<float>() {
  return 9.9692099683868690e+36f;
}
template <>
inline double netCDF4_default<double>() {
  return 9.9692099683868690e+36;
}
template <>
inline unsigned char netCDF4_default<unsigned char>() {
  return static_cast<unsigned char>(255);
}
template <>
inline uint16_t netCDF4_default<uint16_t>() {
  return static_cast<unsigned short>(65535);
}
template <>
inline uint32_t netCDF4_default<uint32_t>() {
  return 4294967295U;
}
template <>
inline int64_t netCDF4_default<int64_t>() {
  return -9223372036854775806LL;
}
template <>
inline uint64_t netCDF4_default<uint64_t>() {
  return 18446744073709551614ULL;
}

/// \brief Applies the fill value policy. This sets default fill values when fill values are not
///   already provided.
/// \ingroup ioda_cxx_variable
template <class T>
void applyFillValuePolicy(FillValuePolicy pol, detail::FillValueData_t& fvd) {
  if (fvd.set_) return;  // If already set, then do nothing.
  if (pol == FillValuePolicy::HDF5)
    detail::assignFillValue(fvd, HDF5_default<T>());
  else if (pol == FillValuePolicy::NETCDF4)
    detail::assignFillValue(fvd, netCDF4_default<T>());
  else
    throw;  // jedi_throw.add("Reason", "Unsupported fill value policy.");
}
}  // namespace FillValuePolicies
}  // namespace ioda

/// @}

