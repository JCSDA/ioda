/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// \file ObsStore-attributes.h
/// \brief Functions for ioda::Attribute and ioda::Has_Attribute backed by ObsStore
#pragma once

#include <memory>
#include <string>
#include <vector>

#include "ioda/Group.h"
#include "ioda/ObsStore/Attributes.hpp"
#include "ioda/ObsStore/Types.hpp"
#include "ioda/defs.h"

namespace ioda {
namespace Engines {
namespace ObsStore {
// Spurious warning on Intel compilers:
// https://stackoverflow.com/questions/2571850/why-does-enable-shared-from-this-have-a-non-virtual-destructor
#if defined(__INTEL_COMPILER)
#pragma warning(push)
#pragma warning(disable : 444)
#endif

/// \brief This is the implementation of Attributes in ioda::ObsStore
class ObsStore_Attribute_Backend : public ioda::detail::Attribute_Backend,
                                   public std::enable_shared_from_this<ObsStore_Attribute_Backend> {
private:
  /// \brief ObsStore Attribute
  std::shared_ptr<ioda::ObsStore::Attribute> backend_;

  /// \brief return an ObsStore type marker
  detail::Type_Provider* getTypeProvider() const final;

public:
  ObsStore_Attribute_Backend();
  ObsStore_Attribute_Backend(std::shared_ptr<ioda::ObsStore::Attribute>);
  virtual ~ObsStore_Attribute_Backend();

  /// \brief transfer data into the ObsStore Attribute
  /// \param data contiguous block of data to transfer
  /// \param in_memory_dataType frontend type marker
  Attribute write(gsl::span<char> data, const Type& in_memory_dataType) final;
  /// \brief transfer data from the ObsStore Attribute
  /// \param data contiguous block of data to transfer
  /// \param in_memory_dataType frontend type marker
  Attribute read(gsl::span<char> data, const Type& in_memory_dataType) const final;

  /// \brief check if requested type matches stored type
  /// \param lhs frontend type marker
  bool isA(Type lhs) const final;

  /// \brief retrieve dimensions of attribute
  Dimensions getDimensions() const final;
};

/// \brief This is the implementation of Has_Attributes in ioda::ObsStore.
class ObsStore_HasAttributes_Backend : public ioda::detail::Has_Attributes_Backend {
private:
  /// \brief ObsStore Has_Attribute
  std::shared_ptr<ioda::ObsStore::Has_Attributes> backend_;

public:
  ObsStore_HasAttributes_Backend();
  ObsStore_HasAttributes_Backend(std::shared_ptr<ioda::ObsStore::Has_Attributes>);
  virtual ~ObsStore_HasAttributes_Backend();

  /// \brief return an ObsStore type marker
  detail::Type_Provider* getTypeProvider() const final;

  /// \brief return list of the names of the attributes in this container
  std::vector<std::string> list() const final;

  /// \brief returns true if attribute is in this container
  /// \param attname name of attribute
  bool exists(const std::string& attname) const final;

  /// \brief remove an attribute from this container
  /// \param attname name of attribute
  void remove(const std::string& attname) final;

  /// \brief open an existing attribute (throws exception if not found)
  /// \param attname name of attribute
  Attribute open(const std::string& attrname) const final;

  /// \brief create a new attribute
  /// \param attname name of attribute
  /// \param in_memory_dataType fronted type marker
  /// \param dimensions dimensions of attribute
  Attribute create(const std::string& attrname, const Type& in_memory_dataType,
                   const std::vector<Dimensions_t>& dimensions = {1}) final;

  /// \brief rename an attribute
  /// \param oldName current name of attribute
  /// \param newName new name for attribute
  void rename(const std::string& oldName, const std::string& newName) final;
};
#if defined(__INTEL_COMPILER)
#pragma warning(pop)
#endif
}  // namespace ObsStore
}  // namespace Engines
}  // namespace ioda
