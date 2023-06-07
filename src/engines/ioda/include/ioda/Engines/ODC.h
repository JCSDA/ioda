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

static constexpr char* metadata_prefix = const_cast<char *>("MetaData/");
static constexpr int metadata_prefix_size = 9;
static constexpr char* obsvalue_prefix = const_cast<char *>("ObsValue/");
static constexpr int obsvalue_prefix_size = 9;
static constexpr char* derived_obsvalue_prefix = const_cast<char *>("DerivedObsValue/");
static constexpr int derived_obsvalue_prefix_size = 16;
static constexpr char* effective_error_prefix = const_cast<char *>("EffectiveError/");
static constexpr int effective_error_prefix_size = 15;
static constexpr char* obserror_prefix = const_cast<char *>("ObsError/");
static constexpr int obserror_prefix_size = 9;
static constexpr char* derived_obserror_prefix = const_cast<char *>("DerivedObsError/");
static constexpr int derived_obserror_prefix_size = 9;
static constexpr char* qc_prefix = const_cast<char *>("EffectiveQC/");
static constexpr int qc_prefix_size = 12;
static constexpr char* hofx_prefix = const_cast<char *>("hofx/");
static constexpr int hofx_prefix_size = 5;
static constexpr char* obsbias_prefix = const_cast<char *>("ObsBias/");
static constexpr int obsbias_prefix_size = 8;
static constexpr char* pge_prefix = const_cast<char *>("GrossErrorProbability/");
static constexpr int pge_prefix_size = 22;

/// \brief Encapsulate the parameters to make calling simpler.
/// \todo Add a conversion function from the oops::Parameters classes.
/// \ingroup ioda_cxx_engines_pub_ODC
struct ODC_Parameters {
  /// \brief The name of the database "file" to open.
  std::string filename;
  std::string mappingFile;
  std::string queryFile;
  std::string outputFile;
  int maxNumberChannels = 0;
  bool missingObsSpaceVariableAbort;
  util::DateTime timeWindowStart;
  util::DateTime timeWindowExtendedLowerBound;
};

/// \brief Import an ODB file.
/// \ingroup ioda_cxx_engines_pub_ODC
/// \param emptyStorageGroup is the initial (empty) group, provided
///   by another engine (ObsStore) that will be populated with the
///   ODC data.
 IODA_DL ObsGroup openFile(const ODC_Parameters& params,
   Group emptyStorageGroup = ioda::Engines::ObsStore::createRootGroup());

 IODA_DL Group createFile(const ODC_Parameters& params, Group emptyStorageGroup = ioda::Engines::ObsStore::createRootGroup());

}  // namespace ODC
}  // namespace Engines
}  // namespace ioda

/// @}
