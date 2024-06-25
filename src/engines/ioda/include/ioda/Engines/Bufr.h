/*
* (C) Copyright 2024 NOAA/NWS/NCEP/EMC
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

#include "../defs.h"
#include "ObsStore.h"
#include "../Group.h"

namespace ioda {
namespace Engines {
namespace Bufr {

/// \brief Encapsulate the parameters to make calling simpler.
/// \ingroup ioda_cxx_engines_pub_Bufr
struct Bufr_Parameters {
  std::string filename;
  std::string mappingFile;
  std::string tablePath = "";
  std::vector<std::string> category = {};
  std::vector<std::vector<std::string>> cacheCategories = {};
};

/// \brief Import an Bufr file.
/// \ingroup ioda_cxx_engines_pub_Bufr
/// \param emptyStorageGroup is the initial (empty) group, provided
///   by another engine (ObsStore) that will be populated with the
///   Bufr data.
IODA_DL ObsGroup openFile(const Bufr_Parameters& params,
                          ioda::Group emptyStorageGroup = ObsStore::createRootGroup());
}  // namespace Bufr
}  // namespace Engines
}  // namespace ioda