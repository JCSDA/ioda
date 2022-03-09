#pragma once
/*
 * (C) Copyright 2020-2022 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \addtogroup ioda_internals_engines_types
 *
 * @{
 * \file Type_Provider.h
 * \brief Frontend/backend bindings for the type system.
 */
#include <initializer_list>
#include <typeindex>
#include <typeinfo>

#include "ioda/defs.h"

namespace ioda {
class Type;  // This file is included by Type.h.

/// \brief The character set used in this string type.
enum class StringCSet {
  ASCII,  ///< ASCII character set
  UTF8    ///< UTF-8 character set
};

namespace detail {
/// \brief Who owns (and should free) pointers passed across the
///   frontend / backend interface?
/// \ingroup ioda_internals_engines_types
enum class PointerOwner {
  Engine,  ///< The backend engine frees pointers that it provides.
  Caller   ///< The user has to free pointers.
};

/// \brief Backends implement type providers in conjunction with
///   Attributes, Has_Attributes, Variables and Has_Variables.
///   The backend objects pass through their underlying logic to represent types.
/// \ingroup ioda_internals_engines_types
class IODA_DL Type_Provider {
public:
  virtual ~Type_Provider();
  /// \brief Make a basic object type, like a double, a float, or a char.
  /// \details Internally, these types already exist, and this function returns
  ///   a type based on a lookup in a map of type_indices.
  /// \param type is a typeid to be matched.
  /// \todo Add another function to make custom integer or float types.
  virtual Type makeFundamentalType(std::type_index type) const;
  /// \brief Make a fixed-length numeric array type, like a double[2].
  /// \param dimensions are the dimensions of the array type.
  /// \param typeOuter is a label that describes the overall type.
  /// \param typeInner is a label that describes the interior type (double[2] -> double).
  /// \todo Remove the type labels in the future? These are used by ObsStore.
  /// \todo Change definition to take the inner type as a parameter instead of using a type_index.
  virtual Type makeArrayType(std::initializer_list<Dimensions_t> dimensions,
                             std::type_index typeOuter, std::type_index typeInner) const;
  /// \brief Make a variable-length string type.
  /// \param typeOuter is a typeid used as a label. (TODO: delete this?)
  /// \param string_length is the length of the string type. '0' denotes a variable-length string.
  ///   Any nonzero positive integer denotes a fixed-length string.
  /// \param cset is the character set used by the string type. ASCII or UTF8.
  virtual Type makeStringType(std::type_index typeOuter, size_t string_length = 0,
                              StringCSet cset = StringCSet::UTF8) const;

  /// \brief Python convenience function.
  /// \deprecated This will by superseded very soon by the IODA type system refactor.
  Type _py_makeStringType(size_t stringLength = 0, StringCSet cset = StringCSet::UTF8) const;

  /// When a pointer is passed from the backend to the frontend, who has to free it?
  virtual PointerOwner getReturnedPointerOwner() const;
};
}  // namespace detail
}  // namespace ioda

/// @}
