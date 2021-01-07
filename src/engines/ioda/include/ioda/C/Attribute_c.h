#pragma once
/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// \file Attribute_c.h
/// \brief C bindings for ioda::Attribute

#include <stdbool.h>

#include "../defs.h"
#include "./String_c.h"
#include "./c_binding_macros.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ioda_attribute;
struct ioda_dimensions;

IODA_DL void ioda_attribute_destruct(struct ioda_attribute* att);
IODA_DL struct ioda_dimensions* ioda_attribute_get_dimensions(const struct ioda_attribute* att);

// isA - int ioda_attribute_isa_char(const ioda_attribute*);
#define IODA_ATTRIBUTE_ISA_TEMPLATE(funcnamestr, junk) \
  IODA_DL int funcnamestr(const struct ioda_attribute* att);
C_TEMPLATE_FUNCTION_DECLARATION(ioda_attribute_isa, IODA_ATTRIBUTE_ISA_TEMPLATE);

// write - bool ioda_attribute_write_char(ioda_attribute*, size_t, const char*);
#define IODA_ATTRIBUTE_WRITE_TEMPLATE(funcnamestr, Type) \
  IODA_DL bool funcnamestr(struct ioda_attribute* att, size_t sz, const Type* vals);
C_TEMPLATE_FUNCTION_DEFINITION_NOSTR(ioda_attribute_write, IODA_ATTRIBUTE_WRITE_TEMPLATE);
IODA_DL bool ioda_attribute_write_str(struct ioda_attribute* att, size_t sz, const char* const* vals);

// read - void ioda_attribute_read_char(const ioda_attribute*, size_t, char*);
#define IODA_ATTRIBUTE_READ_TEMPLATE(funcnamestr, Type) \
  IODA_DL bool funcnamestr(const struct ioda_attribute* att, size_t sz, Type* vals);
C_TEMPLATE_FUNCTION_DEFINITION_NOSTR(ioda_attribute_read, IODA_ATTRIBUTE_READ_TEMPLATE);
IODA_DL struct ioda_string_ret_t* ioda_attribute_read_str(const struct ioda_attribute* att);

struct c_attribute {
  void (*destruct)(struct ioda_attribute*);
  struct ioda_dimensions* (*getDimensions)(const struct ioda_attribute*);

// int isA_char(const ioda_attribute*);
#define IODA_ATTRIBUTE_ISA_FUNC_TEMPLATE(shortnamestr, basenamestr) \
  int (*shortnamestr)(const struct ioda_attribute*);
  C_TEMPLATE_FUNCTION_DECLARATION_3(isA, ioda_attribute_isa, IODA_ATTRIBUTE_ISA_FUNC_TEMPLATE);

// void write_char(ioda_attribute*, size_t, const Type*);
#define IODA_ATTRIBUTE_WRITE_FUNC_TEMPLATE(shortnamestr, basenamestr, Type) \
  bool (*shortnamestr)(struct ioda_attribute*, size_t, const Type*);
  C_TEMPLATE_FUNCTION_DECLARATION_4_NOSTR(write, ioda_attribute_write, IODA_ATTRIBUTE_WRITE_FUNC_TEMPLATE);
  bool (*write_str)(struct ioda_attribute*, size_t, const char* const*);

// bool read_char(const ioda_attribute*, size_t, char*);
#define IODA_ATTRIBUTE_READ_FUNC_TEMPLATE(shortnamestr, basenamestr, Type) \
  bool (*shortnamestr)(const struct ioda_attribute*, size_t, Type*);
  C_TEMPLATE_FUNCTION_DECLARATION_4_NOSTR(read, ioda_attribute_read, IODA_ATTRIBUTE_READ_FUNC_TEMPLATE);
  struct ioda_string_ret_t* (*read_str)(const struct ioda_attribute*);
};

#ifdef __cplusplus
}
#endif
