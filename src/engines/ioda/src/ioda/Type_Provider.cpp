/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include "ioda/Types/Type_Provider.h"

#include <stdexcept>

#include "ioda/Types/Type.h"
#include "ioda/defs.h"
#include "ioda/Exception.h"

namespace ioda {
namespace detail {
Type_Provider::~Type_Provider() = default;

Type Type_Provider::makeFundamentalType(std::type_index) const {
  throw Exception("Backend does not implement fundamental types.", ioda_Here());
}

Type Type_Provider::makeArrayType(std::initializer_list<Dimensions_t>, std::type_index,
                                  std::type_index) const {
  throw Exception("Backend does not implement array types.", ioda_Here());
}

Type Type_Provider::makeStringType(size_t, std::type_index) const {
  throw Exception("Backend does not implement string types.", ioda_Here());
}

PointerOwner Type_Provider::getReturnedPointerOwner() const { return PointerOwner::Caller; }
}  // namespace detail
}  // namespace ioda
