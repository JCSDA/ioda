/*
* (C) Copyright 2023 NOAA/NWS/NCEP/EMC
*
* This software is licensed under the terms of the Apache Licence Version 2.0
* which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
*/

#pragma once

/*! \defgroup ioda_cxx_engines_pub_Bufr Bufr Engine
 * \brief Bufr Engine
 * \ingroup ioda_cxx_engines_pub
 *
 * @{
 * \file Bufr.h
 * \brief Bufr engine
 */

#include <map>
#include <string>
#include <variant>

#include "../defs.h"
#include "ObsStore.h"
#include "../Group.h"

namespace ioda {
namespace Engines {
namespace Script {

/// \brief Encapsulate the parameters to make calling simpler.
/// \ingroup ioda_cxx_engines_pub_Bufr
struct Script_Parameters {
  typedef std::variant<std::string, int, double> ArgType;
  std::string scriptFile;
  std::map<std::string, ArgType> args;
};

/// \brief Import an Script file.
/// \ingroup ioda_cxx_engines_pub_Script
/// \param emptyStorageGroup is the initial (empty) group, provided
///   by another engine (ObsStore) that will be populated with the
///   Bufr data.
IODA_DL ObsGroup openFile(const Script_Parameters& params,
                          ioda::Group emptyStorageGroup = ObsStore::createRootGroup());

}  // namespace Script
}  // namespace Engines
}  // namespace ioda
