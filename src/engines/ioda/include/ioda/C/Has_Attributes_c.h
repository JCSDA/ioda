#pragma once
/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \defgroup ioda_has_attributes Has_Attributes
 * \brief Provides the C-style interface for ioda::Has_Attributes objects.
 * \ingroup ioda_c_api
 *
 * @{
 * \file Has_Attributes_c.h
 * \brief @link ioda_has_attributes C bindings @endlink for ioda::Has_Attributes
 */
#include <stdbool.h>

#include "../defs.h"
#include "./c_binding_macros.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ioda_group;
struct ioda_has_attributes;
struct ioda_attribute;

/// \brief Deallocates a ioda_has_attributes_object.
/// \param has_atts is the object.
IODA_DL void ioda_has_attributes_destruct(struct ioda_has_attributes* has_atts);

/// \brief List the names of the attributes associated with an object.
IODA_DL struct ioda_string_ret_t* ioda_has_attributes_list(const struct ioda_has_attributes*);

/// \brief Check if an attribute exists.
/// \param[in] has_atts is the base container of the attributes.
/// \param[in] name is the candidate attribute name.
/// \param sz_name is strlen(name). Needed for Fortran compatability.
/// \returns 1 if the attribute exists.
/// \returns 0 if the attribute does not exist.
/// \returns -1 on failure, such as when a precondition is invalid.
/// \pre has_atts must be valid.
/// \pre name must be valid.
IODA_DL int ioda_has_attributes_exists(const struct ioda_has_attributes* has_atts, size_t sz_name,
                                       const char* name);

/// \brief Remove an attribute.
/// \param[in] has_atts is the container of the attribute.
/// \param[in] name is the attribute name.
/// \param sz_name is strlen(name). Fortran compatability.
/// \returns true if the attribute is successfully removed.
/// \returns false on error.
/// \pre has_atts must be valid.
/// \pre name must be a valid attribute name. The attribute must exist.
/// \pre The attribute must not be opened (have a valid handle) elsewhere.
/// \post The attribute no longer exists.
IODA_DL bool ioda_has_attributes_remove(struct ioda_has_attributes* has_atts, size_t sz_name,
                                        const char* name);

/// \brief Open (access) an attribute by name.
/// \param[in] has_atts is the container of the attribute.
/// \param[in] name is the attribute name.
/// \param sz_name is strlen(name). Fortran compatability.
/// \returns A handle to the attribute on success.
/// \returns NULL on error.
/// \pre has_atts must be valid.
/// \pre name must be a valid attribute name. The attribute must exist.
IODA_DL struct ioda_attribute* ioda_has_attributes_open(const struct ioda_has_attributes* has_atts,
                                                        size_t sz_name, const char* name);

/// \brief Rename an attribute.
/// \param[in] has_atts is the container of the attribute.
/// \param oldname is the current name of the attribute.
/// \param sz_oldname is strlen(oldname). Fortran compatability.
/// \param newname is the desired name of the attribute.
/// \param sz_newname is strlen(newname). Fortran compatability.
/// \returns True on success.
/// \returns False on error.
/// \pre has_atts must be valid.
/// \pre oldname must be a valid attribute name. The attribute must exist.
/// \pre newname must be a valid attribute name. An attribute with this name must
///   not already exist.
IODA_DL bool ioda_has_attributes_rename(struct ioda_has_attributes* has_atts, size_t sz_oldname,
                                        const char* oldname, size_t sz_newname,
                                        const char* newname);

// ioda_has_attributes_create_* functions
/*!
 * \defgroup ioda_has_attributes_create ioda_has_attributes_create
 * \brief Create a new attribute.
 * \details This is documentation for a series of functions in C that attempt to emulate C++
 * templates using macro magic. The template parameter SUFFIX is written into the function name.
 * Ex:, to create an integer attribute, call
 *      ```ioda_has_attributes_create_int```.
 * \tparam SUFFIX is the type (int, long, int64_t) that is appended to this function name in
 * the C interface.
 * \param has_atts[in] is the container of the attribute. Attributes can be attached to
 *   Groups or Variables.
 * \param name[in] is the name of the new attribute. This name must not already exist.
 * \param sz_name is strlen(name). Fortran compatability.
 * \param n_dims is the dimensionality of the new attribute.
 * \param dims is an array of dimension lengths of the new attribute. The
 *   array must be of rank n_dims.
 * \see ioda_group_atts for a method to get a group's associated has_attributes object.
 * \see ioda_variable_atts for a method to get a variable's associated has_attributes object.
 * \return the new attribute on success.
 * \return NULL on failure.
 * \pre has_atts must be valid. The variable or group associated with has_atts must still exist.
 * \pre name must be a valid name. No attribute with this name should exist in has_atts.
 * \pre dims must be valid and have the rank of n_dims.
 * \post An attribute of the specified name, type, and dimensions now exists.
 * @{
 */

// isA - int ioda_attribute_isa_char(const ioda_attribute*);
/// \def IODA_HAS_ATTRIBUTES_CREATE_TEMPLATE
/// \brief See @link ioda_has_attributes_create ioda_has_attributes_create @endlink
/// \see ioda_has_attributes_create

#define IODA_HAS_ATTRIBUTES_CREATE_TEMPLATE(funcnamestr, junk)                                     \
  IODA_DL struct ioda_attribute* funcnamestr(struct ioda_has_attributes* has_atts, size_t sz_name, \
                                             const char* name, size_t n_dims, const long* dims);

C_TEMPLATE_FUNCTION_DECLARATION(ioda_has_attributes_create, IODA_HAS_ATTRIBUTES_CREATE_TEMPLATE);

/*! @}
 * @brief Class-like encapsulation of C has_attributes functions.
 * @see c_ioda for an example.
 * @see use_c_ioda for an example.
 */

struct c_has_attributes {
  void (*destruct)(struct ioda_has_attributes*);
  struct ioda_string_ret_t* (*list)(const struct ioda_has_attributes*);
  int (*exists)(const struct ioda_has_attributes*, size_t, const char*);
  bool (*remove)(struct ioda_has_attributes*, size_t, const char*);
  struct ioda_attribute* (*open)(const struct ioda_has_attributes*, size_t, const char*);
#define IODA_HAS_ATTRIBUTES_CREATE_FUNC_TEMPLATE(shortnamestr, basenamestr)                        \
  struct ioda_attribute* (*shortnamestr)(struct ioda_has_attributes*, size_t, const char*, size_t, \
                                         const long*);  // NOLINT: cppcheck complains about long
  C_TEMPLATE_FUNCTION_DECLARATION_3(create, ioda_has_attributes_create,
                                    IODA_HAS_ATTRIBUTES_CREATE_FUNC_TEMPLATE);

  /// \note stdio.h on some platforms already defines rename!
  bool (*rename_att)(struct ioda_has_attributes*, size_t, const char*, size_t, const char*);
};

#ifdef __cplusplus
}
#endif

/// @} // End Doxygen block
