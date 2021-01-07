#pragma once
/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "./HH-attributes.h"
#include "./HH-variables.h"
#include "HH/Files.hpp"
#include "ioda/Attributes/Has_Attributes.h"
#include "ioda/Engines/Capabilities.h"
#include "ioda/Engines/HH.h"
#include "ioda/Group.h"
#include "ioda/defs.h"

namespace ioda {
namespace detail {
namespace Engines {
namespace HH {
/// \brief This is the implementation of Groups using HDF5. Do not use outside of IODA.
/// \todo This header should really be stored in a private location.
class IODA_HIDDEN HH_Group_Backend : public ioda::detail::Group_Backend {
  std::shared_ptr<::HH::Group> backend_;
  std::shared_ptr<::HH::File> fileroot_;
  ::ioda::Engines::Capabilities caps_;

public:
  HH_Group_Backend(::HH::Group grp, ::ioda::Engines::Capabilities caps, ::HH::File fileroot)
      : backend_(std::make_shared<::HH::Group>(grp)),
        fileroot_(std::make_shared<::HH::File>(fileroot)),
        caps_(caps) {
    atts = Has_Attributes(std::make_shared<HH_HasAttributes_Backend>(backend_->atts));
    vars = Has_Variables(std::make_shared<HH_HasVariables_Backend>(backend_->dsets, fileroot));
  }

  virtual ~HH_Group_Backend() {}
  ::ioda::Engines::Capabilities getCapabilities() const final { return caps_; };
  bool exists(const std::string& name) const final { return backend_->exists(name); }
  Group create(const std::string& name) final;
  Group open(const std::string& name) const final;
  /// \brief Fill value policy in HDF5 depends on the current group and the root location.
  /// \details If the file was created by NetCDF4, then use the NetCDF4 policy.
  ///    If the file was created by HDF5, see if the root is an ObsGroup. If it is, use the NETCDF4 policy.
  ///    Otherwise, use the HDF5 policy.
  /// \see HH_HasVariables_Backend for the function implementation. It is located there to avoid code
  /// duplication.
  FillValuePolicy getFillValuePolicy() const final { return vars.getFillValuePolicy(); }

  std::map<ObjectType, std::vector<std::string>> listObjects(ObjectType filter = ObjectType::Ignored,
                                                             bool recurse = false) const final;
};

}  // namespace HH
}  // namespace Engines
}  // namespace detail
}  // namespace ioda
