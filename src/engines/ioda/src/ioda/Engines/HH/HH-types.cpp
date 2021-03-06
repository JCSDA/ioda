/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \addtogroup ioda_internals_engines_hh
 *
 * @{
 * \file HH-types.cpp
 * \brief HDF5 engine implementation of Type.
 */
#include "./HH/HH-types.h"

#include <map>

#include "./HH/Handles.h"
#include "ioda/Types/Type.h"
#include "ioda/Exception.h"
#include "ioda/defs.h"

namespace ioda {
namespace detail {
namespace Engines {
namespace HH {
HH_Type::~HH_Type() = default;
HH_Type::HH_Type(HH_hid_t h) : handle(h) {}

size_t HH_Type::getSize() const {
  size_t res = H5Tget_size(handle.get());
  if (res == 0) throw Exception("H5Tget_size failed.", ioda_Here());
  return res;
}

HH_Type_Provider::~HH_Type_Provider() = default;
HH_Type_Provider* HH_Type_Provider::instance() {
  static HH_Type_Provider provider;
  return &provider;
}

HH_hid_t HH_Type_Provider::getFundamentalHHType(std::type_index type) {
  static const std::map<std::type_index, HH_hid_t> fundamental_types
    = {{typeid(bool), {H5T_NATIVE_HBOOL}},
       {typeid(short int), {H5T_NATIVE_SHORT}},                // NOLINT
       {typeid(unsigned short int), {H5T_NATIVE_USHORT}},      // NOLINT
       {typeid(int), {H5T_NATIVE_INT}},                        // NOLINT
       {typeid(unsigned int), {H5T_NATIVE_UINT}},              // NOLINT
       {typeid(long int), {H5T_NATIVE_LONG}},                  // NOLINT
       {typeid(unsigned long int), {H5T_NATIVE_ULONG}},        // NOLINT
       {typeid(long long int), {H5T_NATIVE_LLONG}},            // NOLINT
       {typeid(unsigned long long int), {H5T_NATIVE_ULLONG}},  // NOLINT
       {typeid(signed char), {H5T_NATIVE_SCHAR}},              // NOLINT
       //{typeid(unsigned char), {H5T_NATIVE_U}},
       {typeid(char), {H5T_NATIVE_CHAR}},  // NOLINT
       //{typeid(wchar_t), {H5T_NATIVE_HBOOL}},
       //{typeid(char16_t), {H5T_NATIVE_HBOOL}},
       //{typeid(char32_t), {H5T_NATIVE_HBOOL}},
       //{typeid(char8_t), {H5T_NATIVE_HBOOL}},
       {typeid(float), {H5T_NATIVE_FLOAT}},
       {typeid(double), {H5T_NATIVE_DOUBLE}},
       {typeid(long double), {H5T_NATIVE_LDOUBLE}}};

  if (!fundamental_types.count(type)) throw Exception("HDF5 does not implement this type as a fundamental type.", ioda_Here());
  return fundamental_types.at(type);
}

Type HH_Type_Provider::makeFundamentalType(std::type_index type) const {
  HH_hid_t t = getFundamentalHHType(type);
  return Type{std::make_shared<HH_Type>(t), type};
}

/// \todo typeOuter, typeInner
Type HH_Type_Provider::makeArrayType(std::initializer_list<Dimensions_t> dimensions,
                                     std::type_index typeOuter, std::type_index typeInner) const {
  /// \todo Allow for arrays of string types!
  HH_hid_t fundamental_type = getFundamentalHHType(typeInner);

  std::vector<hsize_t> hdims;
  for (const auto& d : dimensions) hdims.push_back(gsl::narrow<hsize_t>(d));
  hid_t t = H5Tarray_create2(fundamental_type(), gsl::narrow<unsigned int>(dimensions.size()),
                             hdims.data());
  if (t < 0) throw Exception("Failed call to H5Tarray_create2.", ioda_Here());
  auto hnd = HH_hid_t(t, Handles::Closers::CloseHDF5Datatype::CloseP);

  return Type{std::make_shared<HH_Type>(hnd), typeOuter};
}

Type HH_Type_Provider::makeStringType(size_t string_length, std::type_index typeOuter) const {
  if (string_length == Types::constants::_Variable_Length) string_length = H5T_VARIABLE;
  hid_t t = H5Tcreate(H5T_STRING, string_length);
  if (t < 0) throw Exception("Failed call to H5Tcreate.", ioda_Here());
  if (H5Tset_cset(t, H5T_CSET_UTF8) < 0)
    throw Exception("Failed call to H5Tset_cset.", ioda_Here());
  auto hnd = HH_hid_t(t, Handles::Closers::CloseHDF5Datatype::CloseP);

  return Type{std::make_shared<HH_Type>(hnd), typeOuter};
}

}  // namespace HH
}  // namespace Engines
}  // namespace detail
}  // namespace ioda

/// @}
