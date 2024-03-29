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
 * \file Fill.h
 * \brief Fill value getters and setters
 */

#include <cstdint>  // uint64_t
#include <memory>   // weak_ptr
#include <string>

#include "ioda/defs.h"

namespace ioda {
namespace detail {
class Group_Base;

/**
 * \brief Container used to store and manipulate fill values.
 * \ingroup ioda_cxx_variable
 *
 * When reading a fill value, first always check that the fill value is set (set_ == true).
 * Then, check the type of fill value (string, or a fundamental data type),
 * and then only read the correct field.
 *
 * When writing a fill value, use the assignFillValue convenience function.
 **/
struct IODA_DL FillValueData_t {
  union FillValueUnion_t {
    uint64_t ui64;  // Kept for compatability with HH
    const char* cp;
    long long ll;
    unsigned long long ull;
    long double ld;
    long l;
    unsigned long ul;
    double d;
    float f;
    int i;
    unsigned int ui;
    short s;
    unsigned short us;
    char c;
    unsigned char uc;
  } fillValue_ = {0};
  std::string stringFillValue_;
  bool set_      = false;
  bool isString_ = false;
  FillValueUnion_t finalize() const;

  // Type fillValue_type; // Possible TODO - store this and make sure it matches?
};

/// \ingroup ioda_cxx_variable
template <class T>
T getFillValue(const FillValueData_t& data) {
  if (sizeof(T) > sizeof(uint64_t)) {
    const long double* ldp = &(data.fillValue_.ld);
    return *(T*)(ldp);  // NOLINT: compilers may complain if I try to use a reinterpret_cast.
  } else {
    const uint64_t* up = &(data.fillValue_.ui64);
    return *(T*)(up);  // NOLINT: see above comment
  }
}

/// \ingroup ioda_cxx_variable
template <>
inline std::string getFillValue(const FillValueData_t& data) {
  return data.stringFillValue_;
}

/// \ingroup ioda_cxx_variable
template <class T>
void assignFillValue(FillValueData_t& data, T val) {
  if (sizeof(T) > sizeof(uint64_t))
    memcpy(&(data.fillValue_.ld), &val, sizeof(val));
  else
    memcpy(&(data.fillValue_.ui64), &val, sizeof(val));
  data.stringFillValue_ = "";
  data.set_             = true;
  data.isString_        = false;
}
/// \ingroup ioda_cxx_variable
template <>
inline void assignFillValue<std::string>(FillValueData_t& data, std::string val) {
  data.stringFillValue_ = val;
  data.set_             = true;
  data.isString_        = true;
}
}  // namespace detail
}  // namespace ioda

/// @}
