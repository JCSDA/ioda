/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include "ioda/Group.h"

#include "ioda/Engines/Capabilities.h"

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

Engines::Capabilities Group_Base::getCapabilities() const { return backend_->getCapabilities(); }

std::vector<std::string> Group_Base::list() const {
  // Not backend_->... deliberately because we might call this from a backend directly.
  return listObjects(ObjectType::Group, false)[ObjectType::Group];
}

std::map<ObjectType, std::vector<std::string>> Group_Base::listObjects(ObjectType filter,
                                                                       bool recurse) const {
  return backend_->listObjects(filter, recurse);
}

bool Group_Base::exists(const std::string& name) const { return backend_->exists(name); }

Group Group_Base::create(const std::string& name) { return backend_->create(name); }

Group Group_Base::open(const std::string& name) const { return backend_->open(name); }

FillValuePolicy Group_Base::getFillValuePolicy() const { return backend_->getFillValuePolicy(); }

FillValuePolicy Group_Backend::getFillValuePolicy() const { return vars.getFillValuePolicy(); }

}  // namespace detail
}  // namespace ioda
