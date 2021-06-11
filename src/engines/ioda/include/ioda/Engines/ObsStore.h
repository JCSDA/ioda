/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \defgroup ioda_cxx_engines_pub_ObsStore ObsStore Engine
 * \brief ObsStore Engine
 * \ingroup ioda_cxx_engines_pub
 *
 * @{
 * \file ObsStore.h
 * \brief ObsStore engine
 */
#pragma once
#include <string>

#include "../defs.h"
#include "Capabilities.h"

namespace ioda {
class Group;

namespace Engines {
namespace ObsStore {
/// \brief Create a ioda::Group backed by an OsbStore Group object.
/// \ingroup ioda_cxx_engines_pub_ObsStore
IODA_DL Group createRootGroup();

/// \brief Get capabilities of the ObsStore engine
/// \ingroup ioda_cxx_engines_pub_ObsStore
IODA_DL Capabilities getCapabilities();
}  // namespace ObsStore
}  // namespace Engines
}  // namespace ioda

/// @}

/*! \defgroup ioda_cxx_engines_pub_ObsStore ObsStore Engine
 * \brief ObsStore Engine
 * \ingroup ioda_cxx_engines_pub
 */
