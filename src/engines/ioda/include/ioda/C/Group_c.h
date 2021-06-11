#pragma once
/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \defgroup ioda_group Groups
 * \brief Provides the C-style interface for ioda::Group objects.
 * \ingroup ioda_c_api
 *
 * @{
 * \file Group_c.h
 * \brief @link ioda_group C bindings @endlink for ioda::Group
 */
#include "../defs.h"
#include "Has_Attributes_c.h"
#include "Has_Variables_c.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ioda_group;
struct ioda_has_attributes;
struct ioda_has_variables;
struct ioda_string_ret_t;
struct c_has_attributes;
struct c_has_variables;

/// \brief Frees a ioda_group.
/// \param grp is the group to free.
/// \pre grp must be a valid group returned by a ioda_group_* call or an engine invocation.
IODA_DL void ioda_group_destruct(struct ioda_group* grp);

/// \brief Lists all one-level child groups.
/// \see String_c.h.
/// \param grp is the group. Must be valid.
/// \returns ioda_string_ret_t, which is an array of strings that should be freed after use by
///   ioda_string_ret_t_destruct (c_ioda.Strings.destruct).
IODA_DL struct ioda_string_ret_t* ioda_group_list(const struct ioda_group* grp);

/// \brief Check if a group exists.
/// \see ioda::Group::exists
/// \param[in] base is the base group.
/// \param child_sz is the size of the child string, as strlen(child). Explicitly specified for
///   Fortran bindings.
/// \param[in] child is the name of the group whose existence is tested.
/// \returns 1 if the group exists.
/// \returns 0 if the group does not exist.
/// \returns -1 on error, such as if base or child is NULL, or if there is a missing
///   intermediary group between base and child.
IODA_DL int ioda_group_exists(const struct ioda_group* base, size_t child_sz, const char* child);

/// \brief Create a group.
/// \see ioda::Group::create.
/// \param[in] base is the base group.
/// \param[in] name is the name of the new group.
/// \param sz is the length of the group name (as in strlen(name)). Explicitly specified
///   for Fortran bindings.
/// \returns A handle to the new group on success. The handle should be freed after use by
///   ioda_group_destruct (c_ioda.Groups.destruct).
/// \returns NULL on failure.
/// \pre base must be valid. name must be non-null.
/// \post The group will now exist, and the returned on handle must be freed by caller
///   after use. Upon failure, base is unchanged.
IODA_DL struct ioda_group* ioda_group_create(struct ioda_group* base, size_t sz, const char* name);

/// \brief Open a group.
/// \see ioda::Group::open.
/// \param base is the base group
/// \param name is the name of the group to be opened.
/// \param sz is the length of the group name, as strlen(name). Explicitly specified
///   for Fortran bindings.
/// \returns A handle to the group on success, which must be freed after use.
/// \returns NULL on failure.
/// \pre base must be valid. name must be non-null.
IODA_DL struct ioda_group* ioda_group_open(const struct ioda_group* base, size_t sz,
                                           const char* name);

/// \brief Access a group's attributes.
/// \see ioda::Group::atts.
/// \param[in] grp is the group whose attributes are being accessed.
/// \returns A handle to the Has_Attributes container on success (you must free after use).
/// \returns NULL on failure.
IODA_DL struct ioda_has_attributes* ioda_group_atts(const struct ioda_group* grp);

/// \brief Access a group's variables.
/// \see ioda::Group::vars.
/// \param[in] grp is the group whose variables are being accessed.
/// \returns A handle to the Has_Variables container on success (free after use).
/// \returns NULL on failure.
IODA_DL struct ioda_has_variables* ioda_group_vars(const struct ioda_group* grp);

/// \brief Spiffy C++-like container of function pointers for group methods.
/// \see use_c_ioda.
struct c_ioda_group {
  void (*destruct)(struct ioda_group*);
  struct ioda_string_ret_t* (*list)(const struct ioda_group*);
  int (*exists)(const struct ioda_group*, size_t, const char*);
  struct ioda_group* (*create)(struct ioda_group*, size_t, const char*);
  struct ioda_group* (*open)(const struct ioda_group*, size_t, const char*);
  struct ioda_has_attributes* (*getAtts)(const struct ioda_group*);
  struct ioda_has_variables* (*getVars)(const struct ioda_group*);

  struct c_has_attributes atts;
  struct c_has_variables vars;
};

#ifdef __cplusplus
}
#endif

/// @} // End Doxygen block
