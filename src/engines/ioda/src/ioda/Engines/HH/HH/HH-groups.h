#pragma once
/*
 * (C) Copyright 2017-2020 Ryan Honeyager (ryan@honeyager.info)
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \defgroup ioda_internals_engines_hh HDF5 / HH Engine
 * \brief Implementation of the HDF5 backend.
 * \ingroup ioda_internals_engines
 *
 * @{
 * \file HH-groups.h
 * \brief HDF5 group interface
 */

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "./HH-attributes.h"
#include "./HH-variables.h"
#include "ioda/Attributes/Has_Attributes.h"
#include "ioda/Engines/Capabilities.h"
#include "ioda/Engines/HH.h"
#include "ioda/Group.h"
#include "ioda/Variables/Has_Variables.h"
#include "ioda/defs.h"

namespace ioda {
namespace detail {
namespace Engines {
namespace HH {
/// \brief This is the implementation of Groups using HDF5. Do not use outside of IODA.
/// \ingroup ioda_internals_engines_hh
class IODA_HIDDEN HH_Group : public ioda::detail::Group_Backend {
  HH_hid_t backend_;
  HH_hid_t fileroot_;
  ::ioda::Engines::Capabilities caps_;

public:
  // ioda::Has_Attributes atts;
  // ioda::Has_Variables vars;

  /// @brief Group constructor
  /// @param grp is the HDF5 handle
  /// @param caps are the engine capabilities
  /// @param fileroot is a handle to the root object.
  HH_Group(HH_hid_t grp, ::ioda::Engines::Capabilities caps, HH_hid_t fileroot);

  virtual ~HH_Group() {}

  HH_hid_t get() const { return backend_; }

  inline ::ioda::Engines::Capabilities getCapabilities() const final { return caps_; };
  bool exists(const std::string& name) const final;
  Group create(const std::string& name) final;
  Group open(const std::string& name) const final;

  /// \brief Fill value policy in HDF5 depends on the current group and the root location.
  /// \details If the file was created by NetCDF4, then use the NetCDF4 policy.
  ///    If the file was created by HDF5, see if the root is an ObsGroup. If it is, use the NETCDF4
  ///    policy. Otherwise, use the HDF5 policy.
  /// \see HH_HasVariables for the function implementation. It is located there to avoid code
  /// duplication.
  FillValuePolicy getFillValuePolicy() const final;

  std::map<ObjectType, std::vector<std::string>> listObjects(ObjectType filter
                                                             = ObjectType::Ignored,
                                                             bool recurse = false) const final;
};

}  // namespace HH
}  // namespace Engines
}  // namespace detail
}  // namespace ioda

/// @}
