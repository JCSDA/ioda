#pragma once
/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \defgroup ioda_cxx_layout_internal Internal Groups and Data Layout
 * \brief Private API
 * \ingroup ioda_internals
 *
 * @{
 * \file Layout_ObsGroup.h
 * \brief Contains definitions for how data are arranged in ioda internally.
 */

#include <memory>
#include <string>

#include "ioda/Layout.h"
#include "ioda/defs.h"

namespace ioda {
namespace detail {
/// Layout for ObsGroup-like data.
class IODA_DL DataLayoutPolicy_ObsGroup : public DataLayoutPolicy {
  /// \brief Record versioning information for this layout in the ioda object. Provides forward compatability.
  const int32_t ObsGroup_Layout_Version = 0;

public:
  virtual ~DataLayoutPolicy_ObsGroup();
  void initializeStructure(Group_Base &) const override;
  std::string doMap(const std::string &) const override;
  DataLayoutPolicy_ObsGroup();
  // doMap is not present, as the default policy works just fine.
  /// A descriptive name for the policy.
  std::string name() const override;
};

}  // namespace detail
}  // namespace ioda

/// @}
