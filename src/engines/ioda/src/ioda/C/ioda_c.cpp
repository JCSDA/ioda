/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \addtogroup ioda_c
 * @{
 * \file ioda_c.cpp
 * \brief C bindings for ioda-engines. Provides a class-like structure.
 */
#include "ioda/C/ioda_c.h"

/** \brief Creates and returns a c_ioda struct that has all of the function pointers filled in.
 *
 * Unfortunately, you can't have a default initializer in a struct in C.
 **/
extern "C" c_ioda use_c_ioda() {
  c_ioda res;  // NOLINT: Obviously uninitialized. That's the purpose of this function.

  // Strings
  { res.Strings.destruct = &ioda_string_ret_t_destruct; }

  // Dimensions
  {
    res.Dimensions.destruct          = &ioda_dimensions_destruct;
    res.Dimensions.getDimCur         = &ioda_dimensions_get_dim_cur;
    res.Dimensions.getDimensionality = &ioda_dimensions_get_dimensionality;
    res.Dimensions.getDimMax         = &ioda_dimensions_get_dim_max;
    res.Dimensions.getNumElements    = &ioda_dimensions_get_num_elements;
    res.Dimensions.setDimCur         = &ioda_dimensions_set_dim_cur;
    res.Dimensions.setDimensionality = &ioda_dimensions_set_dimensionality;
    res.Dimensions.setDimMax         = &ioda_dimensions_set_dim_max;
  }

  // Engines
  {
    res.Engines.constructFromCmdLine = &ioda_Engines_constructFromCmdLine;
    {
      res.Engines.HH.createFile       = &ioda_Engines_HH_createFile;
      res.Engines.HH.createMemoryFile = &ioda_Engines_HH_createMemoryFile;
      res.Engines.HH.openFile         = &ioda_Engines_HH_openFile;
    }
    { res.Engines.ObsStore.createRootGroup = &ioda_Engines_ObsStore_createRootGroup; }
  }

  // Groups
  {
    // res.Group.atts is considered later.
    // res.Group.vars is considered later.
    res.Group.create   = &ioda_group_create;
    res.Group.destruct = &ioda_group_destruct;
    res.Group.exists   = &ioda_group_exists;
    res.Group.getAtts  = &ioda_group_atts;
    res.Group.getVars  = &ioda_group_vars;
    res.Group.list     = &ioda_group_list;
    res.Group.open     = &ioda_group_open;
  }

  // Attributes
  {
    res.Attribute.destruct      = &ioda_attribute_destruct;
    res.Attribute.getDimensions = &ioda_attribute_get_dimensions;

#define IODA_ATTRIBUTE_INST_TEMPLATE(shortnamestr, basenamestr)                                    \
  res.Attribute.shortnamestr = basenamestr;

    // res.Attribute.isA_*
    C_TEMPLATE_FUNCTION_DECLARATION_3(isA, ioda_attribute_isa, IODA_ATTRIBUTE_INST_TEMPLATE);

    // res.Attribute.read_*
    C_TEMPLATE_FUNCTION_DECLARATION_3(read, ioda_attribute_read, IODA_ATTRIBUTE_INST_TEMPLATE);

    // res.Attribute.write_*
    C_TEMPLATE_FUNCTION_DECLARATION_3(write, ioda_attribute_write, IODA_ATTRIBUTE_INST_TEMPLATE);
  }

  // Has_Attributes
  {
#define IODA_HAS_ATTRIBUTES_INST_TEMPLATE(shortnamestr, basenamestr)                               \
  res.Has_Attributes.shortnamestr = basenamestr;
    // res.Has_Attributes.create_*
    C_TEMPLATE_FUNCTION_DECLARATION_3(create, ioda_has_attributes_create,
                                      IODA_HAS_ATTRIBUTES_INST_TEMPLATE);

    res.Has_Attributes.destruct   = &ioda_has_attributes_destruct;
    res.Has_Attributes.exists     = &ioda_has_attributes_exists;
    res.Has_Attributes.list       = &ioda_has_attributes_list;
    res.Has_Attributes.open       = &ioda_has_attributes_open;
    res.Has_Attributes.remove     = &ioda_has_attributes_remove;
    res.Has_Attributes.rename_att = &ioda_has_attributes_rename;
  }

  // Variable Creation Parameters
  {
    res.VariableCreationParams.destruct = &ioda_variable_creation_parameters_destruct;
    res.VariableCreationParams.create   = &ioda_variable_creation_parameters_create;
    res.VariableCreationParams.clone    = &ioda_variable_creation_parameters_clone;

#define IODA_VCP_INST_TEMPLATE(shortnamestr, basenamestr)                                          \
  res.VariableCreationParams.shortnamestr = basenamestr;

    // res.VariableCreationParams.setFillValue_*
    C_TEMPLATE_FUNCTION_DECLARATION_3_NOSTR(
      setFillValue, ioda_variable_creation_parameters_setFillValue, IODA_VCP_INST_TEMPLATE);

    res.VariableCreationParams.chunking   = &ioda_variable_creation_parameters_chunking;
    res.VariableCreationParams.noCompress = &ioda_variable_creation_parameters_noCompress;
    res.VariableCreationParams.compressWithGZIP
      = &ioda_variable_creation_parameters_compressWithGZIP;
    res.VariableCreationParams.compressWithSZIP
      = &ioda_variable_creation_parameters_compressWithSZIP;
  }

  // Variables
  {
    res.Variable.destruct                 = &ioda_variable_destruct;
    res.Variable.getAtts                  = &ioda_variable_atts;
    res.Variable.getDimensions            = &ioda_variable_get_dimensions;
    res.Variable.resize                   = &ioda_variable_resize;
    res.Variable.attachDimensionScale     = &ioda_variable_attachDimensionScale;
    res.Variable.detachDimensionScale     = &ioda_variable_detachDimensionScale;
    res.Variable.setDimScale              = &ioda_variable_setDimScale;
    res.Variable.isDimensionScale         = &ioda_variable_isDimensionScale;
    res.Variable.setIsDimensionScale      = &ioda_variable_setIsDimensionScale;
    res.Variable.getDimensionScaleName    = &ioda_variable_getDimensionScaleName;
    res.Variable.isDimensionScaleAttached = &ioda_variable_isDimensionScaleAttached;

#define IODA_VARIABLE_INST_TEMPLATE(shortnamestr, basenamestr)                                     \
  res.Variable.shortnamestr = basenamestr;

    // res.Variable.isA_*
    C_TEMPLATE_FUNCTION_DECLARATION_3(isA, ioda_variable_isa, IODA_VARIABLE_INST_TEMPLATE);

    // res.Variable.read_*
    C_TEMPLATE_FUNCTION_DECLARATION_3(read_full, ioda_variable_read_full,
                                      IODA_VARIABLE_INST_TEMPLATE);

    // res.Variable.write_*
    C_TEMPLATE_FUNCTION_DECLARATION_3(write_full, ioda_variable_write_full,
                                      IODA_VARIABLE_INST_TEMPLATE);

    res.Variable.atts = res.Has_Attributes;
  }

  // Has_Variables
  {
    res.Has_Variables.destruct = &ioda_has_variables_destruct;
    res.Has_Variables.list     = &ioda_has_variables_list;
    res.Has_Variables.exists   = &ioda_has_variables_exists;
    res.Has_Variables.remove   = &ioda_has_variables_remove;
    res.Has_Variables.open     = &ioda_has_variables_open;

#define IODA_HAS_VARIABLES_INST_TEMPLATE(shortnamestr, basenamestr)                                \
  res.Has_Variables.shortnamestr = basenamestr;
    // res.Has_Variables.create_*
    C_TEMPLATE_FUNCTION_DECLARATION_3(create, ioda_has_variables_create,
                                      IODA_HAS_VARIABLES_INST_TEMPLATE);

    res.Has_Variables.VariableCreationParams = res.VariableCreationParams;
  }

  res.Group.atts = res.Has_Attributes;
  res.Group.vars = res.Has_Variables;

  return res;
}

/// @}
