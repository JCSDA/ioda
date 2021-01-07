#pragma once
/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// \file Engines_c.h
/// \brief C bindings for ioda::Engines
#include <stdbool.h>

#include "../defs.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ioda_group;
enum ioda_Engines_BackendOpenModes {
  ioda_Engines_BackendOpenModes_Read_Only,
  ioda_Engines_BackendOpenModes_Read_Write
};
enum ioda_Engines_BackendCreateModes {
  ioda_Engines_BackendCreateModes_Truncate_If_Exists,
  ioda_Engines_BackendCreateModes_Fail_If_Exists
};

IODA_DL struct ioda_group* ioda_Engines_ObsStore_createRootGroup();
IODA_DL struct ioda_group* ioda_Engines_HH_createMemoryFile(
  const char* filename, bool flush_on_close,
  long increment_len_bytes);  // NOLINT: cppcheck complains about long
IODA_DL struct ioda_group* ioda_Engines_HH_openFile(const char* filename,
                                                    enum ioda_Engines_BackendOpenModes mode);
IODA_DL struct ioda_group* ioda_Engines_HH_createFile(const char* filename,
                                                      enum ioda_Engines_BackendCreateModes mode);

IODA_DL struct ioda_group* ioda_Engines_constructFromCmdLine(int argc, char** argv,
                                                             const char* defaultFilename);

struct c_ioda_engines_ObsStore {
  struct ioda_group* (*createRootGroup)();
};
struct c_ioda_engines_HH {
  struct ioda_group* (*createMemoryFile)(const char*, bool, long);  // NOLINT: cppcheck complains about long
  struct ioda_group* (*openFile)(const char*, enum ioda_Engines_BackendOpenModes);
  struct ioda_group* (*createFile)(const char*, enum ioda_Engines_BackendCreateModes);
};
struct c_ioda_engines {
  struct ioda_group* (*constructFromCmdLine)(int, char**, const char*);

  struct c_ioda_engines_HH HH;
  struct c_ioda_engines_ObsStore ObsStore;
};

#ifdef __cplusplus
}
#endif
