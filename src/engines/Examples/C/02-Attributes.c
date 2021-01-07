/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/** \file 02-Attributes.c
 * \brief Attribute manipulation using the C interface
 *
 * This example parallels the C++ examples.
 * \see 02-Attributes.cpp for comments and the walkthrough.
 *
 * \author Ryan Honeyager (honeyage@ucar.edu)
 **/

#include <stdio.h>
#include <string.h>

#include "ioda/C/ioda_c.h"
#include "ioda/defs.h"  // Always include this first.

#define sslin(x) #x
#define slin(x) sslin(x)
#define doErr                                                       \
  {                                                                 \
    errlin = "Error in " __FILE__ " at line " slin(__LINE__) ".\n"; \
    goto hadError;                                                  \
  }

int main(int argc, char** argv) {
  int errval = 0;
  const char* errlin = NULL;
  struct c_ioda ioda = use_c_ioda();
  struct ioda_group* g = NULL;
  struct ioda_has_attributes* gatts = NULL;
  struct ioda_attribute *intatt1 = NULL, *intatt2 = NULL, *intatt3 = NULL, *intatt4 = NULL;
  struct ioda_attribute *intatt5 = NULL, *intatt6 = NULL, *intatt7 = NULL, *intatt8 = NULL;
  struct ioda_attribute *floatatt1 = NULL, *doubleatt1 = NULL, *stratt1 = NULL, *charatt1 = NULL;
  struct ioda_attribute *f1 = NULL, *d1 = NULL;
  struct ioda_dimensions* f1_dims = NULL;
  struct ioda_string_ret_t *att_list = NULL, *str_list = NULL;

  g = ioda.Engines.constructFromCmdLine(argc, argv, "Example-02-C.hdf5");
  if (!g) doErr;

  gatts = ioda.Group.getAtts(g);
  if (!gatts) doErr;

  long intatt1_dims[1] = {1};
  intatt1 = ioda.Group.atts.create_int(gatts, "int-att-1", 1, intatt1_dims);
  if (!intatt1) doErr;
  int intatt1_data[1] = {5};
  if (!ioda.Attribute.write_int(intatt1, 1, intatt1_data)) doErr;

  long intatt2_dims[1] = {2};
  intatt2 = ioda.Group.atts.create_int(gatts, "int-att-2", 1, intatt2_dims);
  if (!intatt2) doErr;
  int intatt2_data[] = {1, 2};
  if (!ioda.Attribute.write_int(intatt2, 2, intatt2_data)) doErr;

  long intatt3_dims[1] = {3};
  intatt3 = ioda.Group.atts.create_int(gatts, "int-att-3", 1, intatt3_dims);
  if (!intatt3) doErr;
  int intatt3_data[] = {1, 2, 3};
  if (!ioda.Attribute.write_int(intatt3, 3, intatt3_data)) doErr;

  long intatt4_dims[1] = {1};
  intatt4 = ioda.Group.atts.create_int(gatts, "int-att-4", 1, intatt4_dims);
  if (!intatt4) doErr;
  int intatt4_data[] = {42};
  if (!ioda.Attribute.write_int(intatt4, 1, intatt4_data)) doErr;

  long intatt5_dims[] = {9};
  intatt5 = ioda.Group.atts.create_int(gatts, "int-att-5", 1, intatt5_dims);
  if (!intatt5) doErr;
  int intatt5_data[] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
  if (!ioda.Attribute.write_int(intatt5, 9, intatt5_data)) doErr;

  long intatt6_dims[] = {6};
  intatt6 = ioda.Group.atts.create_int(gatts, "int-att-6", 1, intatt6_dims);
  if (!intatt6) doErr;
  int intatt6_data[] = {1, 2, 3, 4, 5, 6};
  if (!ioda.Attribute.write_int(intatt6, 6, intatt6_data)) doErr;

  long intatt7_dims[] = {4};
  intatt7 = ioda.Group.atts.create_int(gatts, "int-att-7", 1, intatt7_dims);
  if (!intatt7) doErr;
  int intatt7_data[] = {1, 2, 3, 4};
  if (!ioda.Attribute.write_int(intatt7, 4, intatt7_data)) doErr;

  long intatt8_dims[] = {7};
  intatt8 = ioda.Group.atts.create_int(gatts, "int-att-8", 1, intatt8_dims);
  if (!intatt8) doErr;
  int intatt8_data[] = {1, 2, 3, 4, 5, 6, 7};
  if (!ioda.Attribute.write_int(intatt8, 7, intatt8_data)) doErr;

  long float1_dims[] = {2};
  floatatt1 = ioda.Group.atts.create_float(gatts, "float-1", 1, float1_dims);
  if (!floatatt1) doErr;
  float float1_data[] = {3.1159f, 2.78f};
  if (!ioda.Attribute.write_float(floatatt1, 2, float1_data)) doErr;

  long double1_dims[] = {4};
  doubleatt1 = ioda.Group.atts.create_double(gatts, "double-1", 1, double1_dims);
  if (!doubleatt1) doErr;
  double double1_data[] = {1.1, 2.2, 3.3, 4.4};
  if (!ioda.Attribute.write_double(doubleatt1, 4, double1_data)) doErr;

  long str1_dims[] = {1};
  stratt1 = ioda.Group.atts.create_str(gatts, "str-1", 1, str1_dims);
  if (!stratt1) doErr;
  const char* str1_data1 = "This is a test.";
  const char* str1_data[] = {str1_data1};
  if (!ioda.Attribute.write_str(stratt1, 1, str1_data)) doErr;

  const char char1_data[15] = "This is a test";  // Note: an array, not a pointer.
  long char1_dims[1] = {15};                     // Note: sizeof(array) vs sizeof(pointer)
  charatt1 = ioda.Group.atts.create_char(gatts, "char-1", 1, char1_dims);
  if (!charatt1) doErr;
  if (!ioda.Attribute.write_char(charatt1, 15, char1_data)) doErr;

  str_list = ioda.Attribute.read_str(stratt1);
  if (!str_list) doErr;
  if (str_list->n != 1) doErr;

  att_list = ioda.Group.atts.list(gatts);
  if (!att_list) doErr;
  if (att_list->n != 12) doErr;
  // att_list->strings[0 - 11] are the strings!

  f1 = ioda.Group.atts.open(gatts, "float-1");
  if (!f1) doErr;
  d1 = ioda.Group.atts.open(gatts, "double-1");
  if (!d1) doErr;

  f1_dims = ioda.Attribute.getDimensions(f1);
  if (!f1_dims) doErr;
  size_t f1_dimensionality = 0;
  if (!ioda.Dimensions.getDimensionality(f1_dims, &f1_dimensionality)) doErr;
  if (f1_dimensionality != 1) doErr;
  ptrdiff_t dim = 0;
  if (!ioda.Dimensions.getDimCur(f1_dims, 0, &dim)) doErr;
  if (dim != 2) doErr;
  if (!ioda.Dimensions.getDimMax(f1_dims, 0, &dim)) doErr;
  if (dim != 2) doErr;
  size_t f1_numelems = 0;
  if (!ioda.Dimensions.getNumElements(f1_dims, &f1_numelems)) doErr;
  if (f1_numelems != 2) doErr;

  // Updating the struct does not update the attribute's dimensions.
  if (!ioda.Dimensions.setDimensionality(f1_dims, 3)) doErr;
  if (!ioda.Dimensions.setDimCur(f1_dims, 1, 5)) doErr;
  if (!ioda.Dimensions.setDimCur(f1_dims, 2, 7)) doErr;
  if (!ioda.Dimensions.setDimMax(f1_dims, 1, 5)) doErr;
  if (!ioda.Dimensions.setDimMax(f1_dims, 2, 7)) doErr;

  if (ioda.Attribute.isA_int(intatt1) <= 0) doErr;

  int check_intatt1_val = 0;
  if (!ioda.Attribute.read_int(intatt1, 1, &check_intatt1_val)) doErr;
  if (check_intatt1_val != 5) doErr;

  float check_floatatt1_val[2];
  if (!ioda.Attribute.read_float(floatatt1, 2, check_floatatt1_val)) doErr;

  double check_doubleatt1_val[4];
  if (!ioda.Attribute.read_double(doubleatt1, 4, check_doubleatt1_val)) doErr;

  if (ioda.Has_Attributes.exists(gatts, "int-att-1") <= 0) doErr;

  if (!ioda.Has_Attributes.rename_att(gatts, "int-att-2", "int-add-2b")) doErr;

  ioda.Attribute.destruct(intatt3);
  intatt3 = NULL;

  if (!ioda.Has_Attributes.remove(gatts, "int-att-3")) doErr;

  goto cleanup;

hadError:
  printf("%s", (errlin) ? errlin : "An unknown error has occurred somewhere.");
  errval = 1;
cleanup:
  if (g) ioda.Group.destruct(g);
  if (gatts) ioda.Has_Attributes.destruct(gatts);
  if (intatt1) ioda.Attribute.destruct(intatt1);
  if (intatt2) ioda.Attribute.destruct(intatt2);
  if (intatt3) ioda.Attribute.destruct(intatt3);
  if (intatt4) ioda.Attribute.destruct(intatt4);
  if (intatt5) ioda.Attribute.destruct(intatt5);
  if (intatt6) ioda.Attribute.destruct(intatt6);
  if (intatt7) ioda.Attribute.destruct(intatt7);
  if (intatt8) ioda.Attribute.destruct(intatt8);
  if (floatatt1) ioda.Attribute.destruct(floatatt1);
  if (doubleatt1) ioda.Attribute.destruct(doubleatt1);
  if (stratt1) ioda.Attribute.destruct(stratt1);
  if (charatt1) ioda.Attribute.destruct(charatt1);
  if (f1) ioda.Attribute.destruct(f1);
  if (d1) ioda.Attribute.destruct(d1);
  if (f1_dims) ioda.Dimensions.destruct(f1_dims);
  if (att_list) ioda.Strings.destruct(att_list);
  if (str_list) ioda.Strings.destruct(str_list);

  return errval;
}
