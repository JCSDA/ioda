#pragma once
/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// \file Group_c.h
/// \brief C bindings for ioda::Group
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

/// Frees a ioda_group.
IODA_DL void ioda_group_destruct(struct ioda_group*);
/// Lists all one-level child groups.
/// \see String_c.h.
/// \returns ioda_string_ret_t, which is an array of strings that should be freed after use by
/// ioda_string_ret_t_destruct (c_ioda.Strings.destruct).
IODA_DL struct ioda_string_ret_t* ioda_group_list(const struct ioda_group*);
/// \brief Check if a group exists.
/// \see ioda::Group::exists
/// \param base is the base group.
/// \param child is the group whose existence is tested.
/// \returns > 0 if the group exists, ==0 if the group does not exist, and < 0 on error.
/// \throws <0 if base or child is NULL, or if there is a missing intermediary group between
///   base and child.
IODA_DL int ioda_group_exists(const struct ioda_group* base, const char* child);
/// \brief Create a group.
/// \see ioda::Group::create.
/// \returns A handle to the new group on success. The handle should be freed after use by
/// ioda_group_destruct (c_ioda.Groups.destruct). \returns NULL on failure.
IODA_DL struct ioda_group* ioda_group_create(struct ioda_group*, const char*);
/// \brief Open a group.
/// \see ioda::Group::open.
/// \returns A handle to the group on success, which must be freed after use.
/// \returns NULL on failure.
IODA_DL struct ioda_group* ioda_group_open(const struct ioda_group*, const char*);
/// \brief Access a group's attributes.
/// \see ioda::Group::atts.
/// \returns A handle to the Has_Attributes container on success (free after use).
/// \returns NULL on failure.
IODA_DL struct ioda_has_attributes* ioda_group_atts(const struct ioda_group*);
/// \brief Access a group's variables.
/// \see ioda::Group::vars.
/// \returns A handle to the Has_Variables container on success (free after use).
/// \returns NULL on failure.
IODA_DL struct ioda_has_variables* ioda_group_vars(const struct ioda_group*);

/// \brief Spiffy C++-like container of function pointers for group methods.
/// \see use_c_ioda.
struct c_ioda_group {
  void (*destruct)(struct ioda_group*);
  struct ioda_string_ret_t* (*list)(const struct ioda_group*);
  int (*exists)(const struct ioda_group*, const char*);
  struct ioda_group* (*create)(struct ioda_group*, const char*);
  struct ioda_group* (*open)(const struct ioda_group*, const char*);
  struct ioda_has_attributes* (*getAtts)(const struct ioda_group*);
  struct ioda_has_variables* (*getVars)(const struct ioda_group*);

  struct c_has_attributes atts;
  struct c_has_variables vars;
};

#ifdef __cplusplus
}
#endif
