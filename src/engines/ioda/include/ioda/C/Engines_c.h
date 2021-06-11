#pragma once
/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \defgroup ioda_engines Engines
 * \brief Provides the C-style interface for the ioda::Engines namespace.
 * \ingroup ioda_c_api
 *
 * @{
 * \file Engines_c.h
 * \brief @link ioda_engines C bindings @endlink for ioda::Engines
 */
#include <stdbool.h>

#include "../defs.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ioda_group;

/// \brief Options when opening a file.
enum ioda_Engines_BackendOpenModes {
  /// Open in read-only mode.
  ioda_Engines_BackendOpenModes_Read_Only,
  /// Open in read-write mode.
  ioda_Engines_BackendOpenModes_Read_Write
};
/// \brief Options when creating a file.
enum ioda_Engines_BackendCreateModes {
  /// Create a new file. If a file already exists, overwrite it.
  ioda_Engines_BackendCreateModes_Truncate_If_Exists,
  /// Create a new file. If a file already exists at the path, fail.
  ioda_Engines_BackendCreateModes_Fail_If_Exists
};

/// \brief Create a new ObsStore instance.
/// \todo Document the ObsStore engine.
/// \return The new instance, encapsulated as a ioda_group.
/// \see ioda::Engines::ObsStore::createRootGroup.
IODA_DL struct ioda_group* ioda_Engines_ObsStore_createRootGroup();

/// \brief Create a new in-memory data store, backed by HDF5.
/// \param filename is an identifier to the "file" that HDF5 is accessing. Multiple opens of the
///   same identifier open the same object.
/// \param sz_filename is strlen(filename). Needed by Fortran bindings.
/// \param flush_on_close denotes whether the in-memory object should be flushed (written)
///   to disk once it is closed. Useful for debugging. If true, then file "filename" will
///   be created on success.
/// \param increment_len_bytes represents the size of new memory allocations that occur when
///   data is written to the in-memory storage. Basically, when the engine needs more memory,
///   allocate additional blocks with this size.
/// \return The new instance, encapsulated as a ioda_group.
/// \todo Clarify whether flush_on_close truncates a file or fails if it exists. Add an option?
/// \see ioda::Engines::HH::createMemoryFile.
IODA_DL struct ioda_group* ioda_Engines_HH_createMemoryFile(
  size_t sz_filename, const char* filename, bool flush_on_close,
  long increment_len_bytes);  // NOLINT: cppcheck complains about long

/// \brief Open a handle to a file that is backed by HDF5.
/// \param filename is the path to the file.
/// \param sz_filename is strlen(filename). Needed by Fortran bindings.
/// \param mode is the access mode (read or read/write).
/// \return The file, encapsulated as a ioda_group.
IODA_DL struct ioda_group* ioda_Engines_HH_openFile(size_t sz_filename, const char* filename,
                                                    enum ioda_Engines_BackendOpenModes mode);
/// \brief Create a new file using the HDF5 interface.
/// \param filename is the path to the file.
/// \param sz_filename is strlen(filename). Needed by Fortran bindings.
/// \param mode is the access mode. Essentially, is the file created if a file with the same
/// name already exists?
IODA_DL struct ioda_group* ioda_Engines_HH_createFile(size_t sz_filename, const char* filename,
                                                      enum ioda_Engines_BackendCreateModes mode);

/// \brief Function used in the ioda C examples and unit tests to construct different
///   backends based on different command-line parameters.
/// \param argc is the number of command-line arguments.
/// \param argv is the command-line arguments.
/// \param defaultFilename is a default file to be used in case no command-line arguments are
///   specified.
IODA_DL struct ioda_group* ioda_Engines_constructFromCmdLine(int argc, char** argv,
                                                             const char* defaultFilename);

/// Class-like encapsulation of ioda::Engines::ObsStore functions.
struct c_ioda_engines_ObsStore {
  struct ioda_group* (*createRootGroup)();
};
/// Class-like encapsulation of ioda::Engines::HH functions.
struct c_ioda_engines_HH {
  struct ioda_group* (*createMemoryFile)(size_t, const char*, bool,
                                         long);  // NOLINT: cppcheck complains about long
  struct ioda_group* (*openFile)(size_t, const char*, enum ioda_Engines_BackendOpenModes);
  struct ioda_group* (*createFile)(size_t, const char*, enum ioda_Engines_BackendCreateModes);
};
/// Class-like encapsulation of ioda::Engines functions.
struct c_ioda_engines {
  struct ioda_group* (*constructFromCmdLine)(int, char**, const char*);

  struct c_ioda_engines_HH HH;
  struct c_ioda_engines_ObsStore ObsStore;
};

#ifdef __cplusplus
}
#endif

/// @} // End Doxygen block
