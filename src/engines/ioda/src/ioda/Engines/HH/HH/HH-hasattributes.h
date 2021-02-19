#pragma once
/*
 * (C) Copyright 2017-2020 Ryan Honeyager (ryan@honeyager.info)
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// \file HH-hasattributes.h
/// \brief HDF5 engine implementation of HasAttributes.

#include <string>
#include <vector>

#include "./Handles.h"
#include "ioda/Group.h"
#include "ioda/defs.h"

namespace ioda {
namespace detail {
namespace Engines {
namespace HH {
/// \brief This is the implementation of Has_Attributes using HDF5.
class IODA_HIDDEN HH_HasAttributes : public ioda::detail::Has_Attributes_Backend,
                                     public std::enable_shared_from_this<HH_HasAttributes> {
private:
  HH_hid_t base_;

public:
  HH_HasAttributes();
  HH_HasAttributes(HH_hid_t);
  virtual ~HH_HasAttributes();
  detail::Type_Provider* getTypeProvider() const final;
  std::vector<std::string> list() const final;
  bool exists(const std::string& attname) const final;
  void remove(const std::string& attname) final;
  Attribute open(const std::string& name) const final;
  Attribute create(const std::string& attrname, const Type& in_memory_dataType,
                   const std::vector<Dimensions_t>& dimensions = {1}) final;
  void rename(const std::string& oldName, const std::string& newName) final;
};
}  // namespace HH
}  // namespace Engines
}  // namespace detail
}  // namespace ioda
