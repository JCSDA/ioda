/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include "ioda/Attributes/Has_Attributes.h"

namespace ioda {
namespace detail {
Has_Attributes_Base::Has_Attributes_Base(std::shared_ptr<Has_Attributes_Backend> b) : backend_(b) {}
Has_Attributes_Base::~Has_Attributes_Base() = default;

Has_Attributes_Backend::Has_Attributes_Backend() : Has_Attributes_Base(nullptr) {}
Has_Attributes_Backend::~Has_Attributes_Backend() = default;

std::vector<std::string> Has_Attributes_Base::list() const {
  Expects(backend_ != nullptr && "Unimplemented function for backend");
  return backend_->list();
}
bool Has_Attributes_Base::exists(const std::string& attname) const {
  Expects(backend_ != nullptr && "Unimplemented function for backend");
  return backend_->exists(attname);
}
void Has_Attributes_Base::remove(const std::string& attname) {
  Expects(backend_ != nullptr && "Unimplemented function for backend");
  return backend_->remove(attname);
}
Attribute Has_Attributes_Base::open(const std::string& name) const {
  Expects(backend_ != nullptr && "Unimplemented function for backend");
  return backend_->open(name);
}
void Has_Attributes_Base::rename(const std::string& oldName, const std::string& newName) {
  Expects(backend_ != nullptr && "Unimplemented function for backend");
  return backend_->rename(oldName, newName);
}

detail::Type_Provider* Has_Attributes_Base::getTypeProvider() const {
  Expects(backend_ != nullptr);
  return backend_->getTypeProvider();
}

Attribute Has_Attributes_Base::create(const std::string& attrname, const Type& in_memory_dataType,
                                      const std::vector<Dimensions_t>& dimensions) {
  Expects(dimensions.size());
  for (const auto& d : dimensions) {
    Expects(d > 0);
  }
  Expects(backend_ != nullptr);
  auto att = backend_->create(attrname, in_memory_dataType, dimensions);
  return att;
}

}  // namespace detail

Has_Attributes::Has_Attributes() : detail::Has_Attributes_Base(nullptr) {}
Has_Attributes::~Has_Attributes() = default;
Has_Attributes::Has_Attributes(std::shared_ptr<detail::Has_Attributes_Backend> g)
    : detail::Has_Attributes_Base(g) {}

}  // namespace ioda
