/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \addtogroup ioda_internals_engines_obsstore
 *
 * @{
 * \file ObsStore-attributes.cpp
 * \brief Functions for ObsStore Attribute and Has_Attributes
 */
#include "./ObsStore-attributes.h"

#include "./ObsStore-types.h"
#include "ioda/Misc/Dimensions.h"

namespace ioda {
namespace Engines {
namespace ObsStore {
//**********************************************************************
// ObsStore_Attribute_Backend functions
//**********************************************************************
ObsStore_Attribute_Backend::ObsStore_Attribute_Backend() = default;
ObsStore_Attribute_Backend::ObsStore_Attribute_Backend(std::shared_ptr<ioda::ObsStore::Attribute> h)
    : backend_(h) {}
ObsStore_Attribute_Backend::~ObsStore_Attribute_Backend() = default;

detail::Type_Provider* ObsStore_Attribute_Backend::getTypeProvider() const {
  return ObsStore_Type_Provider::instance();
}

Attribute ObsStore_Attribute_Backend::write(gsl::span<char> data, const Type& in_memory_dataType) {
  // Convert to an obs store data type
  auto typeBackend = std::dynamic_pointer_cast<ObsStore_Type>(in_memory_dataType.getBackend());
  ioda::ObsStore::ObsTypes dtype = typeBackend->dtype();
  backend_->write(data, dtype);
  return Attribute{shared_from_this()};
}

Attribute ObsStore_Attribute_Backend::read(gsl::span<char> data,
                                           const Type& in_memory_dataType) const {
  // Convert to an obs store data type
  auto typeBackend = std::dynamic_pointer_cast<ObsStore_Type>(in_memory_dataType.getBackend());
  ioda::ObsStore::ObsTypes dtype = typeBackend->dtype();

  backend_->read(data, dtype);
  // Need to construct a shared_ptr to "this", instead of using
  // shared_from_this() because of the const qualifier on this method.
  return Attribute{std::make_shared<ObsStore_Attribute_Backend>(*this)};
}

Type ObsStore_Attribute_Backend::getType() const {
  auto backend_type = backend_->dtype();
  ObsTypeInfo typ{backend_type.first, backend_type.second};
  return Type{std::make_shared<ObsStore_Type>(typ), typeid(ObsStore_Type)};
}

bool ObsStore_Attribute_Backend::isA(Type lhs) const {
  auto typeBackend               = std::dynamic_pointer_cast<ObsStore_Type>(lhs.getBackend());
  ioda::ObsStore::ObsTypes dtype = typeBackend->dtype();
  return backend_->isOfType(dtype);
}

Dimensions ObsStore_Attribute_Backend::getDimensions() const {
  // Convert to Dimensions types
  std::vector<std::size_t> attrDims = backend_->get_dimensions();
  std::vector<Dimensions_t> iodaDims;
  std::size_t numElems = 1;
  for (std::size_t i = 0; i < attrDims.size(); ++i) {
    iodaDims.push_back(gsl::narrow<Dimensions_t>(attrDims[i]));
    numElems *= attrDims[i];
  }

  // Create and return a Dimensions object
  auto iodaRank     = gsl::narrow<Dimensions_t>(iodaDims.size());
  auto iodaNumElems = gsl::narrow<Dimensions_t>(numElems);
  Dimensions dims(iodaDims, iodaDims, iodaRank, iodaNumElems);
  return dims;
}

//**********************************************************************
// ObsStore_HasAttributes_Backend functions
//**********************************************************************
ObsStore_HasAttributes_Backend::ObsStore_HasAttributes_Backend() : backend_(nullptr) {}
ObsStore_HasAttributes_Backend::ObsStore_HasAttributes_Backend(
  std::shared_ptr<ioda::ObsStore::Has_Attributes> b)
    : backend_(b) {}
ObsStore_HasAttributes_Backend::~ObsStore_HasAttributes_Backend() = default;

detail::Type_Provider* ObsStore_HasAttributes_Backend::getTypeProvider() const {
  return ObsStore_Type_Provider::instance();
}

std::vector<std::string> ObsStore_HasAttributes_Backend::list() const { return backend_->list(); }

bool ObsStore_HasAttributes_Backend::exists(const std::string& attname) const {
  return backend_->exists(attname);
}

void ObsStore_HasAttributes_Backend::remove(const std::string& attname) {
  return backend_->remove(attname);
}

Attribute ObsStore_HasAttributes_Backend::open(const std::string& attrname) const {
  auto res = backend_->open(attrname);
  auto b   = std::make_shared<ObsStore_Attribute_Backend>(res);
  Attribute att{b};
  return att;
}

Attribute ObsStore_HasAttributes_Backend::create(const std::string& attrname,
                                                 const Type& in_memory_dataType,
                                                 const std::vector<Dimensions_t>& dimensions) {
  /// Convert to an obs store data type
  auto typeBackend = std::dynamic_pointer_cast<ObsStore_Type>(in_memory_dataType.getBackend());
  ioda::ObsStore::ObsTypes dtype = typeBackend->dtype();

  // Convert to obs store dimensions
  std::vector<std::size_t> dims;
  for (std::size_t i = 0; i < dimensions.size(); ++i) {
    dims.push_back(gsl::narrow<std::size_t>(dimensions[i]));
  }

  auto res = backend_->create(attrname, dtype, dims);
  auto b   = std::make_shared<ObsStore_Attribute_Backend>(res);
  Attribute att{b};
  return att;
}

void ObsStore_HasAttributes_Backend::rename(const std::string& oldName,
                                            const std::string& newName) {
  backend_->rename(oldName, newName);
}
}  // namespace ObsStore
}  // namespace Engines
}  // namespace ioda

/// @}
