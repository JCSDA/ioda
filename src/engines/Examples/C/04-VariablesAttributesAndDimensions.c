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
 * \defgroup ioda_c_ex_4 Ex 4: Variables, Attributes, and Dimension Scales
 * \brief Variables, Attributes, and Dimension Scales using the C interface
 * \details This example parallels the C++ examples.
 * \see 04-VariablesAttributesAndDimensions.cpp for comments and the walkthrough.
 *
 * @{
 *
 * \file 04-VariablesAttributesAndDimensions.c
 * \brief Variables, Attributes, and Dimension Scales using the C interface
 * \see 04-VariablesAttributesAndDimensions.cpp for comments and the walkthrough.
 **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ioda/C/ioda_c.h"
#include "ioda/defs.h"  // Always include this first.

#define sslin(x) #x
#define slin(x) sslin(x)
#define doErr                                                                                      \
  {                                                                                                \
    errlin = "Error in file " __FILE__ " at line " slin(__LINE__) ".\n";                           \
    goto hadError;                                                                                 \
  }

int main(int argc, char** argv) {
  int errval                         = 0;
  const char* errlin                 = NULL;
  struct c_ioda ioda                 = use_c_ioda();
  struct ioda_group* g               = NULL;
  struct ioda_has_variables* gvars   = NULL;
  struct ioda_variable *dim_location = NULL, *dim_channel = NULL;
  struct ioda_variable_creation_parameters *params_default = NULL, *params_dimchannel = NULL,
                                           *p1 = NULL;
  int* dim_location_data                       = NULL;
  int* dim_channel_data                        = NULL;
  struct ioda_variable *var_longitude = NULL, *var_latitude = NULL;
  struct ioda_has_attributes *var_lon_atts = NULL, *var_lat_atts = NULL;
  struct ioda_attribute *v_lon_units = NULL, *v_lon_valid_range = NULL, *v_lon_long_name = NULL;
  struct ioda_attribute *v_lat_units = NULL, *v_lat_valid_range = NULL, *v_lat_long_name = NULL;
  struct ioda_variable* var_tb            = NULL;
  struct ioda_has_attributes* var_tb_atts = NULL;
  struct ioda_attribute *v_tb_units = NULL, *v_tb_valid_range = NULL, *v_tb_long_name = NULL;
  struct ioda_variable* var_sza            = NULL;
  struct ioda_has_attributes* var_sza_atts = NULL;
  struct ioda_attribute *v_sza_units = NULL, *v_sza_valid_range = NULL;

  g = ioda.Engines.constructFromCmdLine(argc, argv, "Example-04-C.hdf5");
  if (!g) doErr;

  gvars = ioda.Group.getVars(g);
  if (!gvars) doErr;

  // C++ line 70

  const long num_locs     = 3000;
  const long num_channels = 23;

  params_default = ioda.VariableCreationParams.create();
  if (!params_default) doErr;

  dim_location
    = ioda.Has_Variables.create_int(gvars, 8, "Location", 1, &num_locs, &num_locs, params_default);
  if (!dim_location) doErr;
  if (!ioda.Variable.setIsDimensionScale(dim_location, 8, "Location")) doErr;

  dim_location_data = (int*)malloc(sizeof(int) * num_locs);
  if (!dim_location_data) doErr;
  for (long i = 0; i < num_locs; ++i) dim_location_data[i] = (int)(i + 1);
  if (!ioda.Variable.write_full_int(dim_location, num_locs, dim_location_data)) doErr;

  params_dimchannel = ioda.VariableCreationParams.create();
  if (!params_dimchannel) doErr;
  if (!ioda.VariableCreationParams.setIsDimensionScale(params_dimchannel, 19, "ATMS Channel Number"))
    doErr;
  if (ioda.VariableCreationParams.isDimensionScale(params_dimchannel) <= 0) doErr;

  dim_channel = ioda.Has_Variables.create_int(gvars, 12, "ATMS Channel", 1, &num_channels,
                                              &num_channels, params_dimchannel);
  if (!dim_channel) doErr;
  if (ioda.Variable.isDimensionScale(dim_channel) <= 0) doErr;

  const char* check_str_dimscale_name = "ATMS Channel Number";
  const int buf_sz                    = 60;
  char buf[60]                        = {"\n"};
  size_t sz = ioda.Variable.getDimensionScaleName(dim_channel, buf_sz, buf);
  if (sz != 20) doErr;
  if (0 != strncmp(buf, check_str_dimscale_name, buf_sz)) doErr;

  sz = ioda.VariableCreationParams.getDimensionScaleName(params_dimchannel, buf_sz, buf);
  if (sz != 20) doErr;
  if (0 != strncmp(buf, check_str_dimscale_name, buf_sz)) doErr;

  dim_channel_data = (int*)malloc(sizeof(int) * num_channels);
  if (!dim_channel_data) doErr;
  for (long i = 0; i < num_channels; ++i) dim_channel_data[i] = (int)(i + 1);
  if (!ioda.Variable.write_full_int(dim_channel, num_channels, dim_channel_data)) doErr;

  // C++ line 98
  var_longitude = ioda.Has_Variables.create_float(gvars, 9, "Longitude", 1, &num_locs, &num_locs,
                                                  params_default);
  if (!var_longitude) doErr;
  const struct ioda_variable* dims_loc[] = {dim_location};
  if (!ioda.Variable.setDimScale(var_longitude, 1, dims_loc)) doErr;
  var_lon_atts = ioda.Variable.getAtts(var_longitude);
  if (!var_lon_atts) doErr;

  const char* lon_units          = "degrees_east";
  const char* lon_long_name      = "Longitude";
  const long valid_range_dims[]  = {2};
  const float lon_valid_range[2] = {-180, 180};
  const long str_dims[]          = {1};

  v_lon_valid_range
    = ioda.Has_Attributes.create_float(var_lon_atts, 11, "valid_range", 1, valid_range_dims);
  if (!v_lon_valid_range) doErr;
  if (!ioda.Attribute.write_float(v_lon_valid_range, 2, lon_valid_range)) doErr;

  v_lon_units = ioda.Has_Attributes.create_str(var_lon_atts, 5, "units", 1, str_dims);
  if (!v_lon_units) doErr;
  if (!ioda.Attribute.write_str(v_lon_units, 1, &lon_units)) doErr;

  v_lon_long_name = ioda.Has_Attributes.create_str(var_lon_atts, 9, "long_name", 1, str_dims);
  if (!v_lon_long_name) doErr;
  if (!ioda.Attribute.write_str(v_lon_long_name, 1, &lon_long_name)) doErr;

  // C++ line 110
  var_latitude = ioda.Has_Variables.create_float(gvars, 8, "Latitude", 1, &num_locs, &num_locs,
                                                 params_default);
  if (!var_latitude) doErr;
  if (!ioda.Variable.setDimScale(var_latitude, 1, dims_loc)) doErr;
  var_lat_atts = ioda.Variable.getAtts(var_latitude);
  if (!var_lat_atts) doErr;

  const char* lat_units          = "degrees_north";
  const char* lat_long_name      = "Latitude";
  const float lat_valid_range[2] = {-90, 90};

  v_lat_valid_range
    = ioda.Has_Attributes.create_float(var_lat_atts, 11, "valid_range", 1, valid_range_dims);
  if (!v_lat_valid_range) doErr;
  if (!ioda.Attribute.write_float(v_lat_valid_range, 2, lat_valid_range)) doErr;

  v_lat_units = ioda.Has_Attributes.create_str(var_lat_atts, 5, "units", 1, str_dims);
  if (!v_lat_units) doErr;
  if (!ioda.Attribute.write_str(v_lat_units, 1, &lat_units)) doErr;

  v_lat_long_name = ioda.Has_Attributes.create_str(var_lat_atts, 9, "long_name", 1, str_dims);
  if (!v_lat_long_name) doErr;
  if (!ioda.Attribute.write_str(v_lat_long_name, 1, &lat_long_name)) doErr;

  // C++ line 117
  const long sz_tb[] = {num_locs, num_channels};
  var_tb = ioda.Has_Variables.create_float(gvars, 22, "Brightness Temperature", 2, sz_tb, sz_tb,
                                           params_default);
  if (!var_tb) doErr;
  if (!ioda.Variable.attachDimensionScale(var_tb, 0, dim_location)) doErr;
  if (!ioda.Variable.attachDimensionScale(var_tb, 1, dim_channel)) doErr;
  var_tb_atts = ioda.Variable.getAtts(var_tb);
  if (!var_tb_atts) doErr;

  if (ioda.Variable.isDimensionScaleAttached(var_tb, 0, dim_location) <= 0) doErr;

  const char* tb_units          = "K";
  const char* tb_long_name      = "ATMS Observed (Uncorrected) Brightness Temperature";
  const float tb_valid_range[2] = {100, 400};

  v_tb_valid_range
    = ioda.Has_Attributes.create_float(var_tb_atts, 11, "valid_range", 1, valid_range_dims);
  if (!v_tb_valid_range) doErr;
  if (!ioda.Attribute.write_float(v_tb_valid_range, 2, tb_valid_range)) doErr;

  v_tb_units = ioda.Has_Attributes.create_str(var_tb_atts, 5, "units", 1, str_dims);
  if (!v_tb_units) doErr;
  if (!ioda.Attribute.write_str(v_tb_units, 1, &tb_units)) doErr;

  v_tb_long_name = ioda.Has_Attributes.create_str(var_tb_atts, 9, "long_name", 1, str_dims);
  if (!v_tb_long_name) doErr;
  if (!ioda.Attribute.write_str(v_tb_long_name, 1, &tb_long_name)) doErr;

  // C++ line 155
  p1 = ioda.VariableCreationParams.create();
  if (!p1) doErr;
  ioda.VariableCreationParams.setFillValue_float(p1, -999);
  const ptrdiff_t p1_chunks[] = {100};
  ioda.VariableCreationParams.chunking(p1, true, 1, p1_chunks);
  ioda.VariableCreationParams.compressWithGZIP(p1, 6);
  const struct ioda_variable* sza_dimscale[] = {dim_location};
  ioda.VariableCreationParams.setDimScale(p1, 1, sza_dimscale);
  if (!ioda.VariableCreationParams.hasSetDimScales(p1)) doErr;

  // C++ line 166
  var_sza
    = ioda.Has_Variables.create_float(gvars, 18, "Solar Zenith Angle", 1, &num_locs, &num_locs, p1);
  if (!var_sza) doErr;
  if (ioda.Variable.isDimensionScaleAttached(var_sza, 0, dim_location) <= 0) doErr;
  if (!ioda.Variable.detachDimensionScale(var_sza, 0, dim_location))
    doErr;  // Just trying to check...
  if (ioda.Variable.isDimensionScaleAttached(var_sza, 0, dim_location) != 0) doErr;
  if (!ioda.Variable.attachDimensionScale(var_sza, 0, dim_location)) doErr;
  if (ioda.Variable.isDimensionScaleAttached(var_sza, 0, dim_location) <= 0) doErr;
  var_sza_atts = ioda.Variable.getAtts(var_sza);
  if (!var_sza_atts) doErr;

  const char* sza_units          = "degrees";
  const float sza_valid_range[2] = {-90, 90};

  v_sza_valid_range
    = ioda.Has_Attributes.create_float(var_sza_atts, 11, "valid_range", 1, valid_range_dims);
  if (!v_sza_valid_range) doErr;
  if (!ioda.Attribute.write_float(v_sza_valid_range, 2, sza_valid_range)) doErr;

  v_sza_units = ioda.Has_Attributes.create_str(var_sza_atts, 5, "units", 1, str_dims);
  if (!v_sza_units) doErr;
  if (!ioda.Attribute.write_str(v_sza_units, 1, &sza_units)) doErr;

  // This is effectively a no-op, but we want to test that the code executes.
  if (!ioda.VariableCreationParams.attachDimensionScale(p1, 1, dim_channel)) doErr;

  goto cleanup;

hadError:
  printf("%s", (errlin) ? errlin : "An unknown error has occurred somewhere.");
  errval = 1;

cleanup:
  if (g) ioda.Group.destruct(g);
  if (gvars) ioda.Has_Variables.destruct(gvars);
  if (dim_location) ioda.Variable.destruct(dim_location);
  if (dim_channel) ioda.Variable.destruct(dim_channel);
  if (params_default) ioda.VariableCreationParams.destruct(params_default);
  if (params_dimchannel) ioda.VariableCreationParams.destruct(params_dimchannel);
  if (p1) ioda.VariableCreationParams.destruct(p1);
  if (dim_location_data) free((void*)dim_location_data);
  if (dim_channel_data) free((void*)dim_channel_data);
  if (var_longitude) ioda.Variable.destruct(var_longitude);
  if (var_latitude) ioda.Variable.destruct(var_latitude);
  if (var_lon_atts) ioda.Has_Attributes.destruct(var_lon_atts);
  if (var_lat_atts) ioda.Has_Attributes.destruct(var_lat_atts);
  if (v_lon_units) ioda.Attribute.destruct(v_lon_units);
  if (v_lon_valid_range) ioda.Attribute.destruct(v_lon_valid_range);
  if (v_lon_long_name) ioda.Attribute.destruct(v_lon_long_name);
  if (v_lat_units) ioda.Attribute.destruct(v_lat_units);
  if (v_lat_valid_range) ioda.Attribute.destruct(v_lat_valid_range);
  if (v_lat_long_name) ioda.Attribute.destruct(v_lat_long_name);
  if (var_tb) ioda.Variable.destruct(var_tb);
  if (var_tb_atts) ioda.Has_Attributes.destruct(var_tb_atts);
  if (v_tb_units) ioda.Attribute.destruct(v_tb_units);
  if (v_tb_valid_range) ioda.Attribute.destruct(v_tb_valid_range);
  if (v_tb_long_name) ioda.Attribute.destruct(v_tb_long_name);
  if (var_sza) ioda.Variable.destruct(var_sza);
  if (var_sza_atts) ioda.Has_Attributes.destruct(var_sza_atts);
  if (v_sza_units) ioda.Attribute.destruct(v_sza_units);
  if (v_sza_valid_range) ioda.Attribute.destruct(v_sza_valid_range);

  return errval;
}
