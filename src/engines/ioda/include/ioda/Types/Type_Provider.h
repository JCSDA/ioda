#pragma once
/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// \file Type_Provider.h
/// \brief Frontend/backend bindings for the type system.
#include <initializer_list>
#include <typeindex>
#include <typeinfo>

#include "ioda/defs.h"

namespace ioda {
class Type;  // This file is included by Type.h.

namespace detail {
/// Who owns (and should free) pointers passed across the
/// frontend / backend interface?
enum class PointerOwner {
  Engine,  ///< The backend engine frees pointers that it provides.
  Caller   ///< The user has to free pointers.
};

/// Backends implement type providers in conjunction with
/// Attributes, Has_Attributes, Variables and Has_Variables.
/// The backend objects pass through their underlying logic to represent types.
class IODA_DL Type_Provider {
public:
  virtual ~Type_Provider();
  /// Make a basic object type, like a double, a float, or a char.
  virtual Type makeFundamentalType(std::type_index type) const;
  /// Make an array type, like a double[2].
  virtual Type makeArrayType(std::initializer_list<Dimensions_t> dimensions, std::type_index typeOuter,
                             std::type_index typeInner) const;
  /// Make a variable-length string type.
  virtual Type makeStringType(size_t string_length, std::type_index typeOuter) const;

  /// When a pointer is passed from the backend to the frontend, who has to free it?
  virtual PointerOwner getReturnedPointerOwner() const;
};
}  // namespace detail
}  // namespace ioda
