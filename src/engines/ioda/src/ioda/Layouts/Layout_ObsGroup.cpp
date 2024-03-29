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
 * \file Layout_ObsGroup.cpp
 * \brief Contains definitions for how data are arranged in ioda internally.
 */

#include "./Layout_ObsGroup.h"

#include "ioda/Group.h"
#include "ioda/Layout.h"
#include "ioda/Misc/StringFuncs.h"
#include "ioda/defs.h"

namespace ioda {
namespace detail {
const int32_t DataLayoutPolicy_ObsGroup::ObsGroup_Layout_Version = 0;
DataLayoutPolicy_ObsGroup::~DataLayoutPolicy_ObsGroup()    = default;
DataLayoutPolicy_ObsGroup::DataLayoutPolicy_ObsGroup()     = default;
void DataLayoutPolicy_ObsGroup::initializeStructure(Group_Base &g) const {
  // Set attributes to indicate that the data are managed
  // by this data policy.

  // Old names. To be deprecated.
  g.atts.add<std::string>("_ioda_layout", std::string("ObsGroup"));
  g.atts.add<int32_t>("_ioda_layout_version", ObsGroup_Layout_Version);

  // New names. These will invalidate the ioda converters tests.
  //g.atts.add<std::string>("ioda_object_type", std::string("ObsGroup"));
  //g.atts.add<int32_t>("ioda_object_version", ObsGroup_Layout_Version);

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
