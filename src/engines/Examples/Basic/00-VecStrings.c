/*
 * (C) Copyright 2020-2022 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/** \file 00-Strings.c
 *  \brief Demonstrates ioda's C string functions.
 **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ioda/C/ioda_vecstring_c.hpp"

#define sslin(x) #x
#define slin(x) sslin(x)
#define doErr                                                                                      \
  {                                                                                                \
    errlin = "Error in " __FILE__ " at line " slin(__LINE__) ".\n";                                \
    goto failure;                                                                                  \
  }

int main()
{

    int errval                     = 0;
    const char* errlin             = NULL;
    size_t ret = 0;
    char * buf;
    int64_t sz;
    const char* test_str1 = "This is a test.";
    const char* test_str2 = "This is another test.";
      
    void *v2 = 0x0;

    void * v = ioda_vecstring_c_alloc();
    
    if (!v) doErr;

    // Resize the vector
    ioda_vecstring_c_resize(v,2);
    
    // Check size of vector
    if (ioda_vecstring_c_size(v) != 2) doErr;

    // Set the vector to the strings
    ioda_vecstring_c_set_f(v,1,(void*)test_str1);
    ioda_vecstring_c_set_f(v,2,(void*)test_str2);
    
    buf = ioda_vecstring_c_get_f(v,1);
    if (strlen(buf)!=strlen(test_str1) || strcmp(buf,test_str1)!=0) doErr;
    if (strlen(buf)!=ioda_vecstring_c_element_size_f(v,1)) doErr;
    free(buf);    

    buf = ioda_vecstring_c_get_f(v,2);
    if (strlen(buf)!=strlen(test_str1) || strcmp(buf,test_str2)!=0) doErr;
    if (strlen(buf)!=ioda_vecstring_c_element_size_f(v,2)) doErr;    
    free(buf);    
    
    sz = ioda_vecstring_c_size(v);
    if (sz!=2) doErr;

    sz = ioda_vecstring_c_element_size_f(v,1); 
    if (sz != strlen(test_str1)) doErr;
    
    ioda_vecstring_c_clear(v);
    if ( ioda_vecstring_c_size(v) != 0) doErr;
         
    ioda_vecstring_c_copy(&v2,v);
    buf = ioda_vecstring_c_get_f(v2,1);
    if (strlen(buf)!=strlen(test_str1) || strcmp(buf,test_str1)!=0) doErr;
    if (strlen(buf)!=ioda_vecstring_c_element_size_f(v2,1)) doErr;
    free(buf);    

    buf = ioda_vecstring_c_get_f(v2,2);
    if (strlen(buf)!=strlen(test_str1) || strcmp(buf,test_str2)!=0) doErr;
    if (strlen(buf)!=ioda_vecstring_c_element_size_f(v2,2)) doErr;    
    free(buf);    
    
    sz = ioda_vecstring_c_size(v);
    if (sz!=2) doErr;
    
    ioda_vecstring_c_clear(v);
    if ( ioda_vecstring_c_size(v) != 0) doErr;
    

    goto cleanup;

failure:
  printf("%s", (errlin) ? errlin : "An unknown error has occurred somewhere.");
  errval = 1;

cleanup:
  if (v) ioda_vecstring_c_dealloc(&v);
  return EXIT_SUCCESS;
}
