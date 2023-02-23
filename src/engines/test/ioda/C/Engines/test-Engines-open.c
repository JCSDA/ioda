/*
 * (C) Copyright 2020-2022 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/** \file test-Engines.c
 * \brief C binding tests for ioda-engines engines.
 *
 * \author Patrick Nichols ( based on Ryans Honeyager's version)
 **/

#include <stdio.h>
#include <string.h>
/*
#include "ioda/C/Engines_c.h"
#include "ioda/C/Group_c.h"
#include "ioda/defs.h"  // Always include this first.
*/

#include "ioda/C/ioda_group_c_.hpp"
#include "ioda/C/ioda_engines_c.hpp"

int main() {
  int errval = 0;
  ioda_group_t* g4 = NULL;

//  g4 = ioda->Engines->HH->openFile(19, "test-engines-3.hdf5", ioda_Engines_BackendOpenModes_Read_Only);

  g4 = ioda_engines_hh_open_file(reinterpret_cast<void*>(name1),1); 
  if (!g4) goto hadError;

  goto cleanup;

hadError:
  printf("An error was encountered.\n");
  errval = 1;

cleanup:
  if (g4) ioda_group_c_dtor(&g4);

  return errval;
}
