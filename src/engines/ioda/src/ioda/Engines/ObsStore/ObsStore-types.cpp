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

#include <functional>
#include <map>
#include <numeric>

#include "./ObsStore-types.h"
#include "./Type.hpp"
#include "ioda/defs.h"
#include "ioda/Exception.h"
#include "ioda/Types/Type.h"

namespace ioda {
namespace Engines {
namespace ObsStore {
//*********************************************************************
// ObsStore_Type functions
//*********************************************************************
ObsStore_Type::ObsStore_Type(std::shared_ptr<ioda::ObsStore::Type> type) : type_(type) {
}

TypeClass ObsStore_Type::getClass() const {
  ioda::ObsStore::ObsTypeClasses obsStoreClass = type_->getClass();
  TypeClass res;

  switch (obsStoreClass) {
    case ioda::ObsStore::ObsTypeClasses::INTEGER :
      res = TypeClass::Integer;
      break;
    case ioda::ObsStore::ObsTypeClasses::NOCLASS :
      res = TypeClass::Unknown;
      break;
    case ioda::ObsStore::ObsTypeClasses::FLOAT :
      res = TypeClass::Float;
      break;
    case ioda::ObsStore::ObsTypeClasses::STRING :
      res = TypeClass::String;
      break;
    case ioda::ObsStore::ObsTypeClasses::BITFIELD :
      res = TypeClass::Bitfield;
      break;
    case ioda::ObsStore::ObsTypeClasses::OPAQUE :
      res = TypeClass::Opaque;
      break;
    case ioda::ObsStore::ObsTypeClasses::COMPOUND :
      res = TypeClass::Compound;
      break;
    case ioda::ObsStore::ObsTypeClasses::REFERENCE :
      res = TypeClass::Reference;
      break;
    case ioda::ObsStore::ObsTypeClasses::ENUM :
      res = TypeClass::Enum;
      break;
    case ioda::ObsStore::ObsTypeClasses::VLEN_ARRAY :
      res = TypeClass::VlenArray;
      break;
    case ioda::ObsStore::ObsTypeClasses::FIXED_ARRAY :
      res = TypeClass::FixedArray;
      break;
    default :
      throw Exception("Cannot get class. Unknown ObsStore type.", ioda_Here());
  }
  return res;
}

Type ObsStore_Type::getBaseType() const {
  std::shared_ptr<ioda::ObsStore::Type> baseType = type_->getBaseType();
  if (baseType == nullptr) {
    throw Exception("ObsStore base type does not exist.", ioda_Here());
  }
  return Type{std::make_shared<ObsStore_Type>(baseType), typeid(void)};
}

std::vector<Dimensions_t> ObsStore_Type::getDimensions() const {
  std::vector<Dimensions_t> dimensions;
  std::vector<std::size_t> dims = type_->getDims();
  for (auto & i : dims) {
    dimensions.push_back(static_cast<Dimensions_t>(i));
  }
  return dimensions;
}

//*********************************************************************
// ObsStore_Type_Provider functions
//*********************************************************************
ObsStore_Type_Provider* ObsStore_Type_Provider::instance() {
  static ObsStore_Type_Provider provider;
  return &provider;
}
ObsStore_Type_Provider::~ObsStore_Type_Provider() = default;

ObsStore_Type_Provider::ObsTypeInfo
ObsStore_Type_Provider::getFundamentalObsStoreType(std::type_index type) {
  // The fourth entry in the ObsTypeInfo initializer lists is true if the type
  // is signed or false if the type is unsigned (or the quality of being signed
  // doesn't apply such as the case of a string).
  static const std::map<std::type_index, ObsTypeInfo> fundamental_types
    = {{typeid(float), {ioda::ObsStore::ObsTypes::FLOAT,
                        ioda::ObsStore::ObsTypeClasses::FLOAT,
                        sizeof(float), false}},
       {typeid(double), {ioda::ObsStore::ObsTypes::DOUBLE,
                         ioda::ObsStore::ObsTypeClasses::FLOAT,
                         sizeof(double), false}},
       {typeid(long double), {ioda::ObsStore::ObsTypes::LDOUBLE,
                         ioda::ObsStore::ObsTypeClasses::FLOAT,
                         sizeof(long double), false}},

       {typeid(signed char), {ioda::ObsStore::ObsTypes::SCHAR,
                              ioda::ObsStore::ObsTypeClasses::NOCLASS,
                              sizeof(signed char), true}},
       {typeid(short), {ioda::ObsStore::ObsTypes::SHORT,
                        ioda::ObsStore::ObsTypeClasses::INTEGER,
                        sizeof(short), true}},
       {typeid(int), {ioda::ObsStore::ObsTypes::INT,
                      ioda::ObsStore::ObsTypeClasses::INTEGER,
                      sizeof(int), true}},
       {typeid(long), {ioda::ObsStore::ObsTypes::LONG,
                       ioda::ObsStore::ObsTypeClasses::INTEGER,
                       sizeof(long), true}},             // NOLINT
       {typeid(long long), {ioda::ObsStore::ObsTypes::LLONG,
                            ioda::ObsStore::ObsTypeClasses::INTEGER,
                            sizeof(long long), true}},  // NOLINT

       {typeid(unsigned char), {ioda::ObsStore::ObsTypes::UCHAR,
                                ioda::ObsStore::ObsTypeClasses::NOCLASS,
                                sizeof(unsigned char), false}},
       {typeid(unsigned short), {ioda::ObsStore::ObsTypes::USHORT,
                                 ioda::ObsStore::ObsTypeClasses::INTEGER,
                                 sizeof(unsigned short), false}},
       {typeid(unsigned int), {ioda::ObsStore::ObsTypes::UINT,
                               ioda::ObsStore::ObsTypeClasses::INTEGER,
                               sizeof(unsigned int), false}},     // NOLINT
       {typeid(unsigned long), {ioda::ObsStore::ObsTypes::ULONG,
                                ioda::ObsStore::ObsTypeClasses::INTEGER,
                                sizeof(unsigned long), false}},  // NOLINT
       {typeid(unsigned long long), {ioda::ObsStore::ObsTypes::ULLONG,
                                     ioda::ObsStore::ObsTypeClasses::INTEGER,
                                     sizeof(unsigned long long), false}},  // NOLINT

       {typeid(char), {ioda::ObsStore::ObsTypes::CHAR,
                       ioda::ObsStore::ObsTypeClasses::NOCLASS,
                       sizeof(char), false}},
       {typeid(wchar_t), {ioda::ObsStore::ObsTypes::WCHAR,
                          ioda::ObsStore::ObsTypeClasses::NOCLASS,
                          sizeof(wchar_t), false}},
       {typeid(char16_t), {ioda::ObsStore::ObsTypes::CHAR16,
                           ioda::ObsStore::ObsTypeClasses::NOCLASS,
                           sizeof(char16_t), false}},
       {typeid(char32_t), {ioda::ObsStore::ObsTypes::CHAR32,
                           ioda::ObsStore::ObsTypeClasses::NOCLASS,
                           sizeof(char32_t), false}}};

  if (!fundamental_types.count(type)) {
    throw Exception("ObsStore does not recognize this type.", ioda_Here());
  }
  return fundamental_types.at(type);
}

Type ObsStore_Type_Provider::makeFundamentalType(std::type_index type) const {
  ObsTypeInfo tinfo = getFundamentalObsStoreType(type);
  std::shared_ptr<ioda::ObsStore::Type> backend_type =
      std::make_shared<ioda::ObsStore::Type>(tinfo.type, tinfo.type_class,
                                             tinfo.size, tinfo.is_signed);
  return Type{std::make_shared<ObsStore_Type>(backend_type), type};
}

Type ObsStore_Type_Provider::makeArrayType(std::initializer_list<Dimensions_t> dimensions,
                              std::type_index typeOuter, std::type_index typeInner) const {
  // Create the inner type (aka, base type)
  ObsTypeInfo tinfo = getFundamentalObsStoreType(typeInner);
  std::shared_ptr<ioda::ObsStore::Type> backend_base_type =
      std::make_shared<ioda::ObsStore::Type>(tinfo.type, tinfo.type_class,
                                             tinfo.size, tinfo.is_signed);

  // Create the outer type
  std::vector<std::size_t> dims;
  for (auto & i : dimensions) {
    dims.push_back(static_cast<std::size_t>(i));
  }
  std::shared_ptr<ioda::ObsStore::Type> backend_type =
      std::make_shared<ioda::ObsStore::Type>(dims, ioda::ObsStore::ObsTypes::ARRAY,
          ioda::ObsStore::ObsTypeClasses::FIXED_ARRAY, backend_base_type);
  return Type{std::make_shared<ObsStore_Type>(backend_type), typeOuter};
}

Type ObsStore_Type_Provider::makeStringType(std::type_index typeOuter,
                                            size_t /*string_length*/,
                                            StringCSet /*cset*/) const {
  std::shared_ptr<ioda::ObsStore::Type> backend_type =
      std::make_shared<ioda::ObsStore::Type>(ioda::ObsStore::ObsTypes::STRING,
          ioda::ObsStore::ObsTypeClasses::STRING, sizeof(char*), false);
  return Type{std::make_shared<ObsStore_Type>(backend_type), typeOuter};
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
