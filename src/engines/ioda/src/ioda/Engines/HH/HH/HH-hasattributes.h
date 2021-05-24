#pragma once
/*
 * (C) Copyright 2017-2020 Ryan Honeyager (ryan@honeyager.info)
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \addtogroup ioda_internals_engines_hh
 *
 * @{
 * \file HH-hasattributes.h
 * \brief HDF5 engine implementation of Has_Attributes.
 */

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
/// \ingroup ioda_internals_engines_hh
class IODA_HIDDEN HH_HasAttributes : public ioda::detail::Has_Attributes_Backend,
                                     public std::enable_shared_from_this<HH_HasAttributes> {
private:
  HH_hid_t base_;
  static const hsize_t thresholdLinear = 10;

public:
  HH_HasAttributes();
  HH_HasAttributes(HH_hid_t);
  virtual ~HH_HasAttributes();
  detail::Type_Provider* getTypeProvider() const final;
  std::vector<std::string> list() const final;
  /// @brief Check if an attribute exists.
  /// @param attname is the name of the attribute.
  /// @return true if exists, false otherwise.
  /// @details This uses an optimized search.
  /// @see open for search details.
  bool exists(const std::string& attname) const final;
  void remove(const std::string& attname) final;
  /// @brief Open an attribute
  /// @param name is the name of the attribute
  /// @return The opened attribute
  /// @details This uses an optimized search. If the number of attributes in the container is
  ///   less than ten, performs a linear search. Otherwise, it uses the usual H5Aopen call.
  Attribute open(const std::string& name) const final;
  Attribute create(const std::string& attrname, const Type& in_memory_dataType,
                   const std::vector<Dimensions_t>& dimensions = {1}) final;
  void rename(const std::string& oldName, const std::string& newName) final;
};
}  // namespace HH
}  // namespace Engines
}  // namespace detail
}  // namespace ioda

/// @}
