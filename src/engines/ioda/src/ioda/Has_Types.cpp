/*
 * (C) Copyright 2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include "ioda/Types/Has_Types.h"
#include "ioda/Exception.h"

#include <stdexcept>

namespace ioda {
namespace detail {

Has_Types_Base::~Has_Types_Base() = default;

Has_Types_Base::Has_Types_Base(std::shared_ptr<Has_Types_Backend> b)
    : backend_{b} {}

Has_Types_Backend::~Has_Types_Backend() = default;

Has_Types_Backend::Has_Types_Backend() : Has_Types_Base(nullptr) {}

Type_Provider* Has_Types_Base::getTypeProvider() const {
  try {
    if (backend_ == nullptr)
      throw Exception("Missing backend or unimplemented backend function.", ioda_Here());
    return backend_->getTypeProvider();
  } catch (...) {
    std::throw_with_nested(Exception(
      "An exception occurred in ioda while getting a backend's type provider interface.",
      ioda_Here()));
  }
}

bool Has_Types_Base::exists(const std::string& name) const {
  try {
    if (backend_ == nullptr)
      throw Exception("Missing backend or unimplemented backend function.", ioda_Here());
    return backend_->exists(name);
  } catch (...) {
    std::throw_with_nested(Exception(
      "An exception occurred inside ioda while checking named type existence.", ioda_Here())
      .add("name", name));
  }
}

void Has_Types_Base::remove(const std::string& name) {
  try {
    if (backend_ == nullptr)
      throw Exception("Missing backend or unimplemented backend function.", ioda_Here());
    backend_->remove(name);
  } catch (...) {
    std::throw_with_nested(Exception(
      "An exception occurred inside ioda while removing a named type.", ioda_Here())
      .add("name", name));
  }
}

Type Has_Types_Base::open(const std::string& name) const {
  try {
    if (backend_ == nullptr)
      throw Exception("Missing backend or unimplemented backend function.", ioda_Here());
    return backend_->open(name);
  } catch (...) {
    std::throw_with_nested(Exception(
      "An exception occurred inside ioda while opening a named type.", ioda_Here())
      .add("name", name));
  }
}

// This is a one-level search. For searching contents of an ObsGroup, you need to
// list the Variables in each child group.
std::vector<std::string> Has_Types_Base::list() const {
  try {
    if (backend_ == nullptr)
      throw Exception("Missing backend or unimplemented backend function.", ioda_Here());
    return backend_->list();
  } catch (...) {
    std::throw_with_nested(Exception(
      "An exception occurred inside ioda while listing one-level child variables of a group.",
      ioda_Here()));
  }
}

}  // namespace detail

Has_Types::~Has_Types() = default;
Has_Types::Has_Types() : Has_Types_Base(nullptr) {}
Has_Types::Has_Types(std::shared_ptr<detail::Has_Types_Backend> b) : Has_Types_Base(b) {}

}  // namespace ioda
