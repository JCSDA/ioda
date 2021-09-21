#pragma once
/*
 * (C) Copyright 2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \defgroup ioda_cxx_engines_pub_ODC ODB / ODC Engine
 * \brief ODB / ODC Engine
 * \ingroup ioda_cxx_engines_pub
 *
 * @{
 * \file ODC.h
 * \brief ODB / ODC engine
 */

#include "../defs.h"
#include "ObsStore.h"
#include "../Group.h"

namespace ioda {
class Group;
class ObsGroup;

namespace Engines {
/// \brief Functions that are helpful in creating a new ioda::Group that is imported from an ODB database.
namespace ODC {

/// \brief Encapsulate the parameters to make calling simpler.
/// \todo Add a conversion function from the oops::Parameters classes.
/// \ingroup ioda_cxx_engines_pub_ODC
struct ODC_Parameters {
  /// \brief The name of the database "file" to open.
  std::string filename;
  std::string mappingFile;
  std::string queryFile;
};

/// \brief Import an ODB file.
/// \ingroup ioda_cxx_engines_pub_ODC
/// \param emptyStorageGroup is the initial (empty) group, provided
///   by another engine (ObsStore) that will be populated with the
///   ODC data.
 IODA_DL ObsGroup openFile(const ODC_Parameters& params,
   Group emptyStorageGroup = ioda::Engines::ObsStore::createRootGroup());

}  // namespace ODC
}  // namespace Engines
}  // namespace ioda

/// @}
