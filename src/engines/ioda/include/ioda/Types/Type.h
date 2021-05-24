#pragma once
/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \defgroup ioda_cxx_types Type System
 * \brief The data type system
 * \ingroup ioda_cxx_api
 *
 * @{
 * \file Type.h
 * \brief Interfaces for ioda::Type and related classes. Implements the type system.
 */
#include <array>
#include <cstring>
#include <functional>
#include <gsl/gsl-lite.hpp>
#include <memory>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <vector>

#include "ioda/Types/Type_Provider.h"
#include "ioda/Exception.h"
#include "ioda/defs.h"

namespace ioda {
class Type;

/// Basic pre-defined types (Python convenience wrappers)
/// \see py_ioda.cpp
/// \note Names here do not match the python equivalents. The
///   Python names match numpy's definitions.
enum class BasicTypes {
  undefined_,  ///< Internal use only
  float_,
  double_,
  ldouble_,
  char_,
  short_,
  ushort_,
  int_,
  uint_,
  lint_,
  ulint_,
  llint_,
  ullint_,
  int32_,
  uint32_,
  int16_,
  uint16_,
  int64_,
  uint64_,
  bool_,
  str_
};

namespace detail {
IODA_DL size_t COMPAT_strncpy_s(char* dest, size_t destSz, const char* src, size_t srcSz);

class Type_Backend;

template <class Type_Implementation = Type>
class Type_Base {
  friend class ::ioda::Type;
  std::shared_ptr<Type_Backend> backend_;

protected:
  ::ioda::detail::Type_Provider* provider_;

  /// @name General Functions
  /// @{

  Type_Base(std::shared_ptr<Type_Backend> b, ::ioda::detail::Type_Provider* p)
      : backend_(b), provider_(p) {}

  /// Get the type provider.
  inline detail::Type_Provider* getTypeProvider() const { return provider_; }

  /*
  /// \brief Convenience function to check a type.
  /// \param DataType is the type of the data. I.e. float, int, int32_t, uint16_t, std::string, etc.
  /// \returns True if the type matches
  /// \returns False (0) if the type does not match
  /// \throws if an error occurred.
  template <class DataType>
  bool isA() const {
    Type templateType = Types::GetType_Wrapper<DataType>::GetType(getTypeProvider());

    return isA(templateType);
  }
  /// Hand-off to the backend to check equivalence
  virtual bool isA(Type lhs) const;

  /// Python compatability function
  inline bool isA(BasicTypes dataType) const { return isA(Type(dataType, getTypeProvider())); }
  */
public:
  virtual ~Type_Base() {}
  std::shared_ptr<Type_Backend> getBackend() const { return backend_; }
  bool isValid() const { return (backend_.use_count() > 0); }

  /// \brief Get the size of a single element of a type, in bytes.
  /// \details This function is paired with the read and write functions to allow you to
  /// read and write data in a type-agnostic manner.
  /// This size report is a bit complicated when variable-length strings are encountered.
  /// In these cases, the size of the string pointer is returned.
  virtual size_t getSize() const;

  /// @}
};
}  // namespace detail

/// \brief Represents the "type" (i.e. integer, string, float) of a piece of data.
/// \ingroup ioda_cxx_types
///
/// Generally, you do not have to use this class directly. Attributes and Variables have
/// templated functions that convert your type into the type used internally by ioda.
/// \see Types::GetType and Types::GetType_Wrapper for functions that produce these types.
class IODA_DL Type : public detail::Type_Base<> {
public:
  Type();
  Type(std::shared_ptr<detail::Type_Backend> b, std::type_index t);
  Type(BasicTypes, gsl::not_null<::ioda::detail::Type_Provider*> t);

  virtual ~Type();

  /// @name Type-querying functions
  /// @{

  /// @deprecated This function is problematic since we cannot query a type properly
  /// when loading from a file.
  std::type_index getType() const { return as_type_index_; }
  inline std::type_index operator()() const { return getType(); }
  inline std::type_index get() const { return getType(); }

  /// @}
private:
  std::type_index as_type_index_;
};

namespace detail {

/// Backends inherit from this and they provide their own functions.
/// Lots of std::dynamic_cast, unfortunately.
class IODA_DL Type_Backend : public Type_Base<> {
public:
  virtual ~Type_Backend();
  size_t getSize() const override;

protected:
  Type_Backend();
};

}  // namespace detail

/// \brief Defines the type system used for manipulating IODA objects.
namespace Types {

// using namespace ioda::Handles;

/// \brief Convenience struct to determine if a type can represent a string.
/// \ingroup ioda_cxx_types
/// \todo extend to UTF-8 strings, as HDF5 supports these. No support for UTF-16, but conversion
/// functions may be applied.
/// \todo Fix for "const std::string".
template <typename T>
struct is_string : public std::integral_constant<
                     bool, std::is_same<char*, typename std::decay<T>::type>::value
                             || std::is_same<const char*, typename std::decay<T>::type>::value> {};
/// \brief Convenience struct to determine if a type can represent a string.
/// \ingroup ioda_cxx_types
template <>
struct is_string<std::string> : std::true_type {};

/// Useful compile-time definitions.
namespace constants {
/// \note Different than ObsSpace variable-length dimension. This is for a Type.
constexpr size_t _Variable_Length = 0;
// constexpr int _Not_An_Array_type = -3;
}  // namespace constants

/// \brief For fundamental, non-string types.
/// \ingroup ioda_cxx_types
template <class DataType, int Array_Type_Dimensionality = 0>
Type GetType(gsl::not_null<const ::ioda::detail::Type_Provider*> t,
             std::initializer_list<Dimensions_t> Adims                   = {},
             typename std::enable_if<!is_string<DataType>::value>::type* = 0) {
  if (Array_Type_Dimensionality <= 0)
    throw Exception(
      "Bad assertion / unsupported fundamental type at the frontend side "
      "of the ioda type system.", ioda_Here());
  else
    return t->makeArrayType(Adims, typeid(DataType[]), typeid(DataType));
}
/// \brief For fundamental string types. These are either constant or variable length arrays.
/// Separate handling elsewhere.
/// \ingroup ioda_cxx_types
template <class DataType, int String_Type_Length = constants::_Variable_Length>
Type GetType(gsl::not_null<const ::ioda::detail::Type_Provider*> t,
             std::initializer_list<Dimensions_t>                        = {},
             typename std::enable_if<is_string<DataType>::value>::type* = 0) {
  return t->makeStringType(String_Type_Length, typeid(DataType));
}

// This macro just repeats a long definition

/// @def IODA_ADD_FUNDAMENTAL_TYPE
/// Macro that defines a "fundamental type" that needs to be supported
/// by the backend. These match C++11.
/// \ingroup ioda_cxx_types
/// \see https://en.cppreference.com/w/cpp/language/types
/// \since C++11: we use bool, short int, unsigned short int,
///   int, unsigned int, long int, unsigned long int,
///   long long int, unsigned long long int,
///   signed char, unsigned char, char,
///   wchar_t, char16_t, char32_t,
///   float, double, long double.
/// \since C++20: we also add char8_t.
#define IODA_ADD_FUNDAMENTAL_TYPE(x)                                                               \
  template <>                                                                                      \
  inline Type GetType<x, 0>(gsl::not_null<const ::ioda::detail::Type_Provider*> t,                 \
                            std::initializer_list<Dimensions_t>, void*) {                          \
    return t->makeFundamentalType(typeid(x));                                                      \
  }

IODA_ADD_FUNDAMENTAL_TYPE(bool);
IODA_ADD_FUNDAMENTAL_TYPE(short int);
IODA_ADD_FUNDAMENTAL_TYPE(unsigned short int);
IODA_ADD_FUNDAMENTAL_TYPE(int);
IODA_ADD_FUNDAMENTAL_TYPE(unsigned int);
IODA_ADD_FUNDAMENTAL_TYPE(long int);
IODA_ADD_FUNDAMENTAL_TYPE(unsigned long int);
IODA_ADD_FUNDAMENTAL_TYPE(long long int);
IODA_ADD_FUNDAMENTAL_TYPE(unsigned long long int);
IODA_ADD_FUNDAMENTAL_TYPE(signed char);
IODA_ADD_FUNDAMENTAL_TYPE(unsigned char);
IODA_ADD_FUNDAMENTAL_TYPE(char);
IODA_ADD_FUNDAMENTAL_TYPE(wchar_t);
IODA_ADD_FUNDAMENTAL_TYPE(char16_t);
IODA_ADD_FUNDAMENTAL_TYPE(char32_t);
// IODA_ADD_FUNDAMENTAL_TYPE(char8_t); // C++20
IODA_ADD_FUNDAMENTAL_TYPE(float);
IODA_ADD_FUNDAMENTAL_TYPE(double);
IODA_ADD_FUNDAMENTAL_TYPE(long double);

#undef IODA_ADD_FUNDAMENTAL_TYPE

/*
/// Used in an example. Incomplete.
/// \todo Pop off std::array as a 1-D object
template<> inline Type GetType<std::array<int,2>, 0>
        (gsl::not_null<const ::ioda::detail::Type_Provider*> t,
        std::initializer_list<Dimensions_t>, void*) {
        return t->makeArrayType({2}, typeid(std::array<int,2>), typeid(int)); }

*/

/*
template <class DataType, int Array_Type_Dimensionality = 0>
Type GetType(
        gsl::not_null<const ::ioda::detail::Type_Provider*> t,
        std::initializer_list<Dimensions_t> Adims = {},
        typename std::enable_if<!is_string<DataType>::value>::type* = 0);
template <class DataType, int String_Type_Length = constants::_Variable_Length>
Type GetType(
        gsl::not_null<const ::ioda::detail::Type_Provider*> t,
        typename std::enable_if<is_string<DataType>::value>::type* = 0);
        */

/// \brief Wrapper struct to call GetType. Needed because of C++ template rules.
/// \ingroup ioda_cxx_types
/// \see ioda::Attribute, ioda::Has_Attributes, ioda::Variable, ioda::Has_Variables
template <class DataType,
          int Length = 0>  //, typename = std::enable_if_t<!is_string<DataType>::value>>
struct GetType_Wrapper {
  static Type GetType(gsl::not_null<const ::ioda::detail::Type_Provider*> t) {
    /// \note Currently breaks array types, but these are not yet used.
    return ::ioda::Types::GetType<DataType, Length>(t, {Length});
  }
};
/// \ingroup ioda_cxx_types
typedef std::function<Type(gsl::not_null<const ::ioda::detail::Type_Provider*>)>
  TypeWrapper_function;
/*
template <class DataType, int Length = 0, typename = std::enable_if_t<is_string<DataType>::value>>
struct GetType_Wrapper {
        Type GetType(gsl::not_null<const ::ioda::detail::Type_Provider*> t) const {
                // string split
                return ::ioda::Types::GetType<DataType, Length>(t);
        }
};
*/

// inline Encapsulated_Handle GetTypeFixedString(Dimensions_t sz);
}  // namespace Types
}  // namespace ioda

/// @}
