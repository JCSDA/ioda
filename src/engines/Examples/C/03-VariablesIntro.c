/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/** \file 03-VariablesIntro.c
 * \brief Basic usage of Variables using the C interface
 *
 * This example parallels the C++ examples.
 * \see 03-VariablesIntro.cpp for comments and the walkthrough.
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
  struct ioda_has_variables* gvars = NULL;
  struct ioda_has_attributes* v1atts = NULL;
  struct ioda_attribute* v1a1 = NULL;
  struct ioda_variable_creation_parameters *params_default = NULL, *p1 = NULL;
  struct ioda_variable *var1 = NULL, *var2 = NULL, *var3 = NULL;
  struct ioda_variable *mixed_int_float_1 = NULL, *removable_var1 = NULL;
  struct ioda_variable *var1_reopened = NULL, *var2_reopened = NULL;
  struct ioda_string_ret_t *list_of_vars = NULL, *str_list = NULL;
  struct ioda_dimensions* dims = NULL;
  struct ioda_variable* var_strs = NULL;

  // C++ line 46
  g = ioda.Engines.constructFromCmdLine(argc, argv, "Example-03-C.hdf5");
  if (!g) doErr;

  gvars = ioda.Group.getVars(g);
  if (!gvars) doErr;

  // C++ line 61
  params_default = ioda.VariableCreationParams.create();
  if (!params_default) doErr;

  // We are creating a 2x3 array and writing it.
  const size_t var1_dimensionality = 2;
  long var1_dimsCur[2] = {2, 3};
  long var1_dimsMax[2] = {2, 3};
  var1 = ioda.Has_Variables.create_int(gvars, "var-1", var1_dimensionality, var1_dimsCur, var1_dimsMax,
                                       params_default);
  if (!var1) doErr;

  int var1_data[6] = {1, 2, 3, 4, 5, 6};
  // Note: The "6" here is the number of elements of var1_data that we are writing.
  if (!ioda.Variable.write_full_int(var1, 6, var1_data)) doErr;

  v1atts = ioda.Variable.getAtts(var1);
  const long s1sz = 1;
  v1a1 = ioda.Has_Attributes.create_str(v1atts, "Test", 1, &s1sz);
  if (!v1a1) doErr;
  const char* const v1a1_data[] = {"This is a test."};
  // We are writing a single variable-length string, which is why we are passing a "1" here.
  if (!ioda.Attribute.write_str(v1a1, 1, v1a1_data)) doErr;

  // C++ line 73
  const size_t var2_dimensionality = 3;
  long var2_dims[3] = {2, 3, 4};
  var2 = ioda.Has_Variables.create_float(gvars, "var-2", var2_dimensionality, var2_dims, var2_dims,
                                         params_default);
  if (!var2) doErr;

  float var2_data[24] = {1.1f, 2.2f, 3.14159f, 4,  5,  6,  7,  8,  9,  10, 11.5f, 12.6f,
                         13,   14,   15,       16, 17, 18, 19, 20, 21, 22, 23,    24};
  if (!ioda.Variable.write_full_float(var2, 24, var2_data)) doErr;

  // C++ line 85
  p1 = ioda.VariableCreationParams.clone(params_default);  // Slight difference in C++. Code cov.
  if (!p1) doErr;
  ptrdiff_t chunks[2] = {200, 3};
  ioda.VariableCreationParams.chunking(p1, true, 2, chunks);
  ioda.VariableCreationParams.setFillValue_int(p1, -999);
  ioda.VariableCreationParams.compressWithSZIP(p1, 0, 16);  // Turn on SZIP compression.
  ioda.VariableCreationParams.noCompress(p1);               // Turn off SZIP compression
  ioda.VariableCreationParams.compressWithGZIP(p1, 6);      // Turn on GZIP compression.

  const size_t var3_dimensionality = 2;
  long var3_dimsCur[2] = {200, 3};
  long var3_dimsMax[2] = {2000, 3};
  var3 = ioda.Has_Variables.create_int(gvars, "var-3", var3_dimensionality, var3_dimsCur, var3_dimsMax, p1);
  if (!var3) doErr;

  long var3_dimsNew[2] = {400, 3};
  if (!ioda.Variable.resize(var3, 2, var3_dimsNew)) doErr;

  // Skip C++ lines 97 - 123. C hoes not have these containers.

  // C++ line 127
  long mixed_int_float_1_dims[] = {1};
  // mixed_int_float_1 = ioda.Group.vars.create_int(gvars, "bad-int-1", 1, mixed_int_float_1_dims,
  // mixed_int_float_1_dims, params_default); if (!mixed_int_float_1) doErr; float mixed_int_float_1_test_var
  // = 3.1f; bool mixed_int_float_1_res_write = ioda.Variable.write_full_float(mixed_int_float_1, 1,
  // &mixed_int_float_1_test_var); if (!mixed_int_float_1_res_write) doErr;

  // Skip C++ lines 134-182.
  // C does not have Eigen.

  // C++ line 184
  list_of_vars = ioda.Group.vars.list(gvars);
  if (!list_of_vars) doErr;
  // if (list_of_vars->n != 4) doErr;
  // list_of_vars->strings[0 - 3] are the strings!

  // C++ line 195
  if (ioda.Group.vars.exists(gvars, "var-2") <= 0) doErr;

  // C++ line 197
  removable_var1 = ioda.Group.vars.create_int(gvars, "removable-int-1", 1, mixed_int_float_1_dims,
                                              mixed_int_float_1_dims, params_default);
  if (!removable_var1) doErr;
  ioda.Variable.destruct(removable_var1);  // We have to release the variable before we delete it!
  removable_var1 = NULL;
  if (!ioda.Group.vars.remove(gvars, "removable-int-1")) doErr;

  var1_reopened = ioda.Group.vars.open(gvars, "var-1");
  if (!var1_reopened) doErr;

  var2_reopened = ioda.Group.vars.open(gvars, "var-2");
  if (!var2_reopened) doErr;

  // C++ line 207
  // TODO(rhoneyager): Add in error handling for
  // forgivable exceptions.

  // C++ line 217
  dims = ioda.Variable.getDimensions(var1_reopened);
  if (!dims) doErr;
  size_t dimensionality = 0;
  ptrdiff_t dim = 0;
  if (!ioda.Dimensions.getDimensionality(dims, &dimensionality)) doErr;
  if (dimensionality != 2) doErr;
  if (!ioda.Dimensions.getDimCur(dims, 0, &dim)) doErr;
  if (dim != 2) doErr;
  if (!ioda.Dimensions.getDimCur(dims, 1, &dim)) doErr;
  if (dim != 3) doErr;
  if (!ioda.Dimensions.getDimMax(dims, 0, &dim)) doErr;
  if (dim != 2) doErr;
  if (ioda.Variable.isA_int(var1_reopened) <= 0) doErr;

  // C++ line 236
  int check_var1[6] = {0, 0, 0, 0, 0, 0};
  if (!ioda.Variable.read_full_int(var1_reopened, 6, check_var1)) doErr;

  // C++ line 271
  // double mixed_double_read[6] = { 0,0,0,0,0,0 };
  // bool mixed_read_success = ioda.Variable.read_full_double(var1_reopened, 6, mixed_double_read);
  // if (!mixed_read_success) doErr;

  // Some more stuff, not in the C++ example

  // Strings are a bit special, so we show how to read and write these separately.
  // Writing strings
  const char* strings[] = {"str-1", "string 2", "s3", "Hello, world!"};
  const long n_strs = 4;
  var_strs = ioda.Has_Variables.create_str(gvars, "var_strs", 1, &n_strs, &n_strs, params_default);
  if (!var_strs) doErr;
  if (!ioda.Variable.write_full_str(var_strs, n_strs, strings)) doErr;

  // String read test
  // Read into str_list. Free when done.
  str_list = ioda.Variable.read_full_str(var_strs);
  if (!str_list) doErr;
  if (str_list->n != 4) doErr;
  // Compare each string, up to 20 chars. This size is bigger than the input strings.
  for (size_t i = 0; i < 4; ++i)
    if (strncmp(strings[i], str_list->strings[i], 20) != 0) doErr;

  goto cleanup;

hadError:
  printf("%s", (errlin) ? errlin : "An unknown error has occurred somewhere.");
  errval = 1;

cleanup:
  if (g) ioda.Group.destruct(g);
  if (gvars) ioda.Has_Variables.destruct(gvars);
  if (v1atts) ioda.Has_Attributes.destruct(v1atts);
  if (params_default) ioda.VariableCreationParams.destruct(params_default);
  if (p1) ioda.VariableCreationParams.destruct(p1);
  if (var1) ioda.Variable.destruct(var1);
  if (v1a1) ioda.Attribute.destruct(v1a1);
  if (var2) ioda.Variable.destruct(var2);
  if (var3) ioda.Variable.destruct(var3);
  if (mixed_int_float_1) ioda.Variable.destruct(mixed_int_float_1);
  if (removable_var1) ioda.Variable.destruct(removable_var1);
  if (var1_reopened) ioda.Variable.destruct(var1_reopened);
  if (var2_reopened) ioda.Variable.destruct(var2_reopened);
  if (list_of_vars) ioda.Strings.destruct(list_of_vars);
  if (var_strs) ioda.Variable.destruct(var_strs);
  if (str_list) ioda.Strings.destruct(str_list);
  if (dims) ioda.Dimensions.destruct(dims);

  return errval;
}
