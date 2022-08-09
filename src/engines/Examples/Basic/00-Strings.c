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
#include "ioda/C/ioda_c.h"
#include "ioda/C/String_c.h"

int main()
{
	size_t ret = 0;
	char buf[50];
	const size_t buf_sz = sizeof(buf);
	memset(buf, 0, buf_sz);

	const char* test_str = "This is a test.";

	const struct ioda_c_interface* ioda  = get_ioda_c_interface();

	// Set and get a string.
	struct ioda_string* s = ioda->Strings->construct();
	if (!s->set(s, test_str, strlen(test_str))) goto failure;
	if ((ret = s->get(s, buf, buf_sz)) != strlen(test_str)) goto failure;
	printf("%s\n", buf);

	// Test truncation when getting.
	if ((ret = s->get(s,buf,5)) != 5) goto failure;
	// buf would be "This"
	if (strlen(buf) != 4) goto failure;

	// Test string copy
	struct ioda_string *s_copy = s->copy(s);
	if (s_copy->size(s) != s->size(s)) goto failure;

	// Clear a string and check length.
	s->clear(s);
	if (s->length(s) != 0) goto failure;

	return 0;

failure:
	printf("Test failed. Buf is \"%s\".\n", buf);
	return 1;
}
