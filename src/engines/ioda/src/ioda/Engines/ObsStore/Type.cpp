/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \addtogroup ioda_internals_engines_obsstore
 *
 * @{
 * \file Type.cpp
 * \brief Functions for ObsStore Type and Has_Types
 */
#include "./Type.hpp"

#include <functional>
#include <map>
#include <numeric>
#include <stdexcept>

#include "ioda/defs.h"
#include "ioda/Exception.h"

namespace ioda {
namespace ObsStore {

//---------------------------------------------------------------------------------------
Type::Type() : dims_({}), type_(ObsTypes::NOTYPE), class_(ObsTypeClasses::NOCLASS),
               base_type_(nullptr), num_elements_(0), size_(0), is_signed_(false) {
}

Type::Type(const ObsTypes dataType, const ObsTypeClasses typeClass,
           const std::size_t typeSize, const bool isTypeSigned)
               : dims_({}), type_(dataType), class_(typeClass), base_type_(nullptr),
                 num_elements_(1), size_(typeSize), is_signed_(isTypeSigned) {
}

Type::Type(const std::vector<std::size_t> & dims, const ObsTypes dataType,
           const ObsTypeClasses typeClass, std::shared_ptr<Type> baseType)
               : dims_(dims), type_(dataType), class_(typeClass), base_type_(baseType) {
  num_elements_ =
    std::accumulate(dims.begin(), dims.end(), (std::size_t)1, std::multiplies<std::size_t>());
  size_ = baseType->getSize() * num_elements_;
  is_signed_ = baseType->isTypeSigned();
}

//---------------------------------------------------------------------------------------
bool Type::operator==(const Type & rhs) const {
  // objects are equal if data members all match
  bool match = ((type_ == rhs.type_) && (size_ == rhs.size_) &&
                (class_ == rhs.class_) && (num_elements_ == rhs.num_elements_) &&
                (is_signed_ == rhs.is_signed_) && (dims_.size() == rhs.dims_.size()));
  if (match) {
    // Check that all dimension sizes match, already have determined that dim ranks match
    for (std::size_t i = 0; i < dims_.size(); ++i) {
      if (dims_[i] != rhs.dims_[i]) {
        match = false;
        break;
      }
    }
  }

  if (match) {
    bool lhs_HasBaseType = (base_type_ != nullptr);
    bool rhs_HasBaseType = (rhs.base_type_ != nullptr);
    if ((lhs_HasBaseType) && (rhs_HasBaseType)) {
      // Both types have a base type, check to see if these match.
      match = (*base_type_ == *rhs.base_type_);
    } else if (((!lhs_HasBaseType) && (rhs_HasBaseType)) ||
               ((lhs_HasBaseType) && (!rhs_HasBaseType))) {
      // One has a base type the other doesn't. This case shouldn't happen often
      match = false;
    }
    // If we didn't hit the cases in the above if structure, then both lhs and rhs
    // do not have a base type (both are nullptr). There's no need to check anything,
    // and match can remain set to true.
  }

  return match;
}

bool Type::operator!=(const Type & rhs) const {
  return !(*this == rhs);
}

detail::Engines::HH::HH_Type Type::getHDF5Type() const {
  // We can look at ObsTypes using getType() or type_ and return the matching HDF5 type.
  using namespace detail::Engines::HH;
  static auto HH_default_string_type
    = std::dynamic_pointer_cast<HH_Type>(HH_Type_Provider::instance()->makeStringType(
        typeid(char),Types::constants::_Variable_Length, StringCSet::UTF8).getBackend());

  static const std::map<ObsTypes, HH_Type> mappings{
    {ObsTypes::FLOAT, HH_Type{HH_Type_Provider::getFundamentalHHType(typeid(float))}},
    {ObsTypes::DOUBLE, HH_Type{HH_Type_Provider::getFundamentalHHType(typeid(double))}},
    {ObsTypes::LDOUBLE, HH_Type{HH_Type_Provider::getFundamentalHHType(typeid(long double))}},
    {ObsTypes::SCHAR, HH_Type{HH_Type_Provider::getFundamentalHHType(typeid(signed char))}},
    {ObsTypes::SHORT, HH_Type{HH_Type_Provider::getFundamentalHHType(typeid(short))}},
    {ObsTypes::INT, HH_Type{HH_Type_Provider::getFundamentalHHType(typeid(int))}},
    {ObsTypes::LONG, HH_Type{HH_Type_Provider::getFundamentalHHType(typeid(long))}},
    {ObsTypes::LLONG, HH_Type{HH_Type_Provider::getFundamentalHHType(typeid(long long))}},
    {ObsTypes::UCHAR, HH_Type{HH_Type_Provider::getFundamentalHHType(typeid(unsigned char))}},
    {ObsTypes::UINT, HH_Type{HH_Type_Provider::getFundamentalHHType(typeid(unsigned int))}},
    {ObsTypes::USHORT, HH_Type{HH_Type_Provider::getFundamentalHHType(typeid(unsigned short))}},
    {ObsTypes::ULONG, HH_Type{HH_Type_Provider::getFundamentalHHType(typeid(unsigned long))}},
    {ObsTypes::ULLONG, HH_Type{HH_Type_Provider::getFundamentalHHType(typeid(unsigned long long))}},
    {ObsTypes::CHAR, HH_Type{HH_Type_Provider::getFundamentalHHType(typeid(char))}},
    // No support for wchars, char16, char32. Unimplemented in the hdf5 backend's
    // current iteration of the type system, and unused in ioda.
    //{ObsTypes::WCHAR, HH_Type{HH_Type_Provider::getFundamentalHHType(typeid(wchar_t))}},
    //{ObsTypes::CHAR16, HH_Type{HH_Type_Provider::getFundamentalHHType(typeid(char16_t))}},
    //{ObsTypes::CHAR32, HH_Type{HH_Type_Provider::getFundamentalHHType(typeid(char32_t))}}
    // No support for array types
    //{ObsTypes::ARRAY, ...}
    // We do support a single basic string type. ObsStore doesn't track the string's
    // character set, length, or padding information, so we assume some defaults
    // for the type conversion.
    {ObsTypes::STRING, *HH_default_string_type.get()}
  };
  if (!mappings.count(getType())) throw Exception("Unimplemented mapping", ioda_Here());
  return mappings.at(getType());
}

//---------------------------------------------------------------------------------------

}  // namespace ObsStore
}  // namespace ioda

/// @}
