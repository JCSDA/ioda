/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include "ioda/Types/Type.h"

#include <map>

#include "ioda/defs.h"
#include "ioda/Exception.h"

namespace ioda {
namespace detail {
/** \brief Safe char array copy.
  \returns the number of characters actually written.
  \param dest is the pointer to the destination. Always null terminated.
  \param destSz is the size of the destination buller, including the trailing null character.
  \param src is the pointer to the source. Characters from src are copied either until the
  first null character or until srcSz. Note that null termination comes later.
  \param srcSz is the max size of the source buffer.
  \deprecated This function is old and should not be used!
  **/
size_t COMPAT_strncpy_s(char* dest, size_t destSz, const char* src, size_t srcSz) {
  if (!dest || !src) throw Exception("Null pointer passed to function.", ioda_Here());
#ifdef JEDI_USING_SECURE_STRINGS
  strncpy_s(dest, destSz, src, srcSz);
  return strnlen_s(dest, destSz);
#else
  if (destSz == 0) throw;  // jedi_throw.add("Reason", "Invalid destination size.");
  // See https://devblogs.microsoft.com/oldnewthing/20170927-00/?p=97095
  // Unfortunately, we can't really detect pointer range overlaps in a system-independent
  // manner. This is why we want to use the secure string functions if at all possible.
  if (srcSz <= destSz) {
    strncpy(dest, src, srcSz);
    if (dest[srcSz - 1] != '\0') throw Exception("Non-terminated null copy.", ioda_Here());
  } else {
    strncpy(dest, src, destSz);
    // Additionally, throw on null-string truncation.

    // Not using strchr because of its memory-unsafe nature.
    for (size_t i = 0; i < destSz - 1; ++i) {
      if (i < srcSz) {
        if (dest[i] == '\0' && src[i] == '\0') break;  // First null hit. Success.
        if (dest[i] == '\0' && src[i] != '\0')
          throw Exception("Truncated array copy error.", ioda_Here());
      } else
        throw Exception("Null not reached by end of source!", ioda_Here());
    }
  }

  dest[destSz - 1] = 0;
  for (size_t i = 0; i < destSz; ++i) {
    if (dest[i] == '\0') return i;
  }
  throw Exception("Truncated array copy error.", ioda_Here());
#endif
}

// template<> Type_Base<>::~Type_Base() {}
// template<> Type_Base<>::Type_Base(std::shared_ptr<Type_Backend> b) : backend_(b) {}
// template<> std::shared_ptr<Type_Backend> Type_Base<>::getBackend() const { return backend_; }


template <>
size_t Type_Base<>::getSize() const {
  try {
    if (backend_ == nullptr)
      throw Exception("Missing backend or unimplemented backend function.", ioda_Here());
    return backend_->getSize();
  } catch (...) {
    std::throw_with_nested(Exception(
      "An exception occurred inside ioda while getting the size of a data type.", ioda_Here()));
  }
}

template <>
TypeClass Type_Base<>::getClass() const {
  try {
    if (backend_ == nullptr)
      throw Exception("Missing backend or unimplemented backend function.", ioda_Here());
    return backend_->getClass();
  } catch (...) {
    std::throw_with_nested(Exception(
      "An exception occurred inside ioda while getting the class of a data type.", ioda_Here()));
  }
}

template <>
void Type_Base<>::commitToBackend(Group &d, const std::string &name) const {
  try {
    if (backend_ == nullptr)
      throw Exception("Missing backend or unimplemented backend function.", ioda_Here());
    backend_->commitToBackend(d, name);
  } catch (...) {
    std::throw_with_nested(Exception(
      "An exception occurred inside ioda while committing a datatype to a backend.", ioda_Here()));
  }
}

template <>
bool Type_Base<>::isTypeSigned() const {
  try {
    if (backend_ == nullptr)
      throw Exception("Missing backend or unimplemented backend function.", ioda_Here());
    return backend_->isTypeSigned();
  } catch (...) {
    std::throw_with_nested(Exception(
      "An exception occurred inside ioda while checking if a "
      "numeric type is signed or unsigned.", ioda_Here()));
  }
}

template <>
bool Type_Base<>::isVariableLengthStringType() const {
  try {
    if (backend_ == nullptr)
      throw Exception("Missing backend or unimplemented backend function.", ioda_Here());
    return backend_->isVariableLengthStringType();
  } catch (...) {
    std::throw_with_nested(Exception(
      "An exception occurred inside ioda while checking if a "
      "string type is of variable length.", ioda_Here()));
  }
}

template <>
StringCSet Type_Base<>::getStringCSet() const {
  try {
    if (backend_ == nullptr)
      throw Exception("Missing backend or unimplemented backend function.", ioda_Here());
    return backend_->getStringCSet();
  } catch (...) {
    std::throw_with_nested(Exception(
      "An exception occurred inside ioda while determining the "
      "character set used in a string type.", ioda_Here()));
  }
}

template <>
Type Type_Base<>::getBaseType() const {
  try {
    if (backend_ == nullptr)
      throw Exception("Missing backend or unimplemented backend function.", ioda_Here());
    return backend_->getBaseType();
  } catch (...) {
    std::throw_with_nested(Exception(
      "An exception occurred inside ioda while determining the "
      "base type used in an array or enumeration type.", ioda_Here()));
  }
}

template <>
std::vector<Dimensions_t> Type_Base<>::getDimensions() const {
  try {
    if (backend_ == nullptr)
      throw Exception("Missing backend or unimplemented backend function.", ioda_Here());
    return backend_->getDimensions();
  } catch (...) {
    std::throw_with_nested(Exception(
      "An exception occurred inside ioda while determining the "
      "array dimensions of a type.", ioda_Here()));
  }
}

Type_Backend::Type_Backend() : Type_Base{nullptr, nullptr} {}
Type_Backend::~Type_Backend() = default;

StringCSet Type_Backend::getStringCSet() const {
  return StringCSet::UTF8;
}

}  // namespace detail

Type::Type() : Type_Base(nullptr, nullptr), as_type_index_(typeid(void)) {}
Type::Type(std::shared_ptr<detail::Type_Backend> b, std::type_index t)
    : Type_Base(b, b->provider_), as_type_index_(t) {}

Type::Type(BasicTypes typ, gsl::not_null<::ioda::detail::Type_Provider*> t)
    : Type_Base(nullptr, t.get()), as_type_index_(typeid(void)) {
  static const std::map<BasicTypes, std::type_index> workable_types
    = {{BasicTypes::float_, typeid(float)},    // NOLINT: cpplint doesn't understand this!
       {BasicTypes::double_, typeid(double)},  // NOLINT
       {BasicTypes::ldouble_, typeid(long double)},
       {BasicTypes::char_, typeid(char)},  // NOLINT
       {BasicTypes::short_, typeid(short)},
       {BasicTypes::ushort_, typeid(unsigned short)},
       {BasicTypes::int_, typeid(int)},  // NOLINT
       {BasicTypes::uint_, typeid(unsigned)},
       {BasicTypes::lint_, typeid(long)},
       {BasicTypes::ulint_, typeid(unsigned long)},
       {BasicTypes::llint_, typeid(long long)},
       {BasicTypes::ullint_, typeid(unsigned long long)},
       {BasicTypes::int32_, typeid(int32_t)},
       {BasicTypes::uint32_, typeid(uint32_t)},
       {BasicTypes::int16_, typeid(int16_t)},
       {BasicTypes::uint16_, typeid(uint16_t)},
       {BasicTypes::int64_, typeid(int64_t)},
       {BasicTypes::uint64_, typeid(uint64_t)},
       {BasicTypes::bool_, typeid(bool)},  // NOLINT
       {BasicTypes::str_, typeid(std::string)}};
  as_type_index_ = workable_types.at(typ);

  if (typ == BasicTypes::undefined_) throw Exception("Bad input", ioda_Here());
  if (typ == BasicTypes::str_)
    *this = t->makeStringType(typeid(std::string), Types::constants::_Variable_Length);
  else
    *this = t->makeFundamentalType(workable_types.at(typ));
}

Type::~Type() = default;


}  // namespace ioda
