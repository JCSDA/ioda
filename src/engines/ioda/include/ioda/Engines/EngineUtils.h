#pragma once
/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \defgroup ioda_cxx_engines_pub Engines
 * \brief Public API for engines
 * \ingroup ioda_cxx_api
 *
 * @{
 * \file Factory.h
 * \brief Definitions for setting up backends with file and memory I/O.
 */
#include <iostream>
#include <string>
#include <vector>

#include "../defs.h"

#include "oops/util/parameters/ParameterTraits.h"

namespace ioda {
class Group;
class ObsGroup;

/// The backends that implement the ioda-engines functionality.
namespace Engines {
/// \brief Backend names
/// \ingroup ioda_cxx_engines_pub
enum class BackendNames {
  Hdf5File,  ///< HDF5 file access
  Hdf5Mem,   ///< HDF5 in-memory "file"
  ObsStore,  ///< ObsStore in-memory
};

/// Actions for accessing a file
/// \ingroup ioda_cxx_engines_pub
enum class BackendFileActions {
  Undefined,  ///< Action has not be set
  Create,     ///< Create a new file
  Open        ///< Open an existing file
};

/// Options when creating a new file.
/// \ingroup ioda_cxx_engines_pub
/// \note When changing, you need to update the ostream operators.
enum class BackendCreateModes {
  Undefined,           ///< Mode has not be set
  Truncate_If_Exists,  ///< If the file already exists, overwrite it.
  Fail_If_Exists       ///< If the file already exists, fail with an error.
};

/// Options when opening an file that already exists.
/// \ingroup ioda_cxx_engines_pub
/// \note When changing, you need to update the ostream operators.
enum class BackendOpenModes {
  Undefined,           ///< Mode has not be set
  Read_Only,  ///< Open the file in read-only mode.
  Read_Write  ///< Open the file in read-write mode.
};

/// \brief Used to specify backend creation-time properties
/// \ingroup ioda_cxx_engines_pub
struct BackendCreationParameters {
public:
  /// @name General
  /// @{
  std::string fileName;
  BackendFileActions action;
  BackendCreateModes createMode;
  BackendOpenModes openMode;
  /// @}
  /// @name HH / HDF5
  /// @{
  std::size_t allocBytes;
  bool flush;
  /// @}
};

/// \brief store generated data into an ObsGroup
/// \param latVals vector of latitude values
/// \param lonVals vector of longitude values
/// \param dts vector of time offsets (s) relative to \p epoch
/// \param epoch (ISO 8601 string) relative to which datetimes are computed
/// \param obsVarNames vector (string) of simulated variable names
/// \param obsErrors vector of obs error estimates
/// \param[out] obsGroup destination for the generated data
void storeGenData(const std::vector<float> & latVals,
                  const std::vector<float> & lonVals,
                  const std::vector<int64_t> & dts,
                  const std::string & epoch,
                  const std::vector<std::string> & obsVarNames,
                  const std::vector<float> & obsErrors,
                  ObsGroup &obsGroup);

/// \brief This is a wrapper function around the constructBackend
///   function for creating a backend based on command-line options.
///   Intended for unit testing only.
/// \ingroup ioda_cxx_engines_pub
IODA_DL Group constructFromCmdLine(int argc, char** argv, const std::string& defaultFilename);

/// \brief This is a simple factory style function that will instantiate a
///   different backend based on a given name an parameters.
/// \ingroup ioda_cxx_engines_pub
IODA_DL Group constructBackend(BackendNames name, BackendCreationParameters& params);

/// stream operator
IODA_DL std::ostream& operator<<(std::ostream& os, const BackendCreateModes& mode);
/// stream operator
IODA_DL std::ostream& operator<<(std::ostream& os, const BackendOpenModes& mode);

}  // namespace Engines
}  // namespace ioda

/// @}


