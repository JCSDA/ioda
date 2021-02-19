#pragma once
/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// \file Variable_c.h
/// \brief C bindings for ioda::Variable

#include <stdbool.h>

#include "../defs.h"
#include "./Has_Attributes_c.h"
#include "./String_c.h"
#include "./c_binding_macros.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ioda_has_attributes;
struct ioda_variable;
struct ioda_dimensions;
struct c_has_attributes;

IODA_DL void ioda_variable_destruct(struct ioda_variable*);
IODA_DL struct ioda_has_attributes* ioda_variable_atts(const struct ioda_variable*);
IODA_DL struct ioda_dimensions* ioda_variable_get_dimensions(const struct ioda_variable*);

IODA_DL bool ioda_variable_resize(struct ioda_variable*, size_t N, const long* newDims);

IODA_DL bool ioda_variable_attachDimensionScale(struct ioda_variable*, unsigned int DimensionNumber,
                                                const struct ioda_variable* scale);
IODA_DL bool ioda_variable_detachDimensionScale(struct ioda_variable*, unsigned int DimensionNumber,
                                                const struct ioda_variable* scale);
IODA_DL bool ioda_variable_setDimScale(struct ioda_variable*, size_t n_dims,
                                       const struct ioda_variable* const* dims);
IODA_DL int ioda_variable_isDimensionScale(const struct ioda_variable*);
IODA_DL bool ioda_variable_setIsDimensionScale(struct ioda_variable*,
                                               const char* dimensionScaleName);

/** \brief Get the name of the dimension scale.
 * \param var is the dimension scale.
 * \param out is the output buffer that will hold the name of the dimension scale. This will
 *    always be a null-terminated string. If len_out is smaller than the length of the dimension
 *    scale's name, then out will be truncated to fit.
 * \param len_out is the length (size) of the output buffer, in bytes.
 * \returns The minimum size of an output buffer needed to fully read the scale name.
 *   Callers should check that the return value is less than len_out. If it is not, then
 *   the output buffer is too small and should be expanded. The output buffer is
 *   always at least one byte in size (the null byte). If the return valuse is zero,
 *   then an error has occurred.
 * \throws If any parameter is null or out-of-bounds. Returns 0 in this case.
 **/
IODA_DL size_t ioda_variable_getDimensionScaleName(const struct ioda_variable* var, size_t len_out,
                                                   char* out);

/** \brief Is the variable "scale" attached as dimension "DimensionNumber" to variable "var"?
 * \param var is the variable.
 * \param DimensionNumber is the dimension number.
 * \param scale is the candidate dimension scale.
 * \throws If any parameter is null or out-of-bounds. Causes application termination.
 * \returns 1 if attached, and 0 if not attached, and < 0 on error.
 **/
IODA_DL int ioda_variable_isDimensionScaleAttached(const struct ioda_variable* var,
                                                   unsigned int DimensionNumber,
                                                   const struct ioda_variable* scale);

// isA - int ioda_attribute_isa_char(const ioda_attribute*);
#define IODA_VARIABLE_ISA_TEMPLATE(funcnamestr, junk)                                              \
  IODA_DL int funcnamestr(const struct ioda_variable* att);
C_TEMPLATE_FUNCTION_DECLARATION(ioda_variable_isa, IODA_VARIABLE_ISA_TEMPLATE);

// write - bool ioda_variable_write_full_char(ioda_variable*, size_t, const char*);
#define IODA_VARIABLE_WRITE_FULL_TEMPLATE(funcnamestr, Type)                                       \
  IODA_DL bool funcnamestr(struct ioda_variable* var, size_t sz, const Type* vals);
C_TEMPLATE_FUNCTION_DEFINITION_NOSTR(ioda_variable_write_full, IODA_VARIABLE_WRITE_FULL_TEMPLATE);
IODA_DL bool ioda_variable_write_full_str(struct ioda_variable* var, size_t sz,
                                          const char* const* vals);

// read - void ioda_variable_read_full_char(const ioda_variable*, size_t, char*);
#define IODA_VARIABLE_READ_FULL_TEMPLATE(funcnamestr, Type)                                        \
  IODA_DL bool funcnamestr(const struct ioda_variable* att, size_t sz, Type* vals);
C_TEMPLATE_FUNCTION_DEFINITION_NOSTR(ioda_variable_read_full, IODA_VARIABLE_READ_FULL_TEMPLATE);
IODA_DL struct ioda_string_ret_t* ioda_variable_read_full_str(const struct ioda_variable* var);

struct c_variable {
  void (*destruct)(struct ioda_variable*);
  struct ioda_has_attributes* (*getAtts)(const struct ioda_variable*);
  struct ioda_dimensions* (*getDimensions)(const struct ioda_variable*);
  bool (*resize)(struct ioda_variable*, size_t, const long*);
  bool (*attachDimensionScale)(struct ioda_variable*, unsigned int, const struct ioda_variable*);
  bool (*detachDimensionScale)(struct ioda_variable*, unsigned int, const struct ioda_variable*);
  bool (*setDimScale)(struct ioda_variable*, size_t, const struct ioda_variable* const*);
  int (*isDimensionScale)(const struct ioda_variable*);
  bool (*setIsDimensionScale)(struct ioda_variable*, const char*);
  size_t (*getDimensionScaleName)(const struct ioda_variable*, size_t, char*);
  int (*isDimensionScaleAttached)(const struct ioda_variable*, unsigned int,
                                  const struct ioda_variable*);

  // bool isA_char(const ioda_attribute*);
#define IODA_VARIABLE_ISA_FUNC_TEMPLATE(shortnamestr, basenamestr)                                 \
  int (*shortnamestr)(const struct ioda_variable*);
  C_TEMPLATE_FUNCTION_DECLARATION_3(isA, ioda_variable_isa, IODA_VARIABLE_ISA_FUNC_TEMPLATE);

  // void write_full_char(ioda_variable*, size_t, const Type*);
#define IODA_VARIABLE_WRITE_FULL_FUNC_TEMPLATE(shortnamestr, basenamestr, Type)                    \
  bool (*shortnamestr)(struct ioda_variable*, size_t, const Type*);
  C_TEMPLATE_FUNCTION_DECLARATION_4_NOSTR(write_full, ioda_variable_write_full,
                                          IODA_VARIABLE_WRITE_FULL_FUNC_TEMPLATE);
  bool (*write_full_str)(struct ioda_variable*, size_t, const char* const*);

  // void read_full_char(const ioda_variable*, size_t, char*);
#define IODA_VARIABLE_READ_FULL_FUNC_TEMPLATE(shortnamestr, basenamestr, Type)                     \
  bool (*shortnamestr)(const struct ioda_variable*, size_t, Type*);
  C_TEMPLATE_FUNCTION_DECLARATION_4_NOSTR(read_full, ioda_variable_read_full,
                                          IODA_VARIABLE_READ_FULL_FUNC_TEMPLATE);
  struct ioda_string_ret_t* (*read_full_str)(const struct ioda_variable*);

  struct c_has_attributes atts;
};

#ifdef __cplusplus
}
#endif
