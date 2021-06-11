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
#include <string>

#include "../defs.h"

namespace ioda {
class Group;

/// The backends that implement the ioda-engines functionality.
namespace Engines {
/// \brief Backend names
/// \ingroup ioda_cxx_engines_pub
enum class BackendNames {
  Hdf5File,  ///< HDF5 file access
  Hdf5Mem,   ///< HDF5 in-memory "file"
  ObsStore   ///< ObsStore in-memory
};

/// Actions for accessing a file
/// \ingroup ioda_cxx_engines_pub
enum class BackendFileActions {
  Create,  ///< Create a new file
  Open     ///< Open an existing file
};

/// Options when creating a new file.
/// \ingroup ioda_cxx_engines_pub
enum class BackendCreateModes {
  Truncate_If_Exists,  ///< If the file already exists, overwrite it.
  Fail_If_Exists       ///< If the file already exists, fail with an error.
};

/// Options when opening an file that already exists.
/// \ingroup ioda_cxx_engines_pub
enum class BackendOpenModes {
  Read_Only,  ///< Open the file in read-only mode.
  Read_Write  ///< Open the file in read-write mode.
};

/// \brief Used to specify backend creation-time properties
/// \ingroup ioda_cxx_engines_pub
struct BackendCreationParameters {
public:
  std::string fileName;
  BackendFileActions action;
  BackendCreateModes createMode;
  BackendOpenModes openMode;
  std::size_t allocBytes;
  bool flush;
};

/// \brief This is a wrapper function around the constructBackend
///   function for creating a backend based on command-line options.
///   Intended for unit testing only.
/// \ingroup ioda_cxx_engines_pub
IODA_DL Group constructFromCmdLine(int argc, char** argv, const std::string& defaultFilename);

/// \brief This is a simple factory style function that will instantiate a
///   different backend based on a given name an parameters.
/// \ingroup ioda_cxx_engines_pub
IODA_DL Group constructBackend(BackendNames name, BackendCreationParameters& params);
}  // namespace Engines
}  // namespace ioda

/// @}
