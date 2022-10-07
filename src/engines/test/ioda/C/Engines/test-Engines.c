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

#include "ioda/C/ioda_c.h"
#include "ioda/C/Engines_c.h"
#include "ioda/C/Group_c.h"
#include "ioda/defs.h"  // Always include this first.

int main() {
  int errval = 0;
  const struct ioda_c_interface* ioda  = get_ioda_c_interface();
  struct ioda_group *g1 = NULL, *g2 = NULL, *g3 = NULL;

  g1 = ioda->Engines->ObsStore->createRootGroup();
  if (!g1) goto hadError;
  g2 = ioda->Engines->HH->createMemoryFile(1, "1", false, 100000);
  if (!g2) goto hadError;
  g3 = ioda->Engines->HH->createFile(19, "test-engines-3.hdf5", ioda_Engines_BackendCreateModes_Truncate_If_Exists);
  if (!g3) goto hadError;

  goto cleanup;

hadError:
  printf("An error was encountered.\n");
  errval = 1;

cleanup:
  if (g3) ioda->Groups->destruct(g3);
  if (g2) ioda->Groups->destruct(g2);
  if (g1) ioda->Groups->destruct(g1);

  return errval;
}
