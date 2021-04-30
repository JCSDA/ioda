/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// \file Layout_ObsGroup.cpp
/// \brief Contains implementations for how data are arranged in ioda internally.

#include "ioda/Layouts/Layout_ObsGroup.h"

#include "ioda/Group.h"
#include "ioda/Layout.h"
#include "ioda/Misc/SFuncs.h"
#include "ioda/defs.h"

namespace ioda {
namespace detail {
DataLayoutPolicy_ObsGroup::~DataLayoutPolicy_ObsGroup() = default;
DataLayoutPolicy_ObsGroup::DataLayoutPolicy_ObsGroup() = default;
void DataLayoutPolicy_ObsGroup::initializeStructure(Group_Base &g) const {
  // First, set an attribute to indicate that the data are managed
  // by this data policy.
  g.atts.add<std::string>("_ioda_layout", std::string("ObsGroup"));
  g.atts.add<int32_t>("_ioda_layout_version", ObsGroup_Layout_Version);

  // Create the default containers - currently ignored as these are
  // dynamically created.
  /*
  g.create("MetaData");
  g.create("ObsBias");
  g.create("ObsError");
  g.create("ObsValue");
  g.create("PreQC");
  */
}

std::string DataLayoutPolicy_ObsGroup::doMap(const std::string &str) const {
  // If the string contains '@', then it needs to be broken into
  // components and reversed. All other strings are passed through untouched.

  std::string out = convertV1PathToV2Path(str);
  return out;
}

std::string DataLayoutPolicy_ObsGroup::name() const { return std::string{"ObsGroup v1"}; }

}  // namespace detail
}  // namespace ioda
