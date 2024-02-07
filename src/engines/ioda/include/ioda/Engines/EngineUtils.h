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
 * \file EngineUtils.h
 * \brief Definitions for setting up backends with file and memory I/O.
 */
#include <mpi.h>
#include <string>
#include <vector>

#include "../defs.h"

#include "eckit/config/LocalConfiguration.h"

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
  ODB,       ///< ODB in-memory
};

/// Actions for accessing a file
/// \ingroup ioda_cxx_engines_pub
enum class BackendFileActions {
  Undefined,      ///< Action has not be set
  Create,         ///< Create a new file - single process access
  CreateParallel, ///< Create a new file - multi-process access
  Open,           ///< Open an existing file - single process access
  OpenParallel    ///< Open an existing file - multi-process access
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
  MPI_Comm comm;
  std::size_t allocBytes;
  bool flush;
  /// @}

  BackendCreationParameters() { }
};

/// \brief uniquify the output file name
/// \details This function will tag on the MPI task number to the end of the file name
/// to avoid collisions when running with multiple MPI tasks.
/// \param fileName raw output file name
/// \param createMultipleFiles if true need to append rankNum to the file name
/// \param rankNum MPI group communicator rank number
/// \param timeRankNum MPI time communicator rank number
std::string uniquifyFileName(const std::string & fileName, const bool createMultipleFiles,
                             const std::size_t rankNum, const int timeRankNum);

/// \brief store generated data into an ObsGroup
/// \param latVals vector of latitude values
/// \param lonVals vector of longitude values
/// \param vcoordType string specifying type of vertical coordinate to use
/// \param vcoordVals vector of vertical coordinate values
/// \param dts vector of time offsets (s) relative to \p epoch
/// \param epoch (ISO 8601 string) relative to which datetimes are computed
/// \param obsVarNames vector (string) of simulated variable names
/// \param obsValues vector of observed values
/// \param obsErrors vector of obs error estimates
/// \param[out] obsGroup destination for the generated data
void storeGenData(const std::vector<float> & latVals,
                  const std::vector<float> & lonVals,
                  const std::string & vcoordType,
                  const std::vector<float> & vcoordVals,
                  const std::vector<int64_t> & dts,
                  const std::string & epoch,
                  const std::vector<std::string> & obsVarNames,
                  const std::vector<float> & obsValues,
                  const std::vector<float> & obsErrors,
                  ObsGroup &obsGroup);

/// \brief This is a wrapper function around the constructBackend
///   function for creating a backend based on command-line options.
///   Intended for unit testing only.
/// \ingroup ioda_cxx_engines_pub
IODA_DL Group constructFromCmdLine(int argc, char** argv, const std::string& defaultFilename);

/// \brief create an eckit local configuration containing proper engine parameters
/// \details This function creates a YAML engines configuration that is suitable for
/// use with the EngineFactory functions. The purpose of this is to provide a utility
/// that can be used to create a backend through the same process that the ioda reader
/// and writer use. Placing this utility in the examples and tests will be more
/// instructive on the proper way to use the Engine file io backends.
/// \param fileType type of file, currently accepts "hdf5" or "odb".
/// \param fileName name, including path, of file to read or write.
/// \param mapFileName yaml containing variable number/name mappings (only for odb file)
/// \param queryFileName yaml containing which varibles to use (only for odb file)
/// \ingroup ioda_cxx_engines_pub
IODA_DL eckit::LocalConfiguration constructFileBackendConfig(const std::string & fileType,
                const std::string & fileName, const std::string & mapFileName = "",
                const std::string & queryFileName = "", const std::string & odbType = "");

/// \brief This is a simple factory style function that will instantiate a
///   different backend based on a given name an parameters.
/// \ingroup ioda_cxx_engines_pub
IODA_DL Group constructBackend(BackendNames name, BackendCreationParameters& params);

/// \brief check to see if a file can be opened for reading
/// \param fileName path to input file name being tested
IODA_DL bool openInputFileCheck(const std::string & fileName);

/// stream operator
IODA_DL std::ostream& operator<<(std::ostream& os, const BackendCreateModes& mode);
/// stream operator
IODA_DL std::ostream& operator<<(std::ostream& os, const BackendOpenModes& mode);

}  // namespace Engines
}  // namespace ioda

/// @}


