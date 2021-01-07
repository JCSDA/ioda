/*
 * (C) Copyright 2020 UCAR
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

#include "ioda/C/Engines_c.h"
#include "ioda/C/Group_c.h"
#include "ioda/defs.h"  // Always include this first.

int main() {
  int errval = 0;
  struct ioda_group *g1 = NULL, *g2 = NULL, *g3 = NULL;

  g1 = ioda_Engines_ObsStore_createRootGroup();
  if (!g1) goto hadError;
  g2 = ioda_Engines_HH_createMemoryFile("1", false, 100000);
  if (!g2) goto hadError;
  g3 = ioda_Engines_HH_createFile("test-engines-3.hdf5", ioda_Engines_BackendCreateModes_Truncate_If_Exists);
  if (!g3) goto hadError;

  goto cleanup;

hadError:
  printf("An error was encountered.\n");
  errval = 1;

cleanup:
  if (g3) ioda_group_destruct(g3);
  if (g2) ioda_group_destruct(g2);
  if (g1) ioda_group_destruct(g1);

  return errval;
}
