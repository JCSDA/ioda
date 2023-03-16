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

#include <array>
#include <map>

#include "./HH/Handles.h"
#include "./HH/HH-groups.h"
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

TypeClass HH_Type::getClass() const {
  H5T_class_t cls = H5Tget_class(handle.get());
  // Enums are in H5Tpublic.h, and are ordered. We can use
  // a std::array to store the lookup result. 
  const std::array<TypeClass, H5T_NCLASSES> typemap {
    TypeClass::Integer,
    TypeClass::Float,
    TypeClass::Unknown,    // H5T_TIME is deprecated.
    TypeClass::String,
    TypeClass::Bitfield,
    TypeClass::Opaque,
    TypeClass::Compound,
    TypeClass::Reference,
    TypeClass::Enum,
    TypeClass::VlenArray,
    TypeClass::FixedArray
  };
  if (cls < 0 || cls >= H5T_NCLASSES)
    throw Exception("Cannot get class. Unknown HDF5 type.", ioda_Here());
  return typemap.at(cls);
}

void HH_Type::commitToBackend(Group &g, const std::string &name) const {
  // Check that the Group is an HDF5-backed group.
  try {
    auto groupBackend = std::dynamic_pointer_cast<HH_Group>(g.getBackend());
    herr_t res = H5Tcommit2(groupBackend->get()(),
                            name.c_str(),
                            handle.get(),
                            H5P_DEFAULT,
                            H5P_DEFAULT,
                            H5P_DEFAULT);
    if (res < 0) throw Exception("H5Tcommit2 failed.", ioda_Here());
  } catch (std::bad_cast) {
    throw Exception("Group passed to function is not an HDF5 group.", ioda_Here());
  }
}

bool HH_Type::isTypeSigned() const {
  if (getClass() != TypeClass::Integer) throw Exception("Non-integer data type.", ioda_Here());
  return H5Tget_sign(handle.get()) == H5T_SGN_2;
}

bool HH_Type::isVariableLengthStringType() const {
  htri_t res = H5Tis_variable_str(handle.get());
  if (res < 0) throw Exception("HDF5 type is not a string type, or another error has occurred.", ioda_Here());
  return (res > 0);
}

StringCSet HH_Type::getStringCSet() const {
  H5T_cset_t res = H5Tget_cset(handle.get());
  const std::map<H5T_cset_t, StringCSet> csmap {
    {H5T_CSET_ASCII, StringCSet::ASCII},
    {H5T_CSET_UTF8, StringCSet::UTF8}
  };
  if (!csmap.count(res)) throw Exception("Error in H5Tget_cset. Likely bad HDF5 type.", ioda_Here());
  return csmap.at(res);
}

Type HH_Type::getBaseType() const {
  hid_t h = H5Tget_super(handle.get());
  if (h < 0)
    throw Exception("Error in H5Tget_super. Likely not an enumeration or array type.", ioda_Here());
  auto hnd = HH_hid_t(h, Handles::Closers::CloseHDF5Datatype::CloseP);

  return Type{std::make_shared<HH_Type>(hnd), typeid(void)};
}

std::vector<Dimensions_t> HH_Type::getDimensions() const {
  int ndims = H5Tget_array_ndims(handle.get());
  if (ndims < 0) throw Exception("Error in H5Tget_array_ndims. Likely bad HDF5 type.", ioda_Here());
  std::vector<hsize_t> hdims(static_cast<size_t>(ndims));
  if (H5Tget_array_dims2(handle.get(), hdims.data()) < 0)
    throw Exception("Error in H5Tget_array_dims2.", ioda_Here());

  std::vector<Dimensions_t> res(hdims.size());
  for (size_t i=0; i < hdims.size(); ++i)
    res[i] = gsl::narrow<Dimensions_t>(hdims[i]);
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
       {typeid(unsigned char), {H5T_NATIVE_UCHAR}},            // NOLINT
       {typeid(char), {H5T_NATIVE_CHAR}},                      // NOLINT
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

Type HH_Type_Provider::makeStringType(
    std::type_index typeOuter,
    size_t string_length, 
    StringCSet cset) const
{
  if (string_length == Types::constants::_Variable_Length) string_length = H5T_VARIABLE;
  hid_t t = H5Tcreate(H5T_STRING, string_length);
  if (t < 0) throw Exception("Failed call to H5Tcreate.", ioda_Here());
  if (cset == StringCSet::UTF8) {
    if (H5Tset_cset(t, H5T_CSET_UTF8) < 0)
      throw Exception("Failed call to H5Tset_cset.", ioda_Here());
  }
  auto hnd = HH_hid_t(t, Handles::Closers::CloseHDF5Datatype::CloseP);

  return Type{std::make_shared<HH_Type>(hnd), typeOuter};
}

}  // namespace HH
}  // namespace Engines
}  // namespace detail
}  // namespace ioda

/// @}
