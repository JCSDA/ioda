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

#include "ioda/C/ioda_c.h"
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
  struct c_ioda ioda = use_c_ioda();
  struct ioda_group *grpFromFile = NULL, *g1 = NULL, *g2 = NULL, *g3 = NULL, *g4 = NULL, *g5 = NULL,
                    *g6 = NULL, *opened_g3 = NULL, *opened_g6 = NULL;
  struct ioda_string_ret_t *g3_list = NULL, *g4_list = NULL;

  grpFromFile = ioda.Engines.constructFromCmdLine(argc, argv, "Example-01b-C.hdf5");
  if (!grpFromFile) goto hadError;

  g1 = ioda.Group.create(grpFromFile, "g1");
  if (!g1) doErr;
  g2 = ioda.Group.create(grpFromFile, "g2");
  if (!g2) doErr;
  g3 = ioda.Group.create(g1, "g3");
  if (!g3) doErr;
  g4 = ioda.Group.create(g3, "g4");
  if (!g4) doErr;
  g5 = ioda.Group.create(g4, "g5");
  if (!g5) doErr;
  g6 = ioda.Group.create(g4, "g6");
  if (!g6) doErr;

  if (ioda.Group.exists(g1, "g3") <= 0) doErr;

  if (ioda.Group.exists(g1, "g3/g4") <= 0) doErr;

  g3_list = ioda.Group.list(g3);
  if (!g3_list) doErr;
  if (g3_list->n != 1) doErr;
  g4_list = ioda.Group.list(g4);
  if (!g4_list) doErr;
  if (g4_list->n != 2) doErr;

  opened_g3 = ioda.Group.open(g1, "g3");
  if (!opened_g3) doErr;
  opened_g6 = ioda.Group.open(g3, "g4/g6");
  if (!opened_g6) doErr;

  ioda_group_destruct(opened_g3);

  goto cleanup;

hadError:
  printf("%s", (errlin) ? errlin : "An unknown error has occurred somewhere.");
  errval = 1;
cleanup:
  if (opened_g6) ioda.Group.destruct(opened_g6);
  // opened_g3 was deliberately destructed earlier
  if (g4_list) ioda.Strings.destruct(g4_list);
  if (g3_list) ioda.Strings.destruct(g3_list);
  if (g6) ioda.Group.destruct(g6);
  if (g5) ioda.Group.destruct(g5);
  if (g4) ioda.Group.destruct(g4);
  if (g3) ioda.Group.destruct(g3);
  if (g2) ioda.Group.destruct(g2);
  if (g1) ioda.Group.destruct(g1);
  if (grpFromFile) ioda.Group.destruct(grpFromFile);

  return errval;
}
