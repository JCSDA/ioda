/*
 * (C) Copyright 2020-2022 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \addtogroup ioda_c_ex
 *
 * @{
 *
 * \defgroup ioda_c_ex_1 Ex 1: Groups and ObsSpaces
 * \brief Group manipulation using the C interface
 * \details This example parallels the C++ examples.
 *
 * @{
 *
 * \file 01-GroupsAndObsSpaces.c More group examples
 **/

#include <stdio.h>
#include <string.h>

#include "ioda/C/ioda_c.h"
#include "ioda/C/Engines_c.h"
#include "ioda/C/Group_c.h"
#include "ioda/C/VecString_c.h"
#include "ioda/defs.h"  // Always include this first.

#define sslin(x) #x
#define slin(x) sslin(x)
#define doErr                                                                                      \
  {                                                                                                \
    errlin = "Error in " __FILE__ " at line " slin(__LINE__) ".\n";                                \
    goto hadError;                                                                                 \
  }

int main(int argc, char** argv) {
  int errval                     = 0;
  const char* errlin             = NULL;
  const struct ioda_c_interface* ioda  = get_ioda_c_interface();
  struct ioda_group *grpFromFile = NULL, *g1 = NULL, *g2 = NULL, *g3 = NULL, *g4 = NULL, *g5 = NULL,
                    *g6 = NULL, *opened_g3 = NULL, *opened_g6 = NULL;
  struct ioda_VecString *g3_list = NULL, *g4_list = NULL;

  grpFromFile = ioda->Engines->constructFromCmdLine(argc, argv, "Example-01-C.hdf5");
  if (!grpFromFile) goto hadError;

  g1 = ioda->Groups->create(grpFromFile, 2, "g1");
  if (!g1) doErr;
  g2 = ioda->Groups->create(grpFromFile, 2, "g2");
  if (!g2) doErr;
  g3 = ioda->Groups->create(g1, 2, "g3");
  if (!g3) doErr;
  g4 = ioda->Groups->create(g3, 2, "g4");
  if (!g4) doErr;
  g5 = ioda->Groups->create(g4, 2, "g5");
  if (!g5) doErr;
  g6 = ioda->Groups->create(g4, 2, "g6");
  if (!g6) doErr;

  if (ioda->Groups->exists(g1, 2, "g3") <= 0) doErr;

  if (ioda->Groups->exists(g1, 5, "g3/g4") <= 0) doErr;

  g3_list = ioda->Groups->list(g3);
  if (!g3_list) doErr;
  if (g3_list->size(g3_list) != 1) doErr;
  g4_list = ioda->Groups->list(g4);
  if (!g4_list) doErr;
  if (ioda->VecStrings->size(g4_list) != 2) doErr;

  opened_g3 = ioda->Groups->open(g1, 2, "g3");
  if (!opened_g3) doErr;
  opened_g6 = ioda->Groups->open(g3, 5, "g4/g6");
  if (!opened_g6) doErr;

  opened_g3->destruct(opened_g3);

  goto cleanup;

hadError:
  printf("%s", (errlin) ? errlin : "An unknown error has occurred somewhere.");
  errval = 1;
cleanup:
  if (opened_g6) ioda->Groups->destruct(opened_g6);
  // opened_g3 was deliberately destructed earlier
  if (g4_list) ioda->VecStrings->destruct(g4_list);
  if (g3_list) ioda->VecStrings->destruct(g3_list);
  if (g6) ioda->Groups->destruct(g6);
  if (g5) ioda->Groups->destruct(g5);
  if (g4) ioda->Groups->destruct(g4);
  if (g3) ioda->Groups->destruct(g3);
  if (g2) ioda->Groups->destruct(g2);
  if (g1) ioda->Groups->destruct(g1);
  if (grpFromFile) ioda->Groups->destruct(grpFromFile);

  return errval;
}
