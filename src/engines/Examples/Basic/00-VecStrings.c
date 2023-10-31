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
#include "ioda/C/ioda_decls.hpp"
#include "ioda/C/cxx_string.hpp"
#include "ioda/C/cxx_vector_string.hpp"

int main()
{
    const char* errlin             = NULL;
    char * buf;
    int64_t sz;
    const char* test_str1 = "This is a test.";
    const char* test_str2 = "This is another test.";
      
    cxx_vector_string_t  v2 = 0x0;

    cxx_vector_string_t v = cxx_vector_string_c_alloc();

    if (!v) {
      fprintf(stderr,"error on line %d\n",__LINE__);
      goto failure;
    }

    // Resize the vector
    cxx_vector_string_c_resize(v,2);

    // Check size of vector
    if (cxx_vector_string_c_size(v) != 2) {
      fprintf(stderr,"error on line %d\n",__LINE__);
      goto failure;
    }

    // Set the vector to the strings
    cxx_vector_string_c_set_f(v,1L,test_str1);
    if (cxx_vector_string_c_size(v) != 2) {
      fprintf(stderr,"error on line %d\n",__LINE__);
      goto failure;
    }

    cxx_vector_string_c_set_f(v,2L,test_str2);
    if (cxx_vector_string_c_size(v) != 2) {
      fprintf(stderr,"error on line %d\n",__LINE__);
      goto failure;
    }

    buf = cxx_vector_string_c_get_f(v,1L);
    if (strlen(buf)!=strlen(test_str1) || strcmp(buf,test_str1)!=0) {
      fprintf(stderr,"error on line %d\n",__LINE__);
      goto failure;
    }
    if (strlen(buf)!=cxx_vector_string_c_element_size_f(v,1)) {
      fprintf(stderr,"error on line %d\n",__LINE__);
      goto failure;
    }
    free(buf);    
    fprintf(stderr,"get_f 1 succeded\n");

    buf = cxx_vector_string_c_get_f(v,2L);
    if (strlen(buf)!=strlen(test_str2)) {
      fprintf(stderr,"error on line %d\n",__LINE__);
      goto failure;    
    }
    if ( strcmp(buf,test_str2)!=0) {
      fprintf(stderr,"error on line %d\n",__LINE__);
      goto failure;    
    }
    if (strlen(buf)!=cxx_vector_string_c_element_size_f(v,2)) {
       fprintf(stderr,"error on line %d\n",__LINE__);
       goto failure;   
    }
    if (strlen(buf)!=cxx_vector_string_c_element_size_f(v,2)) {
      fprintf(stderr,"error on line %d\n",__LINE__);
      goto failure;    
    }   
    free(buf);    

    sz = cxx_vector_string_c_size(v);
    if (sz!=2) {
      fprintf(stderr,"error on line %d\n",__LINE__);
      goto failure;
    }

    sz = cxx_vector_string_c_element_size_f(v,1);
    if (sz != strlen(test_str1)) {
      fprintf(stderr,"error on line %d\n",__LINE__);
      goto failure;
    }

    cxx_vector_string_c_clear(v);
    if ( cxx_vector_string_c_size(v) != 0) {
      fprintf(stderr,"error on line %d\n",__LINE__);
      goto failure;
    }

    cxx_vector_string_c_copy(&v2,v);
    buf = cxx_vector_string_c_get_f(v2,1);
    if (strlen(buf)!=strlen(test_str1) || strcmp(buf,test_str1)!=0) {
      fprintf(stderr,"error on line %d\n",__LINE__);
      goto failure;
    }
    if (strlen(buf)!=cxx_vector_string_c_element_size_f(v2,1)) {
      fprintf(stderr,"error on line %d\n",__LINE__);
      goto failure;
    }
    free(buf);    

    buf = cxx_vector_string_c_get_f(v2,2);
    if (strlen(buf)!=strlen(test_str1) || strcmp(buf,test_str2)!=0) {
      fprintf(stderr,"error on line %d\n",__LINE__);
      goto failure;        
    }
    if (strlen(buf)!=cxx_vector_string_c_element_size_f(v2,2)) {
      fprintf(stderr,"error on line %d\n",__LINE__);
      goto failure;
    }    
    free(buf);    
    
    sz = cxx_vector_string_c_size(v);
    if (sz!=2) {
      fprintf(stderr,"error on line %d\n",__LINE__);
      goto failure;
    }
    cxx_vector_string_c_clear(v);
    if ( cxx_vector_string_c_size(v) != 0) {
      fprintf(stderr,"error on line %d\n",__LINE__);
      goto failure;
    }

    fprintf(stderr,"done success\n");
    if (v) cxx_vector_string_c_dealloc(&v);
    return EXIT_SUCCESS;

failure:
    fprintf(stderr,"done failed\n");
    if (v) cxx_vector_string_c_dealloc(&v);
    printf("%s", (errlin) ? errlin : "An unknown error has occurred somewhere.");
    return EXIT_FAILURE;
}
