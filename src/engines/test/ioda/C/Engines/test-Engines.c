/*
 * (C) Copyright 2020-2022 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/** \file test-Engines.c
 * \brief C binding tests for ioda-engines engines.
 *
 * \author Ryan Honeyager (honeyage@ucar.edu)
 **/

#include <stdio.h>
#include <string.h>

#include "ioda/C/ioda_engines_c.hpp"
#include "ioda/C/ioda_group_c_hpp"

int main() {
  int errval = 0;
  struct ioda_group *g1 = NULL, *g2 = NULL, *g3 = NULL;
  const char *n1 = "1";
  const char *n2 = "test-engines-3.hdf5";
  
/*
  g1 = ioda->Engines->ObsStore->createRootGroup();
  if (!g1) goto hadError;
  g2 = ioda->Engines->HH->createMemoryFile(1, "1", false, 100000);
  if (!g2) goto hadError;
  g3 = ioda->Engines->HH->createFile(19, "test-engines-3.hdf5", ioda_Engines_BackendCreateModes_Truncate_If_Exists);
  if (!g3) goto hadError;
*/
  g1 = ioda_engines_c_create_root_group();
  if (!g1) goto hadErr;
  g2 = ioda_engines_c_hh_create_memory_file(reinterpret_cast<void*>(n1),100000);
  if (!g1) goto hadErr;
  g3 = ioda_engines_c_createFile(reinterpret_cast<void*>(n2),1);
  if (!g3) goto hadErr;

  goto cleanup;

hadError:
  printf("An error was encountered.\n");
  errval = 1;

cleanup:
  if (g3) ioda_group_c_dtor(&g3);
  if (g2) ioda_group_c_dtor(&g2);
  if (g1) ioda_group_c_dtor(&g1);
  return errval;
}
