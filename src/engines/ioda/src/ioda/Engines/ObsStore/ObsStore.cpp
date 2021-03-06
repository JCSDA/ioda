/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \addtogroup ioda_cxx_engines_pub_ObsStore
 *
 * @{
 * \file ObsStore.cpp
 * \brief ObsStore engine bindings
 */
#include "ioda/Engines/ObsStore.h"

#include "./Group.hpp"
#include "./ObsStore-groups.h"
#include "ioda/Group.h"

namespace ioda {
namespace Engines {
namespace ObsStore {

// Create chain of objects Group --> ObsStore_Group_Backend --> ObsStore::Group
Group createRootGroup() {
  auto backend = std::make_shared<ObsStore_Group_Backend>(ioda::ObsStore::Group::createRootGroup());
  return ::ioda::Group{backend};
}

Capabilities getCapabilities() {
  static Capabilities caps;
  static bool inited = false;
  if (!inited) {
    caps.canChunk            = Capability_Mask::Ignored;
    caps.canCompressWithGZIP = Capability_Mask::Ignored;
    caps.canCompressWithSZIP = Capability_Mask::Ignored;
    caps.MPIaware            = Capability_Mask::Unsupported;
  }

  return caps;
}

}  // namespace ObsStore
}  // namespace Engines
}  // namespace ioda
