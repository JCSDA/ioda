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

int main()
{
	char * buf;
	const char* test1 = "This ";
	const char* test2 = "is a test.";
	const char* test_str = "This is a test.";
	int64_t sz;
	
	cxx_string_t s = cxx_string_c_alloc();
	
	// set and get a string
	cxx_string_c_set(s,test_str);
	buf = cxx_string_c_get(s);
         
        if (strlen(buf) != strlen(test_str) || strcmp(test_str,buf)!= 0) {
		fprintf(stderr," errpr lone %d test fsiled\n",__LINE__);
		cxx_string_c_dealloc(s);
		return -1;
        }
        printf(" %s  =? %s\n",buf,test_str);
        free(buf);
        
        cxx_string_c_clear(s);

        sz = cxx_string_c_size((void*)s);
        
        if (sz!=0) {
	    fprintf(stderr,"ioda_string_c_size or clear fun failed\n");
	    cxx_string_c_dealloc(s);
	    return -1;
        }

	cxx_string_c_set(s,test1);
	cxx_string_c_append(s,test2);
	buf = cxx_string_c_get(s);
        
        if ( strcmp(buf,test_str)!=0) {
           fprintf(stderr,"ioda_c_string_append does not work %s != %s\n",buf,test_str);
           free(buf);
           cxx_string_c_dealloc(s);
           return -1;
        }
        fprintf(stderr,"done success\n");
        cxx_string_c_dealloc(&s);
	return EXIT_SUCCESS;
}
