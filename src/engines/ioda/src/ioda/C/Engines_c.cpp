/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \addtogroup ioda_engines
 * @{
 * \file Engines_c.cpp
 * \brief @link ioda_engines C bindings @endlink for ioda::Engines
 */

#include "ioda/C/Engines_c.h"

#include <iostream>
#include <stdexcept>

#include "./structs_c.h"
#include "ioda/C/c_binding_macros.h"  // C_TRY and C_CATCH_AND_TERMINATE
#include "ioda/Engines/HH.h"
#include "ioda/Engines/ObsStore.h"

extern "C" {

IODA_DL ioda_group* ioda_Engines_ObsStore_createRootGroup() {
  ioda_group* res = nullptr;
  C_TRY;
  res    = new ioda_group;
  res->g = ioda::Engines::ObsStore::createRootGroup();
  C_CATCH_RETURN_FREE(res, NULL, res);
}

IODA_DL ioda_group* ioda_Engines_HH_createMemoryFile(
  size_t sz_filename, const char* filename, bool flush_on_close,
  long increment_len_bytes  // NOLINT(google-runtime-int): HDF5 detail must be exposed.
) {
  ioda_group* res = nullptr;
  C_TRY;
  res    = new ioda_group;
  res->g = ioda::Engines::HH::createMemoryFile(
    std::string(filename, sz_filename), ioda::Engines::BackendCreateModes::Truncate_If_Exists,
    flush_on_close, (size_t)increment_len_bytes);
  C_CATCH_RETURN_FREE(res, NULL, res);
}

IODA_DL ioda_group* ioda_Engines_HH_openFile(size_t sz_filename, const char* filename,
                                             ioda_Engines_BackendOpenModes mode) {
  ioda_group* res = nullptr;
  C_TRY;
  res = new ioda_group;

  const std::map<ioda_Engines_BackendOpenModes, ioda::Engines::BackendOpenModes> m{
    {ioda_Engines_BackendOpenModes_Read_Only, ioda::Engines::BackendOpenModes::Read_Only},
    {ioda_Engines_BackendOpenModes_Read_Write, ioda::Engines::BackendOpenModes::Read_Write}};
  if (!m.count(mode)) throw;  // jedi_Unimplemented;

  res->g = ioda::Engines::HH::openFile(std::string(filename, sz_filename), m.at(mode));
  C_CATCH_RETURN_FREE(res, NULL, res);
}

IODA_DL ioda_group* ioda_Engines_HH_createFile(size_t sz_filename, const char* filename,
                                               ioda_Engines_BackendCreateModes mode) {
  ioda_group* res = nullptr;
  C_TRY;
  res = new ioda_group;

  const std::map<ioda_Engines_BackendCreateModes, ioda::Engines::BackendCreateModes> m{
    {ioda_Engines_BackendCreateModes_Truncate_If_Exists,
     ioda::Engines::BackendCreateModes::Truncate_If_Exists},
    {ioda_Engines_BackendCreateModes_Fail_If_Exists,
     ioda::Engines::BackendCreateModes::Fail_If_Exists}};
  if (!m.count(mode)) throw;  // jedi_Unimplemented;

  res->g = ioda::Engines::HH::createFile(std::string(filename, sz_filename), m.at(mode));
  C_CATCH_RETURN_FREE(res, NULL, res);
}

IODA_DL ioda_group* ioda_Engines_constructFromCmdLine(int argc, char** argv,
                                                      const char* defaultFilename) {
  ioda_group* res = nullptr;
  C_TRY;
  res    = new ioda_group;
  res->g = ioda::Engines::constructFromCmdLine(argc, argv, std::string{defaultFilename});
  C_CATCH_RETURN_FREE(res, NULL, res);
}
}

/// @}
