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
#include "ioda/C/VecString_c.h"

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
    char buf[50];
    const size_t buf_sz = sizeof(buf);
    memset(buf, 0, buf_sz);

    const char* test_str1 = "This is a test.";
    const char* test_str2 = "This is another test.";

    const struct ioda_c_interface* ioda  = get_ioda_c_interface();

    struct ioda_VecString *v = NULL, *v2 = NULL;
    
    // Make a VecString
    v = ioda->VecStrings->construct();
    if (!v) doErr;

    // Resize the vector
    ioda->VecStrings->resize(v, 2);

    // Check size of vector
    if (ioda->VecStrings->size(v) != 2) doErr;

    // Set the vector to the strings
    if (!ioda->VecStrings->setFromCharArray(v, 0, test_str1, strlen(test_str1))) doErr;
    if (!ioda->VecStrings->setFromCharArray(v, 1, test_str2, strlen(test_str2))) doErr;

    // Check the size of a string in the vector
    if (ioda->VecStrings->elementSize(v,0) != strlen(test_str1)) doErr;

    // Read strings from the vector and check that they are correct
    if (!ioda->VecStrings->getAsCharArray(v, 0, buf, buf_sz)) doErr;
    if (strlen(test_str1) != strlen(buf)) doErr;
    if (strcmp(test_str1, buf)) doErr;

    if (!ioda->VecStrings->getAsCharArray(v, 1, buf, buf_sz)) doErr;
    if (strlen(test_str2) != strlen(buf)) doErr;
    if (strcmp(test_str2, buf)) doErr;

    // Copy a VecString
    v2 = ioda->VecStrings->copy(v);
    if (!v2) doErr;

    // Verify that v2 has the same number of elements as v.
    if (ioda->VecStrings->size(v) != ioda->VecStrings->size(v2)) doErr;

    // Clear v and verify that its size is zero.
    ioda->VecStrings->clear(v);
    if (ioda->VecStrings->size(v)) doErr;

    // Double check that v2 still has two elements.
    if (ioda->VecStrings->size(v2) != 2) doErr;

    goto cleanup;

failure:
  printf("%s", (errlin) ? errlin : "An unknown error has occurred somewhere.");
  errval = 1;

cleanup:
  if (v2) ioda->VecStrings->destruct(v2);
  if (v) ioda->VecStrings->destruct(v);

  return errval;
}
