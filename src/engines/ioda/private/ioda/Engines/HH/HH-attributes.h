#pragma once
/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include <string>
#include <vector>

#include "HH/Files.hpp"
#include "ioda/Group.h"
#include "ioda/defs.h"

namespace ioda {
namespace detail {
namespace Engines {
namespace HH {
// Spurious warning on Intel compilers:
// https://stackoverflow.com/questions/2571850/why-does-enable-shared-from-this-have-a-non-virtual-destructor
#if defined(__INTEL_COMPILER)
#pragma warning(push)
#pragma warning(disable : 444)
#endif
/// \brief This is the implementation of Attributes using HDF5. Do not use outside of IODA.
class HH_Attribute_Backend : public ioda::detail::Attribute_Backend,
                             public std::enable_shared_from_this<HH::HH_Attribute_Backend> {
  ::HH::Attribute backend_;
  detail::Type_Provider* getTypeProvider() const final;

public:
  HH_Attribute_Backend();
  HH_Attribute_Backend(::HH::Attribute);
  virtual ~HH_Attribute_Backend();
  Attribute write(gsl::span<char> data, const Type& in_memory_dataType) final;
  // Attribute writeFixedLengthString(const std::string& data) override;
  Attribute read(gsl::span<char> data, const Type& in_memory_dataType) const final;
  bool isA(Type lhs) const final;
  Dimensions getDimensions() const final;
};

/// \brief This is the implementation of Has_Attributes using HDF5. Do not use outside of IODA.
class HH_HasAttributes_Backend : public ioda::detail::Has_Attributes_Backend,
                                 public std::enable_shared_from_this<HH_HasAttributes_Backend> {
  ::HH::Has_Attributes backend_;

public:
  HH_HasAttributes_Backend();
  HH_HasAttributes_Backend(::HH::Has_Attributes);
  virtual ~HH_HasAttributes_Backend();
  detail::Type_Provider* getTypeProvider() const final;
  std::vector<std::string> list() const final;
  bool exists(const std::string& attname) const final;
  void remove(const std::string& attname) final;
  Attribute open(const std::string& name) const final;
  Attribute create(const std::string& attrname, const Type& in_memory_dataType,
                   const std::vector<Dimensions_t>& dimensions = {1}) final;
  void rename(const std::string& oldName, const std::string& newName) final;
};
#if defined(__INTEL_COMPILER)
#pragma warning(pop)
#endif
}  // namespace HH
}  // namespace Engines
}  // namespace detail
}  // namespace ioda
