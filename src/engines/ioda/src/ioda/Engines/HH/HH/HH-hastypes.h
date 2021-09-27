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
 * \file HH-hastypes.h
 * \brief HDF5 engine implementation of Has_Types.
 */

#include <memory>
#include <string>
#include <vector>

#include "./Handles.h"
#include "ioda/Types/Has_Types.h"
#include "ioda/defs.h"

namespace ioda {
namespace detail {
namespace Engines {
namespace HH {
/// \brief This is the implementation of Has_Variables using HDF5.
class IODA_HIDDEN HH_HasTypes : public ioda::detail::Has_Types_Backend,
                                public std::enable_shared_from_this<HH_HasTypes> {
  HH_hid_t base_;

public:
  HH_HasTypes();
  HH_HasTypes(HH_hid_t grp);
  virtual ~HH_HasTypes();
  detail::Type_Provider* getTypeProvider() const final;
  bool exists(const std::string& name) const final;
  void remove(const std::string& name) final;
  Type open(const std::string& name) const final;
  std::vector<std::string> list() const final;
};
}  // namespace HH
}  // namespace Engines
}  // namespace detail
}  // namespace ioda

/// @}
