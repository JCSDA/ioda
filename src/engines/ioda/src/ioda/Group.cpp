/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include "ioda/Group.h"

#include "ioda/Engines/Capabilities.h"
#include "ioda/Exception.h"

namespace ioda {
Group::Group() : Group_Base(nullptr) {}

Group::Group(std::shared_ptr<detail::Group_Backend> backend) : Group_Base(backend) {}

Group::~Group() = default;

namespace detail {
/// \note backend may be nullptr. atts needs special handling in this case.
Group_Base::Group_Base(std::shared_ptr<Group_Backend> backend)
    : backend_(backend),
      atts((backend) ? backend->atts : Has_Attributes()),
      vars((backend) ? backend->vars : Has_Variables()) {}

Group_Base::~Group_Base() = default;

Group_Backend::Group_Backend() : Group_Base(nullptr) {}

Group_Backend::~Group_Backend() = default;

Engines::Capabilities Group_Base::getCapabilities() const { 
  try {
    if (backend_ == nullptr) throw Exception("Missing backend or unimplemented backend function.",
      ioda_Here());
      return backend_->getCapabilities();
  } catch (...) {
    std::throw_with_nested(Exception("An exception occurred inside ioda while "
      "determining backend engine capabilities.", ioda_Here()));
  }
}

std::vector<std::string> Group_Base::list() const {
  try {
    // Not backend_->... deliberately because we might call this from a backend directly.
    return listObjects(ObjectType::Group, false)[ObjectType::Group];
  } catch (...) {
    std::throw_with_nested(Exception("An exception occurred inside ioda while "
      "listing one-level child groups.", ioda_Here()));
  }
}

std::map<ObjectType, std::vector<std::string>> Group_Base::listObjects(ObjectType filter,
                                                                       bool recurse) const {
  try {
    if (backend_ == nullptr) throw Exception("Missing backend or unimplemented backend function.",
      ioda_Here());
    return backend_->listObjects(filter, recurse);
  } catch (...) {
    std::throw_with_nested(Exception("An exception occurred inside ioda while listing objects.",
      ioda_Here()).add("recurse", recurse));
  }
}

bool Group_Base::exists(const std::string& name) const { 
  try {
    if (backend_ == nullptr) throw Exception("Missing backend or unimplemented backend function.",
      ioda_Here());
    return backend_->exists(name);
  } catch (...) {
    std::throw_with_nested(Exception("An exception occurred inside ioda while checking "
      "to see whether a group exists.", ioda_Here()).add("name", name));
  }
}

Group Group_Base::create(const std::string& name) { 
  try {
    if (backend_ == nullptr) throw Exception("Missing backend or unimplemented backend function.",
      ioda_Here());
    return backend_->create(name);
  } catch (...) {
    std::throw_with_nested(Exception("An exception occurred inside ioda while creating a group.",
      ioda_Here()).add("name", name));
  }
}

Group Group_Base::open(const std::string& name) const { 
  try {
    if (backend_ == nullptr) throw Exception("Missing backend or unimplemented backend function.",
      ioda_Here());
    return backend_->open(name);
  } catch (...) {
    std::throw_with_nested(Exception("An exception occurred inside ioda while opening a group.",
      ioda_Here()).add("name", name));
  }
}

FillValuePolicy Group_Base::getFillValuePolicy() const { 
  try {
    if (backend_ == nullptr) throw Exception("Missing backend or unimplemented backend function.",
      ioda_Here());
    return backend_->getFillValuePolicy();
  } catch (...) {
    std::throw_with_nested(Exception("An exception occurred inside ioda while determining "
      "the fill value policy.", ioda_Here()));
  }
}

FillValuePolicy Group_Backend::getFillValuePolicy() const { 
  try {
    return vars.getFillValuePolicy();
  } catch (...) {
    std::throw_with_nested(Exception("An exception occurred inside ioda while determining "
      "the fill value policy.", ioda_Here()));
  }
}

}  // namespace detail
}  // namespace ioda
