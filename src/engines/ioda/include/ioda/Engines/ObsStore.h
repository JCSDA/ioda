/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// \file ObsStore.h
/// \brief Functions for creating a new top level ioda::Group backed by ObsStore
#pragma once
#include <string>

#include "../defs.h"
#include "Capabilities.h"

namespace ioda {
class Group;

namespace Engines {
namespace ObsStore {
/// \brief Create a ioda::Group backed by an OsbStore Group object.
IODA_DL Group createRootGroup();

/// Get capabilities of the ObsStore engine
IODA_DL Capabilities getCapabilities();
}  // namespace ObsStore
}  // namespace Engines
}  // namespace ioda
