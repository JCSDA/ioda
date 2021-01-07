#pragma once
/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// \file Has_Variables_c.h
/// \brief C bindings for ioda::Has_Variables
#include <stdbool.h>

#include "../defs.h"
#include "./Variable_Creation_Parameters_c.h"
#include "./c_binding_macros.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ioda_group;
struct ioda_has_attributes;
struct ioda_attribute;
struct ioda_has_variables;
struct ioda_variable;
struct ioda_variable_creation_parameters;
struct c_variable_creation_parameters;

IODA_DL void ioda_has_variables_destruct(struct ioda_has_variables* has_atts);
IODA_DL struct ioda_string_ret_t* ioda_has_variables_list(const struct ioda_has_variables*);
/// \returns <0 on error, ==0 if does not exist, >0 if exists.
IODA_DL int ioda_has_variables_exists(const struct ioda_has_variables* has_vars, const char* name);
IODA_DL bool ioda_has_variables_remove(struct ioda_has_variables* has_vars, const char* name);
IODA_DL struct ioda_variable* ioda_has_variables_open(const struct ioda_has_variables* has_vars,
                                                      const char* name);

// ioda_has_attributes_create_* functions

#define IODA_HAS_VARIABLES_CREATE_TEMPLATE(funcnamestr, junk)                               \
  IODA_DL struct ioda_variable* funcnamestr(                                                \
    struct ioda_has_variables* has_vars, const char* name, size_t n_dims, const long* dims, \
    const long* max_dims,                                                                   \
    const struct ioda_variable_creation_parameters* params);  // NOLINT: cppcheck complains about long

C_TEMPLATE_FUNCTION_DECLARATION(ioda_has_variables_create, IODA_HAS_VARIABLES_CREATE_TEMPLATE);

struct c_has_variables {
  void (*destruct)(struct ioda_has_variables*);
  struct ioda_string_ret_t* (*list)(const struct ioda_has_variables*);
  int (*exists)(const struct ioda_has_variables*, const char*);
  bool (*remove)(struct ioda_has_variables*, const char*);
  struct ioda_variable* (*open)(const struct ioda_has_variables*, const char*);

#define IODA_HAS_VARIABLES_CREATE_FUNC_TEMPLATE(shortnamestr, basenamestr)     \
  struct ioda_variable* (*shortnamestr)(                                       \
    struct ioda_has_variables*, const char*, size_t, const long*, const long*, \
    const struct ioda_variable_creation_parameters*);  // NOLINT: cppcheck complains about long
  C_TEMPLATE_FUNCTION_DECLARATION_3(create, ioda_has_variables_create,
                                    IODA_HAS_VARIABLES_CREATE_FUNC_TEMPLATE);

  struct c_variable_creation_parameters VariableCreationParams;
};

#ifdef __cplusplus
}
#endif
