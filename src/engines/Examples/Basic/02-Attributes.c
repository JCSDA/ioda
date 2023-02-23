/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \addtogroup ioda_c_ex
 *
 * @{
 *
 * \defgroup ioda_c_ex_2 Ex 2: Attribute manipulation
 * \brief Attribute manipulation using the C interface
 * \details This example parallels the C++ examples.
 * \see 02-Attributes.cpp for comments and the walkthrough.
 *
 * @{
 *
 * \file 02-Attributes.c
 * \brief Attribute manipulation using the C interface
 * \see 02-Attributes.cpp for comments and the walkthrough.
 **/

#include <stdio.h>
#include <string.h>

#include "ioda/C/ioda_c.h"
#include "ioda/C/Group_c.h"
#include "ioda/C/Engines_c.h"
#include "ioda/C/Has_Attributes_c.h"
#include "ioda/C/Attribute_c.h"
#include "ioda/C/Dimensions_c.h"
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
  int errval                           = 0;
  const char* errlin                   = NULL;
  const struct ioda_c_interface* ioda  = get_ioda_c_interface();
  struct ioda_group* g                 = NULL;
  struct ioda_has_attributes* gatts    = NULL;
  struct ioda_attribute *intatt1 = NULL, *intatt2 = NULL, *intatt3 = NULL, *intatt4 = NULL;
  struct ioda_attribute *intatt5 = NULL, *intatt6 = NULL, *intatt7 = NULL, *intatt8 = NULL;
  struct ioda_attribute *floatatt1 = NULL, *doubleatt1 = NULL, *stratt1 = NULL, *charatt1 = NULL;
  struct ioda_attribute *f1 = NULL, *d1 = NULL;
  struct ioda_dimensions* f1_dims    = NULL;
  struct ioda_VecString *att_list = NULL, *str_list = NULL;

  g = ioda->Engines->constructFromCmdLine(argc, argv, "Example-02-C.hdf5");
  if (!g) doErr
  att_list = ioda->VecStrings->construct();
  str_list = ioda->VecStrings->construct();
  gatts = ioda->Groups->getAtts(g);
  if (!gatts) doErr;

  long intatt1_dims[1] = {1};
  intatt1              = ioda->Has_Attributes->create->create_int(g->atts, 9, "int-att-1", 1, intatt1_dims);
  if (!intatt1) doErr;
  int intatt1_data[1] = {5};
  if (!ioda->Attributes->write->write_int(intatt1, 1, intatt1_data)) doErr;

  long intatt2_dims[1] = {2};
  intatt2              = ioda->Has_Attributes->create->create_int(g->atts, 9, "int-att-2", 1, intatt2_dims);
  if (!intatt2) doErr;
  int intatt2_data[] = {1, 2};
  if (!ioda->Attributes->write->write_int(intatt2, 2, intatt2_data)) doErr;

  long intatt3_dims[1] = {3};
  intatt3              = ioda->Has_Attributes->create->create_int(g->atts, 9, "int-att-3", 1, intatt3_dims);
  if (!intatt3) doErr;
  int intatt3_data[] = {1, 2, 3};
  if (!ioda->Attributes->write->write_int(intatt3, 3, intatt3_data)) doErr;

  long intatt4_dims[1] = {1};
  intatt4              = ioda->Has_Attributes->create->create_int(g->atts, 9, "int-att-4", 1, intatt4_dims);
  if (!intatt4) doErr;
  int intatt4_data[] = {42};
  if (!ioda->Attributes->write->write_int(intatt4, 1, intatt4_data)) doErr;

  long intatt5_dims[] = {9};
  intatt5             = ioda->Has_Attributes->create->create_int(g->atts, 9, "int-att-5", 1, intatt5_dims);
  if (!intatt5) doErr;
  int intatt5_data[] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
  if (!ioda->Attributes->write->write_int(intatt5, 9, intatt5_data)) doErr;

  long intatt6_dims[] = {6};
  intatt6             = ioda->Has_Attributes->create->create_int(g->atts, 9, "int-att-6", 1, intatt6_dims);
  if (!intatt6) doErr;
  int intatt6_data[] = {1, 2, 3, 4, 5, 6};
  if (!ioda->Attributes->write->write_int(intatt6, 6, intatt6_data)) doErr;

  long intatt7_dims[] = {4};
  intatt7             = ioda->Has_Attributes->create->create_int(g->atts, 9, "int-att-7", 1, intatt7_dims);
  if (!intatt7) doErr;
  int intatt7_data[] = {1, 2, 3, 4};
  if (!ioda->Attributes->write->write_int(intatt7, 4, intatt7_data)) doErr;

  long intatt8_dims[] = {7};
  intatt8             = ioda->Has_Attributes->create->create_int(g->atts, 9, "int-att-8", 1, intatt8_dims);
  if (!intatt8) doErr;
  int intatt8_data[] = {1, 2, 3, 4, 5, 6, 7};
  if (!ioda->Attributes->write->write_int(intatt8, 7, intatt8_data)) doErr;

  long float1_dims[] = {2};
  floatatt1          = ioda->Has_Attributes->create->create_float(g->atts, 7, "float-1", 1, float1_dims);
  if (!floatatt1) doErr;
  float float1_data[] = {3.1159f, 2.78f};
  if (!ioda->Attributes->write->write_float(floatatt1, 2, float1_data)) doErr;

  long double1_dims[] = {4};
  doubleatt1          = ioda->Has_Attributes->create->create_double(g->atts, 8, "double-1", 1, double1_dims);
  if (!doubleatt1) doErr;
  double double1_data[] = {1.1, 2.2, 3.3, 4.4};
  if (!ioda->Attributes->write->write_double(doubleatt1, 4, double1_data)) doErr;

  long str1_dims[] = {1};
  stratt1          = ioda->Has_Attributes->create->create_str(g->atts, 5, "str-1", 1, str1_dims);
  if (!stratt1) doErr;
  ioda->VecStrings->clear(str_list);
  ioda->VecStrings->resize(str_list, 1);
  const char* str1_data1  = "This is a test.";
  if (!ioda->VecStrings->setFromCharArray(str_list, 0, str1_data1, strlen(str1_data1))) doErr;
  if (!ioda->Attributes->write->write_str(stratt1, str_list)) doErr;

  if (!ioda->Attributes->read->read_str(stratt1, str_list)) doErr;
  if (ioda->VecStrings->size(str_list) != 1) doErr;

  // Note that a character attribute is not the same as a string attribute.
  // C and HDF5 frequently interpret chars as one-byte integers, so we do not store strings as char sequences.

  // Note also that, unfortunately, some of the C compilers that we test still refuse to admit that
  // a const int is a perfectly valid array size specifier. So, we need to use #define.
#define char1_data_length 15
  const char char1_data[char1_data_length] = "This is a test";  // Note: an array, not a pointer.
  long char1_dims[1]        = {char1_data_length};              // Note: sizeof(array) vs sizeof(pointer)
  charatt1                  = ioda->Has_Attributes->create->create_char(g->atts, 6, "char-1", 1, char1_dims);
  if (!charatt1) doErr;
  if (!ioda->Attributes->write->write_char(charatt1, char1_data_length, char1_data)) doErr;

  char char1_data_check[15];
  if (!ioda->Attributes->read->read_char(charatt1, char1_data_length, char1_data_check)) doErr;
  if (strncmp(char1_data, char1_data_check, char1_data_length)) doErr;

  if (!ioda->Has_Attributes->list(g->atts, att_list)) doErr;
  if (ioda->VecStrings->size(att_list) != 12) doErr;
  // att_list->strings[0 - 11] are the strings!

  f1 = ioda->Has_Attributes->open(g->atts, 7, "float-1");
  if (!f1) doErr;
  d1 = ioda->Has_Attributes->open(g->atts, 8, "double-1");
  if (!d1) doErr;

  f1_dims = ioda->Attributes->getDimensions(f1);
  if (!f1_dims) doErr;
  size_t f1_dimensionality = 0;
  if (!ioda->Dimensions->getDimensionality(f1_dims, &f1_dimensionality)) doErr;
  if (f1_dimensionality != 1) doErr;
  ptrdiff_t dim = 0;
  if (!ioda->Dimensions->getDimCur(f1_dims, 0, &dim)) doErr;
  if (dim != 2) doErr;
  if (!ioda->Dimensions->getDimMax(f1_dims, 0, &dim)) doErr;
  if (dim != 2) doErr;
  size_t f1_numelems = 0;
  if (!ioda->Dimensions->getNumElements(f1_dims, &f1_numelems)) doErr;
  if (f1_numelems != 2) doErr;

  // Updating the struct does not update the attribute's dimensions.
  if (!ioda->Dimensions->setDimensionality(f1_dims, 3)) doErr;
  if (!ioda->Dimensions->setDimCur(f1_dims, 1, 5)) doErr;
  if (!ioda->Dimensions->setDimCur(f1_dims, 2, 7)) doErr;
  if (!ioda->Dimensions->setDimMax(f1_dims, 1, 5)) doErr;
  if (!ioda->Dimensions->setDimMax(f1_dims, 2, 7)) doErr;

  if (ioda->Attributes->isA->isA_int(intatt1) <= 0) doErr;

  int check_intatt1_val = 0;
  if (!ioda->Attributes->read->read_int(intatt1, 1, &check_intatt1_val)) doErr;
  if (check_intatt1_val != 5) doErr;

  float check_floatatt1_val[2];
  if (!ioda->Attributes->read->read_float(floatatt1, 2, check_floatatt1_val)) doErr;

  double check_doubleatt1_val[4];
  if (!ioda->Attributes->read->read_double(doubleatt1, 4, check_doubleatt1_val)) doErr;

  if (ioda->Has_Attributes->exists(g->atts, 9, "int-att-1") <= 0) doErr;

  if (!ioda->Has_Attributes->rename_att(g->atts, 9, "int-att-2", 10, "int-add-2b")) doErr;

  ioda->Attributes->destruct(intatt3);
  intatt3 = NULL;

  if (!ioda->Has_Attributes->remove(g->atts, 9, "int-att-3")) doErr;

  goto cleanup;

hadError:
  printf("%s", (errlin) ? errlin : "An unknown error has occurred somewhere.");
  errval = 1;
cleanup:
  if (g) ioda->Groups->destruct(g);
  if (intatt1) ioda->Attributes->destruct(intatt1);
  if (intatt2) ioda->Attributes->destruct(intatt2);
  if (intatt3) ioda->Attributes->destruct(intatt3);
  if (intatt4) ioda->Attributes->destruct(intatt4);
  if (intatt5) ioda->Attributes->destruct(intatt5);
  if (intatt6) ioda->Attributes->destruct(intatt6);
  if (intatt7) ioda->Attributes->destruct(intatt7);
  if (intatt8) ioda->Attributes->destruct(intatt8);
  if (floatatt1) ioda->Attributes->destruct(floatatt1);
  if (doubleatt1) ioda->Attributes->destruct(doubleatt1);
  if (stratt1) ioda->Attributes->destruct(stratt1);
  if (charatt1) ioda->Attributes->destruct(charatt1);
  if (f1) ioda->Attributes->destruct(f1);
  if (d1) ioda->Attributes->destruct(d1);
  if (f1_dims) ioda->Dimensions->destruct(f1_dims);
  if (att_list) ioda->VecStrings->destruct(att_list);
  if (str_list) ioda->VecStrings->destruct(str_list);

  return errval;
}
