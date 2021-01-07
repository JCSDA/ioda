#pragma once
/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// \file Has_Attributes_c.h
/// \brief C bindings for ioda::Has_Attributes
#include <stdbool.h>

#include "../defs.h"
#include "./c_binding_macros.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ioda_group;
struct ioda_has_attributes;
struct ioda_attribute;

IODA_DL void ioda_has_attributes_destruct(struct ioda_has_attributes* has_atts);
IODA_DL struct ioda_string_ret_t* ioda_has_attributes_list(const struct ioda_has_attributes*);
/// \returns <0 on error, ==0 if does not exist, >0 if exists.
IODA_DL int ioda_has_attributes_exists(const struct ioda_has_attributes* has_atts, const char* name);
IODA_DL bool ioda_has_attributes_remove(struct ioda_has_attributes* has_atts, const char* name);
IODA_DL struct ioda_attribute* ioda_has_attributes_open(const struct ioda_has_attributes* has_atts,
                                                        const char* name);
IODA_DL bool ioda_has_attributes_rename(struct ioda_has_attributes*, const char* oldname,
                                        const char* newname);

// ioda_has_attributes_create_* functions

#define IODA_HAS_ATTRIBUTES_CREATE_TEMPLATE(funcnamestr, junk)                                       \
  IODA_DL struct ioda_attribute* funcnamestr(struct ioda_has_attributes* has_atts, const char* name, \
                                             size_t n_dims, const long* dims);

C_TEMPLATE_FUNCTION_DECLARATION(ioda_has_attributes_create, IODA_HAS_ATTRIBUTES_CREATE_TEMPLATE);

struct c_has_attributes {
  void (*destruct)(struct ioda_has_attributes*);
  struct ioda_string_ret_t* (*list)(const struct ioda_has_attributes*);
  int (*exists)(const struct ioda_has_attributes*, const char*);
  bool (*remove)(struct ioda_has_attributes*, const char*);
  struct ioda_attribute* (*open)(const struct ioda_has_attributes*, const char*);
#define IODA_HAS_ATTRIBUTES_CREATE_FUNC_TEMPLATE(shortnamestr, basenamestr)                \
  struct ioda_attribute* (*shortnamestr)(struct ioda_has_attributes*, const char*, size_t, \
                                         const long*);  // NOLINT: cppcheck complains about long
  C_TEMPLATE_FUNCTION_DECLARATION_3(create, ioda_has_attributes_create,
                                    IODA_HAS_ATTRIBUTES_CREATE_FUNC_TEMPLATE);

  /// \note stdio.h on some platforms already defines rename!
  bool (*rename_att)(struct ioda_has_attributes*, const char*, const char*);
};

#ifdef __cplusplus
}
#endif
