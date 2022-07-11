/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \addtogroup ioda_internals_engines_obsstore
 *
 * @{
 * \file VarAttrStore.cpp
 * \brief Functions for ObsStore variable and attribute data storage
 */

#include "./VarAttrStore.hpp"

#include <exception>

#include "./Type.hpp"
#include "ioda/Exception.h"

namespace ioda {
namespace ObsStore {
//------------------------------------------------------------------------------
VarAttrStore_Base *createVarAttrStore(const std::shared_ptr<Type> & dtype) {
  VarAttrStore_Base *newStore = nullptr;

  // Get the fundamental (base) type marker. In the case of an arrayed type,
  // the fundamental type mark is in the base type data member of dtype.
  ObsTypes topLevelType = dtype->getType();
  ObsTypes baseType;
  if (topLevelType == ObsTypes::ARRAY) {
    baseType = dtype->getBaseType()->getType();
  } else {
    baseType = topLevelType;
  }

  // Record the number of elements in the type. For fundamental types this would be 1,
  // and for arrayed types this would be determined according to the dimension sizes.
  std::size_t numElements = dtype->getNumElements();

  // Use the baseType value to determine which templated version of the data store
  // to instantiate.
  if (baseType == ObsTypes::FLOAT) {
    newStore = new VarAttrStore<float>(numElements);
  } else if (baseType == ObsTypes::DOUBLE) {
    newStore = new VarAttrStore<double>(numElements);
  } else if (baseType == ObsTypes::LDOUBLE) {
    newStore = new VarAttrStore<long double>(numElements);
  } else if (baseType == ObsTypes::SCHAR) {
    newStore = new VarAttrStore<signed char>(numElements);
  } else if (baseType == ObsTypes::SHORT) {
    newStore = new VarAttrStore<short>(numElements);
  } else if (baseType == ObsTypes::INT) {
    newStore = new VarAttrStore<int>(numElements);
  } else if (baseType == ObsTypes::LONG) {
    newStore = new VarAttrStore<long>(numElements);
  } else if (baseType == ObsTypes::LLONG) {
    newStore = new VarAttrStore<long long>(numElements);
  } else if (baseType == ObsTypes::UCHAR) {
    newStore = new VarAttrStore<unsigned char>(numElements);
  } else if (baseType == ObsTypes::USHORT) {
    newStore = new VarAttrStore<unsigned short>(numElements);
  } else if (baseType == ObsTypes::UINT) {
    newStore = new VarAttrStore<unsigned int>(numElements);
  } else if (baseType == ObsTypes::ULONG) {
    newStore = new VarAttrStore<unsigned long>(numElements);
  } else if (baseType == ObsTypes::ULLONG) {
    newStore = new VarAttrStore<unsigned long long>(numElements);
  } else if (baseType == ObsTypes::CHAR) {
    newStore = new VarAttrStore<char>(numElements);
  } else if (baseType == ObsTypes::WCHAR) {
    newStore = new VarAttrStore<wchar_t>(numElements);
  } else if (baseType == ObsTypes::CHAR16) {
    newStore = new VarAttrStore<char16_t>(numElements);
  } else if (baseType == ObsTypes::CHAR32) {
    newStore = new VarAttrStore<char32_t>(numElements);
  } else if (baseType == ObsTypes::STRING) {
    newStore = new VarAttrStore<std::string>(numElements);
  } else
    throw Exception("Unrecognized data type encountered during "
      "Attribute object construnction", ioda_Here());

  return newStore;
}

}  // namespace ObsStore
}  // namespace ioda

/// @}
