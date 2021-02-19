#pragma once
/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// \file Variable_Creation_Parameters_c.h
/// \brief C bindings for ioda::VariableCreationParameters, used in ioda::Has_Variables::create.
#include <stdbool.h>

#include "../defs.h"
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

IODA_DL void ioda_variable_creation_parameters_destruct(
  struct ioda_variable_creation_parameters* params);

IODA_DL struct ioda_variable_creation_parameters* ioda_variable_creation_parameters_create();
IODA_DL struct ioda_variable_creation_parameters* ioda_variable_creation_parameters_clone(
  const struct ioda_variable_creation_parameters*);

// Set fill value
// setFillValue - void
// ioda_variable_creation_parameters_setFillValue_double(ioda_variable_creation_parameters*,
// double);
#define IODA_VCP_FILL_TEMPLATE(funcnamestr, typ)                                                   \
  IODA_DL void funcnamestr(struct ioda_variable_creation_parameters*, typ);
C_TEMPLATE_FUNCTION_DEFINITION_NOSTR(ioda_variable_creation_parameters_setFillValue,
                                     IODA_VCP_FILL_TEMPLATE);

// Set chunking
IODA_DL void ioda_variable_creation_parameters_chunking(struct ioda_variable_creation_parameters*,
                                                        bool doChunking, size_t Ndims,
                                                        const ptrdiff_t* chunks);

// Compression
IODA_DL void ioda_variable_creation_parameters_noCompress(
  struct ioda_variable_creation_parameters*);
IODA_DL void ioda_variable_creation_parameters_compressWithGZIP(
  struct ioda_variable_creation_parameters*, int level);
IODA_DL void ioda_variable_creation_parameters_compressWithSZIP(
  struct ioda_variable_creation_parameters*, unsigned PixelsPerBlock, unsigned options);

// Dimension scales
IODA_DL bool ioda_variable_creation_parameters_hasSetDimScales(
  const struct ioda_variable_creation_parameters*);
IODA_DL bool ioda_variable_creation_parameters_attachDimensionScale(
  struct ioda_variable_creation_parameters*, unsigned int DimensionNumber,
  const struct ioda_variable* scale);
IODA_DL bool ioda_variable_creation_parameters_setDimScale(
  struct ioda_variable_creation_parameters*, size_t n_dims, const struct ioda_variable** dims);
IODA_DL int ioda_variable_creation_parameters_isDimensionScale(
  const struct ioda_variable_creation_parameters*);
IODA_DL bool ioda_variable_creation_parameters_setIsDimensionScale(
  struct ioda_variable_creation_parameters*, const char* dimensionScaleName);
IODA_DL size_t ioda_variable_creation_parameters_getDimensionScaleName(
  const struct ioda_variable_creation_parameters* var, size_t len_out, char* out);

// Attributes
// TODO(ryan): Implement this.
/// \todo Attribute_Creator_Store should derive from Has_Attributes.

struct c_variable_creation_parameters {
  void (*destruct)(struct ioda_variable_creation_parameters*);
  struct ioda_variable_creation_parameters* (*create)();
  struct ioda_variable_creation_parameters* (*clone)(
    const struct ioda_variable_creation_parameters*);

  // Fill values
#define IODA_VCP_FILL_TEMPLATE2(funcnamestr, typ)                                                  \
  void (*funcnamestr)(struct ioda_variable_creation_parameters*, typ);
  C_TEMPLATE_FUNCTION_DEFINITION_NOSTR(setFillValue, IODA_VCP_FILL_TEMPLATE2);

  void (*chunking)(struct ioda_variable_creation_parameters*, bool, size_t, const ptrdiff_t*);
  void (*noCompress)(struct ioda_variable_creation_parameters*);
  void (*compressWithGZIP)(struct ioda_variable_creation_parameters*, int);
  void (*compressWithSZIP)(struct ioda_variable_creation_parameters*, unsigned, unsigned);
  bool (*hasSetDimScales)(const struct ioda_variable_creation_parameters*);
  bool (*attachDimensionScale)(struct ioda_variable_creation_parameters*, unsigned int,
                               const struct ioda_variable*);
  bool (*setDimScale)(struct ioda_variable_creation_parameters*, size_t,
                      const struct ioda_variable**);
  int (*isDimensionScale)(const struct ioda_variable_creation_parameters*);
  bool (*setIsDimensionScale)(struct ioda_variable_creation_parameters*, const char*);
  size_t (*getDimensionScaleName)(const struct ioda_variable_creation_parameters*, size_t, char*);

  // Attributes TODO
};

#ifdef __cplusplus
}
#endif
