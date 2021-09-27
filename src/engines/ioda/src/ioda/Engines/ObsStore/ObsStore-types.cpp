/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \addtogroup ioda_internals_engines_obsstore
 *
 * @{
 * \file ObsStore-types.cpp
 * \brief Functions for translating ioda::Types to ObsStore Types
 */

#include <map>

#include "./ObsStore-types.h"
#include "./Types.hpp"
#include "ioda/defs.h"
#include "ioda/Exception.h"
#include "ioda/Types/Type.h"

namespace ioda {
namespace Engines {
namespace ObsStore {
//*********************************************************************
// ObsStore_Type functions
//*********************************************************************
ObsStore_Type::ObsStore_Type(ObsTypeInfo& h) : dtype_(h.first), dtype_size_(h.second) {}
ObsStore_Type::~ObsStore_Type() = default;

ioda::ObsStore::ObsTypes ObsStore_Type::dtype() const { return dtype_; }

std::size_t ObsStore_Type::dtype_size() const { return dtype_size_; }
std::size_t ObsStore_Type::getSize() const { return dtype_size(); }

//*********************************************************************
// ObsStore_Type_Provider functions
//*********************************************************************
ObsStore_Type_Provider* ObsStore_Type_Provider::instance() {
  static ObsStore_Type_Provider provider;
  return &provider;
}
ObsStore_Type_Provider::~ObsStore_Type_Provider() = default;

ObsTypeInfo ObsStore_Type_Provider::getFundamentalObsStoreType(std::type_index type) {
  static const std::map<std::type_index, ObsTypeInfo> fundamental_types
    = {{typeid(bool), {ioda::ObsStore::ObsTypes::BOOL, sizeof(bool)}},

       {typeid(float), {ioda::ObsStore::ObsTypes::FLOAT, sizeof(float)}},
       {typeid(double), {ioda::ObsStore::ObsTypes::DOUBLE, sizeof(double)}},
       {typeid(long double), {ioda::ObsStore::ObsTypes::LDOUBLE, sizeof(long double)}},

       {typeid(signed char), {ioda::ObsStore::ObsTypes::SCHAR, sizeof(signed char)}},
       {typeid(short), {ioda::ObsStore::ObsTypes::SHORT, sizeof(short)}},
       {typeid(int), {ioda::ObsStore::ObsTypes::INT, sizeof(int)}},
       {typeid(long), {ioda::ObsStore::ObsTypes::LONG, sizeof(long)}},             // NOLINT
       {typeid(long long), {ioda::ObsStore::ObsTypes::LLONG, sizeof(long long)}},  // NOLINT

       {typeid(unsigned char), {ioda::ObsStore::ObsTypes::UCHAR, sizeof(unsigned char)}},
       {typeid(unsigned short), {ioda::ObsStore::ObsTypes::USHORT, sizeof(unsigned short)}},
       {typeid(unsigned int), {ioda::ObsStore::ObsTypes::UINT, sizeof(unsigned int)}},     // NOLINT
       {typeid(unsigned long), {ioda::ObsStore::ObsTypes::ULONG, sizeof(unsigned long)}},  // NOLINT
       {typeid(unsigned long long),
        {ioda::ObsStore::ObsTypes::ULLONG, sizeof(unsigned long long)}},  // NOLINT

       {typeid(char), {ioda::ObsStore::ObsTypes::CHAR, sizeof(char)}},
       {typeid(wchar_t), {ioda::ObsStore::ObsTypes::WCHAR, sizeof(wchar_t)}},
       {typeid(char16_t), {ioda::ObsStore::ObsTypes::CHAR16, sizeof(char16_t)}},
       {typeid(char32_t), {ioda::ObsStore::ObsTypes::CHAR32, sizeof(char32_t)}}};

  if (!fundamental_types.count(type)) {
    throw Exception("ObsStore does not recognize this type.", ioda_Here());
  }
  return fundamental_types.at(type);
}

Type ObsStore_Type_Provider::makeFundamentalType(std::type_index type) const {
  ObsTypeInfo tinfo = getFundamentalObsStoreType(type);
  return Type{std::make_shared<ObsStore_Type>(tinfo), type};
}

// Leave makeArrayType undeclared for now. Add in later if needed.
// By leaving undeclared, instead of empty, the makeArrayType() method
// in ioda::TypeProvider will issue an error.
//
// Type ObsStore_Type_Provider::makeArrayType(
//                         std::initializer_list<Dimensions_t> dimensions,
//                         std::type_index typeOuter,
//                         std::type_index typeInner) const {
// }

Type ObsStore_Type_Provider::makeStringType(std::type_index typeOuter,
                                            size_t /*string_length*/,
                                            StringCSet /*cset*/) const {
  ObsTypeInfo tinfo(std::make_pair(ioda::ObsStore::ObsTypes::STRING, sizeof(char*)));
  return Type{std::make_shared<ObsStore_Type>(tinfo), typeOuter};
}

// The ObsStore backend will take care of freeing memory during a read action,
// so provide a routine to notify the frontend about this.
ioda::detail::PointerOwner ObsStore_Type_Provider::getReturnedPointerOwner() const {
  return ioda::detail::PointerOwner::Engine;
}
}  // namespace ObsStore
}  // namespace Engines
}  // namespace ioda

/// @}
