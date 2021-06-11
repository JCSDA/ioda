#pragma once
/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \defgroup ioda_has_variables Has_Variables
 * \brief Provides the C-style interface for ioda::Has_Variables objects.
 * \ingroup ioda_c_api
 *
 * @{
 * \file Has_Variables_c.h
 * \brief @link ioda_has_variables C bindings @endlink for ioda::Has_Variables
 */
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

/// \brief Deallocates a ioda_has_variables.
/// \param has_vars is the object.
IODA_DL void ioda_has_variables_destruct(struct ioda_has_variables* has_vars);

/// \brief List the names of the variables associated with an object.
IODA_DL struct ioda_string_ret_t* ioda_has_variables_list(const struct ioda_has_variables*);

/// \brief Check if a variable exists.
/// \param[in] has_vars is the base container of the variables.
/// \see ioda_group_vars to get the variable container from a group.
/// \param[in] name is the candidate variable name.
/// \param sz_name is strlen(name). Needed for Fortran compatability.
/// \returns 1 if the variable exists.
/// \returns 0 if the variable does not exist.
/// \returns -1 on failure, such as when a precondition is invalid.
/// \pre has_vars must be valid.
/// \pre name must be valid.
IODA_DL int ioda_has_variables_exists(const struct ioda_has_variables* has_vars, size_t sz_name,
                                      const char* name);

/// \brief Remove a variable.
/// \param[in] has_vars is the container of the variable.
/// \param[in] name is the variable name.
/// \param sz_name is strlen(name). Fortran compatability.
/// \returns true if the variable is successfully removed.
/// \returns false on error.
/// \pre has_vars must be valid.
/// \pre name must be a valid variable name. The variable must exist.
/// \pre The variable must not be opened (have a valid handle) elsewhere.
/// \post The variable no longer exists.
IODA_DL bool ioda_has_variables_remove(struct ioda_has_variables* has_vars, size_t sz_name,
                                       const char* name);

/// \brief Open (access) a variable by name.
/// \param[in] has_vars is the container of the variable.
/// \param[in] name is the variable name.
/// \param sz_name is strlen(name). Fortran compatability.
/// \returns A handle to the variable on success.
/// \returns NULL on error.
/// \pre has_vars must be valid.
/// \pre name must be a valid variable name. The variable must exist.
IODA_DL struct ioda_variable* ioda_has_variables_open(const struct ioda_has_variables* has_vars,
                                                      size_t sz_name, const char* name);

// ioda_has_variables_create_* functions

/*!
 * \defgroup ioda_has_variables_create ioda_has_variables_create
 * \brief Create a new variable.
 * \details This is documentation for a series of functions in C that attempt to emulate C++
 *   templates using macro magic. The template parameter SUFFIX is written into the
 *   function name. Ex:, to create an integer variable, call
 *      ```ioda_has_variables_create_int```.
 * \tparam SUFFIX is the type (int, long, int64_t) that is appended to this function name
 *   in the C interface.
 * \param has_vars[in] is the container of the variable.
 * \param name[in] is the name of the new variable. This name must not already exist.
 * \param sz_name is strlen(name). Fortran compatability.
 * \param n_dims is the dimensionality of the new variable.
 * \param dims is an array of dimension lengths of the new variable. The array must be
 *   of rank n_dims.
 * \param max_dims is an array of maximum dimension lengths of the new variable.
 *   The array must be of rank n_dims.
 * \see ioda_group_vars for a method to get a group's associated has_variables object.
 * \return the new variable on success.
 * \return NULL on failure.
 * \pre has_vars must be valid. The group associated with has_vars must still exist.
 * \pre name must be a valid name. No other object (variable OR group) with this name
 *   should exist at this group level.
 * \pre dims must be valid and have the rank of n_dims.
 * \post A variable of the specified name, type, and dimensions now exists.
 * @{
 */

/// \def IODA_HAS_VARIABLES_CREATE_TEMPLATE
/// \brief See @link ioda_has_variables_create ioda_has_variables_create @endlink
/// \see ioda_has_variables_create
#define IODA_HAS_VARIABLES_CREATE_TEMPLATE(funcnamestr, junk)                                      \
  IODA_DL struct ioda_variable* funcnamestr(struct ioda_has_variables* has_vars, size_t sz_name,   \
                                            const char* name, size_t n_dims, const long* dims,     \
                                            const long* max_dims,                                  \
                                            const struct ioda_variable_creation_parameters*        \
                                              params);  // NOLINT: cppcheck complains about long

C_TEMPLATE_FUNCTION_DECLARATION(ioda_has_variables_create, IODA_HAS_VARIABLES_CREATE_TEMPLATE);

/*! @}
 * @brief Class-like encapsulation of C has_variables functions.
 * @see c_ioda for an example.
 * @see use_c_ioda for an example.
 */
struct c_has_variables {
  void (*destruct)(struct ioda_has_variables*);
  struct ioda_string_ret_t* (*list)(const struct ioda_has_variables*);
  int (*exists)(const struct ioda_has_variables*, size_t, const char*);
  bool (*remove)(struct ioda_has_variables*, size_t, const char*);
  struct ioda_variable* (*open)(const struct ioda_has_variables*, size_t, const char*);

#define IODA_HAS_VARIABLES_CREATE_FUNC_TEMPLATE(shortnamestr, basenamestr)                         \
  struct ioda_variable* (*shortnamestr)(                                                           \
    struct ioda_has_variables*, size_t, const char*, size_t, const long*, const long*,             \
    const struct ioda_variable_creation_parameters*);  // NOLINT: cppcheck complains about long
  C_TEMPLATE_FUNCTION_DECLARATION_3(create, ioda_has_variables_create,
                                    IODA_HAS_VARIABLES_CREATE_FUNC_TEMPLATE);

  struct c_variable_creation_parameters VariableCreationParams;
};

#ifdef __cplusplus
}
#endif

/// @} // End Doxygen block
