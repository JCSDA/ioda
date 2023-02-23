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

#include "ioda/C/ioda_group_c.hpp"
#include "ioda/C/ioda_engines_c.hpp"
#include "ioda/C/ioda_vecstring_c.hpp"

#define sslin(x) #x
#define slin(x) sslin(x)
#define doErr                                                                                      \
  {                                                                                                \
    errlin = "Error in " __FILE__ " at line " slin(__LINE__) ".\n";                                \
    goto hadError;                                                                                 \
  }

int main(int argc, char** argv) {
  int errval                     = 0;
  int r;
  int64_t sz;
  void * g1;
  void * g2;
  void * groot;
  void * g3;
  void * g4;
  void * g5;
  void * g6;
  void * open_g3;
  void * g_list;
  char * errlin = 0x0;
            
  groot = ioda_engines_c_construct_from_command_line((void*)0x0,"Example-01-C.hdf5");
  if (!groot) {
    fprintf(stderr,"root cmdline group failed\n");
    goto hadError;
  }
  g1 = ioda_group_c_create(groot,2,"g1");
  if (!g1) goto hadError;
  g2 = ioda_group_c_create(groot,2,"g2");
  if (!g2) goto hadError;
  g3 = ioda_group_c_create(g1,2,"g3");
  if (!g3) goto hadError;
  g4 = ioda_group_c_create(g3,2,"g4");
  if (!g4) goto hadError;
  g5 = ioda_group_c_create(g4,2,"g5");
  if (!g4) goto hadError;
  g6 = ioda_group_c_create(g4,2,"g6");
  if (!g4) goto hadError;
  fprintf(stderr,"step1 done\n");

  r = ioda_group_c_exists(groot,2,"g1");
     fprintf(stderr,"ioda_group_c_exists %d\n",r);
  if ( r  <= 0) {
     fprintf(stderr,"ioda_group_c_exists g1 failed!\n");
     doErr;
  }
  r = ioda_group_c_exists(groot,2,"g2");
     fprintf(stderr,"ioda_group_c_exists %d\n",r);
  if ( r  <= 0) {
     fprintf(stderr,"ioda_group_c_exists g3 failed!\n");
     doErr;
  }
  if ( ioda_group_c_exists(g1,2,"g3") <= 0) doErr;
  if ( ioda_group_c_exists(g1,2,"g3/g4") <= 0) doErr;
  fprintf(stderr,"step2 done\n");
  
  g_list = ioda_group_c_list(g3);
  if (!g_list) {
    fprintf(stderr,"fiorst ioda_group_c_list failed\n");
    doErr;
  }
  sz = ioda_vecstring_c_size(g_list); 
  fprintf(stderr,"sz of g3 list = %ld\n",sz); 
  if ( sz != 1) doErr;
  ioda_vecstring_c_dealloc(&g_list);

  g_list = ioda_group_c_list(g4);
  if (!g_list) {
    fprintf(stderr,"fiorst ioda_group_c_list failed\n");
    doErr;
  }
  sz = ioda_vecstring_c_size(g_list);
  fprintf(stderr,"sz of g1 list = %ld\n",sz); 
  if ( sz != 2) doErr;
  ioda_vecstring_c_dealloc(&g_list);

  open_g3 = ioda_group_c_open(g1,2,"g3");
  if (!open_g3) {
    fprintf(stderr,"ioda_group_c_open failed!\n");
    doErr;
  }

  fprintf(stderr,"step4 done\n");
  
  goto cleanup;

hadError:
  printf("%s", (errlin) ? errlin : "An unknown error has occurred somewhere.");
  errval = 1;
cleanup:
  ioda_group_c_dtor(&open_g3);
//  ioda_vecstring_c_dealloc(&g_list);
  ioda_group_c_dtor(&g4);
  ioda_group_c_dtor(&g3);
  ioda_group_c_dtor(&g2);
  ioda_group_c_dtor(&g1);
  ioda_group_c_dtor(&groot); 
  return errval;
}
