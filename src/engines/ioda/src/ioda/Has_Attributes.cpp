/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include "ioda/Attributes/Has_Attributes.h"
#include "ioda/Exception.h"

namespace ioda {
namespace detail {
Has_Attributes_Base::Has_Attributes_Base(std::shared_ptr<Has_Attributes_Backend> b) : backend_(b) {}
Has_Attributes_Base::~Has_Attributes_Base() = default;

Has_Attributes_Backend::Has_Attributes_Backend() : Has_Attributes_Base(nullptr) {}
Has_Attributes_Backend::~Has_Attributes_Backend() = default;

std::vector<std::string> Has_Attributes_Base::list() const {
  try {
    if (backend_ == nullptr)
      throw Exception("Missing backend or unimplemented backend function.", ioda_Here());
    return backend_->list();
  } catch (...) {
    std::throw_with_nested(
      Exception("An exception occurred inside ioda while listing attributes of an object.",
                ioda_Here()));
  }
}

bool Has_Attributes_Base::exists(const std::string& attname) const {
  try {
    if (backend_ == nullptr)
      throw Exception("Missing backend or unimplemented backend function.", ioda_Here());
    return backend_->exists(attname);
  } catch (...) {
    std::throw_with_nested(Exception(
      "An exception occurred inside ioda while checking existence of an attribute.", ioda_Here())
      .add("attname", attname));
  }
}

void Has_Attributes_Base::remove(const std::string& attname) {
  try {
    if (backend_ == nullptr)
      throw Exception("Missing backend or unimplemented backend function.", ioda_Here());
    return backend_->remove(attname);
  } catch (...) {
    std::throw_with_nested(Exception(
      "An exception occurred inside ioda while removing an attribute.", ioda_Here())
      .add("attname", attname));
  }
}

Attribute Has_Attributes_Base::open(const std::string& name) const {
  try {
    if (backend_ == nullptr)
      throw Exception("Missing backend or unimplemented backend function.", ioda_Here());
    return backend_->open(name);
  } catch (...) {
    std::throw_with_nested(Exception(
      "An exception occurred inside ioda while opening an attribute.", ioda_Here())
      .add("name", name));
  }
}

std::vector<std::pair<std::string, Attribute>> Has_Attributes_Base::openAll() const {
  try {
    if (backend_ == nullptr)
      throw Exception("Missing backend or unimplemented backend function.", ioda_Here());
    return backend_->openAll();
  } catch (...) {
    std::throw_with_nested(Exception(
      "An exception occurred in ioda while opening all attributes of an object.", ioda_Here()));
  }
}

std::vector<std::pair<std::string, Attribute>> Has_Attributes_Backend::openAll() const {
  try {
    using namespace std;
    vector<pair<string, Attribute>> ret;
    vector<string> names = list();
    for (const auto& name : names) ret.push_back(make_pair(name, open(name)));

    return ret;
  } catch (...) {
    std::throw_with_nested(Exception(
      "An exception occurred in ioda while opening all attributes of an object.", ioda_Here()));
  }
}

void Has_Attributes_Base::rename(const std::string& oldName, const std::string& newName) {
  try {
    if (backend_ == nullptr)
      throw Exception("Missing backend or unimplemented backend function.", ioda_Here());
    return backend_->rename(oldName, newName);
  } catch (...) {
    std::throw_with_nested(Exception(
      "An exception occurred in ioda while renaming an attribute.", ioda_Here())
      .add("oldName", oldName).add("newName", newName));
  }
}

detail::Type_Provider* Has_Attributes_Base::getTypeProvider() const {
  try {
    if (backend_ == nullptr)
      throw Exception("Missing backend or unimplemented backend function.", ioda_Here());
    return backend_->getTypeProvider();
  } catch (...) {
    std::throw_with_nested(Exception(
      "An exception occurred in ioda while getting a Type Provider.", ioda_Here()));
  }
}

Attribute Has_Attributes_Base::create(const std::string& attrname, const Type& in_memory_dataType,
                                      const std::vector<Dimensions_t>& dimensions) {
  try {
    if (backend_ == nullptr)
      throw Exception("Missing backend or unimplemented backend function.", ioda_Here());

    // Apparently netcdf4 loves to declare string types with no dimensions, and
    // it is perfectly valid to declare a zero-dimensionality object. Scalar dimensions
    // instead of simple dimensions. Go figure.
    //Expects(dimensions.size());
    for (const auto& d : dimensions) {
      if (d < 0) throw Exception("Invalid dimension length.", ioda_Here());
    }

    auto att = backend_->create(attrname, in_memory_dataType, dimensions);
    return att;

  } catch (...) {
    std::throw_with_nested(Exception(
      "An exception occurred inside ioda while creating an attribute.", ioda_Here())
      .add("attrname", attrname));
  }
}

}  // namespace detail

Has_Attributes::Has_Attributes() : detail::Has_Attributes_Base(nullptr) {}
Has_Attributes::~Has_Attributes() = default;
Has_Attributes::Has_Attributes(std::shared_ptr<detail::Has_Attributes_Backend> g)
    : detail::Has_Attributes_Base(g) {}

}  // namespace ioda
