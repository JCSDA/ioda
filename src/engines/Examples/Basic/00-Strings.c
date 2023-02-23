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

int main()
{
	size_t ret = 0;

	char * buf;
//	const size_t buf_sz = sizeof(buf);
//	memset(buf, 0, buf_sz);
	const char* test1 = "This ";
	const char* test2 = "is a test.";
	const char* test_str = "This is a test.";
	char * rstr;
	int64_t sz;
	
	void * s = ioda_string_c_alloc();
	
	// set and get a string
	ioda_string_c_set(s,(void*)test_str);
	buf = ioda_string_c_get(s);
         
        if (strlen(buf) != strlen(test_str) || strcmp(test_str,buf)!= 0) goto failure;
        printf(" %s  =? %s\n",buf,test_str);
        free(buf);
        
        ioda_string_c_clear(s);

        sz = ioda_string_c_size((void*)s);
        
        if (sz!=0) {
	    fprintf(stderr,"ioda_string_c_size or clear fun failed\n");
	    goto failure;        
        }

	ioda_string_c_set(s,(void*)test1);
	ioda_string_c_append(s,(void*)test2);
	buf = ioda_string_c_get(s);
        
        if ( strcmp(buf,test_str)!=0) {
           fprintf(stderr,"ioda_c_string_append does not work %s != %s\n",buf,test_str);
           free(buf);
           goto failure; 		
        } 
        ioda_string_c_dealloc(s);
	return 0;

failure:
	ioda_string_c_dealloc(s);
	printf("Test failed. Buf is \"%s\".\n", buf);
	return 1;
}
