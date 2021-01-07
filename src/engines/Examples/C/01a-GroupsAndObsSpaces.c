/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/** \dir C
 * \brief C ioda-engines usage examples
 **/
/** \file 01-GroupsAndObsSpaces.c
 * \brief Group manipulation using the C interface
 *
 * This example parallels the C++ examples.
 * \see 01-GroupsAndObsSpaces.cpp for comments and the walkthrough.
 *
 * \author Ryan Honeyager (honeyage@ucar.edu)
 **/

#include <stdio.h>
#include <string.h>

#include "ioda/C/Engines_c.h"
#include "ioda/C/Group_c.h"
#include "ioda/C/String_c.h"
#include "ioda/defs.h"  // Always include this first.

#define sslin(x) #x
#define slin(x) sslin(x)
#define doErr                                                       \
  {                                                                 \
    errlin = "Error in " __FILE__ " at line " slin(__LINE__) ".\n"; \
    goto hadError;                                                  \
  }

int main(int argc, char** argv) {
  int errval = 0;
  const char* errlin = NULL;
  struct ioda_group *grpFromFile = NULL, *g1 = NULL, *g2 = NULL, *g3 = NULL, *g4 = NULL, *g5 = NULL,
                    *g6 = NULL, *opened_g3 = NULL, *opened_g6 = NULL;
  struct ioda_string_ret_t *g3_list = NULL, *g4_list = NULL;

  grpFromFile = ioda_Engines_constructFromCmdLine(argc, argv, "Example-01a-C.hdf5");
  if (!grpFromFile) doErr;

  g1 = ioda_group_create(grpFromFile, "g1");
  if (!g1) doErr;
  g2 = ioda_group_create(grpFromFile, "g2");
  if (!g2) doErr;
  g3 = ioda_group_create(g1, "g3");
  if (!g3) doErr;
  g4 = ioda_group_create(g3, "g4");
  if (!g4) doErr;
  g5 = ioda_group_create(g4, "g5");
  if (!g5) doErr;
  g6 = ioda_group_create(g4, "g6");
  if (!g6) doErr;

  if (ioda_group_exists(g1, "g3") <= 0) doErr;

  if (ioda_group_exists(g1, "g3/g4") <= 0) doErr;

  g3_list = ioda_group_list(g3);
  if (!g3_list) doErr;
  if (g3_list->n != 1) doErr;
  g4_list = ioda_group_list(g4);
  if (!g4_list) doErr;
  if (g4_list->n != 2) doErr;

  opened_g3 = ioda_group_open(g1, "g3");
  if (!opened_g3) doErr;
  opened_g6 = ioda_group_open(g3, "g4/g6");
  if (!opened_g6) doErr;

  ioda_group_destruct(opened_g3);

  goto cleanup;

hadError:
  printf("%s", (errlin) ? errlin : "An unknown error has occurred somewhere.");
  errval = 1;

cleanup:
  if (opened_g6) ioda_group_destruct(opened_g6);
  // opened_g3 was deliberately destructed earlier
  if (g4_list) ioda_string_ret_t_destruct(g4_list);
  if (g3_list) ioda_string_ret_t_destruct(g3_list);
  if (g6) ioda_group_destruct(g6);
  if (g5) ioda_group_destruct(g5);
  if (g4) ioda_group_destruct(g4);
  if (g3) ioda_group_destruct(g3);
  if (g2) ioda_group_destruct(g2);
  if (g1) ioda_group_destruct(g1);
  if (grpFromFile) ioda_group_destruct(grpFromFile);

  return errval;
}
