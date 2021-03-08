#pragma once
/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \defgroup ioda_variable_creation_parameters Variable Creation Parameters
 * \brief Provides the C-style interface for ioda::VariableCreationParameters.
 * \ingroup ioda_c_api
 *
 * @{
 * \file Variable_Creation_Parameters_c.h
 * \brief @link ioda_variable_creation_parameters C bindings @endlink for
 * ioda::VariableCreationParameters, used in ioda::Has_Variables::create.
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
struct ioda_has_variables;
struct ioda_variable;
struct ioda_variable_creation_parameters;

/// \brief Deallocates variable creation parameters.
/// \param params is the parameters object to be destructed.
IODA_DL void ioda_variable_creation_parameters_destruct(
  struct ioda_variable_creation_parameters* params);

/// \brief Allocates a new variable creation parameters instance.
/// \returns A new instance of ioda_variable_creation_parameters.
IODA_DL struct ioda_variable_creation_parameters* ioda_variable_creation_parameters_create();

/// \brief Make a copy of an existing variable creation parameters object.
/// \param[in] source is the object to be copied.
/// \returns a clone of source if source is valid.
/// \returns NULL if source is invalid.
/// \pre source should be valid.
IODA_DL struct ioda_variable_creation_parameters* ioda_variable_creation_parameters_clone(
  const struct ioda_variable_creation_parameters* source);

/// @name Set fill value
/// @{

/// \def IODA_VCP_FILL_TEMPLATE
/// \brief Sets a template for the fill value functions.
/// \details This is documentation for a series of functions in C that attempt to emulate C++
///   templates using macro magic. The template parameter SUFFIX is written into the function
///   name.
/// \tparam SUFFIX is the type (long, int64_t) that is appended to this function name in the C
///   interface.
/// \param[in] params is the parameters object.
/// \param data is the fill value, applied as a bit pattern (the type here does not strictly
///   need to match the variable's type).
#define IODA_VCP_FILL_TEMPLATE(funcnamestr, typ)                                                   \
  IODA_DL void funcnamestr(struct ioda_variable_creation_parameters* params, typ data);
C_TEMPLATE_FUNCTION_DEFINITION_NOSTR(ioda_variable_creation_parameters_setFillValue,
                                     IODA_VCP_FILL_TEMPLATE);

/// @}
/// @name Chunking
/// @{

/// \brief Set chunking options.
/// \param[in] params is the parameters object.
/// \param doChunking is a flag indicating whether chunking should be used.
/// \param Ndims is the dimensionality of chunks. Ignored if doChunking is false.
/// \param[in] chunks is a sequence of chunk lengths along each dimension. Ignored if doChunking
///   is false.
/// \note Chunking dimensionality must match the dimensionality of the variable to be created.
/// \note Chunk lengths must be nonzero, but this function accepts zero lengths because
///   later on these values might be "filled in" using hints from  dimension scales.
/// \pre params must be valid.
/// \pre chunks must have size Ndims.
IODA_DL void ioda_variable_creation_parameters_chunking(
  struct ioda_variable_creation_parameters* params, bool doChunking, size_t Ndims,
  const ptrdiff_t* chunks);

/// @}
/// @name Compression
/// @{

/// \brief Disable compression.
/// \param[in] params is the parameters object.
/// \pre params is valid.
IODA_DL void ioda_variable_creation_parameters_noCompress(
  struct ioda_variable_creation_parameters* params);

/// \brief Compress with GZIP.
/// \param[in] params is the parameters object.
/// \param level is the compression level [0-9]. Nine is the highest level, but it is also the
///   slowest. One is the lowest level. Zero denotes no compression, but the GZIP filter
///   is still turned on. To disable, call ioda_variable_creation_parameters_noCompress instead.
/// \pre params must be valid.
/// \pre level ranges from zero to nine, inclusive.
IODA_DL void ioda_variable_creation_parameters_compressWithGZIP(
  struct ioda_variable_creation_parameters* params, int level);

/// \brief Compress with SZIP.
/// \param[in] params is the parameters object.
/// \param PixelsPerBlock specifies the pixels per block.
/// \param options specified additional options for this filter.
/// \see VariableCreationParameters::compressWithSZIP for parameter meanings.
/// \pre params must be valid.
/// \pre PixelsPerBlock and options must be valid values according to the SZIP documentation.
IODA_DL void ioda_variable_creation_parameters_compressWithSZIP(
  struct ioda_variable_creation_parameters* params, unsigned PixelsPerBlock, unsigned options);

/// @}
/// @name Dimension scales
/// @{

/// \brief Have dimension scales been specified?
/// \param[in] params is the parameters object.
/// \returns true if yes, false if no or on error (invalid params).
/// \pre params must be valid.
IODA_DL bool ioda_variable_creation_parameters_hasSetDimScales(
  const struct ioda_variable_creation_parameters* params);

/// \brief Attach a dimension scale to a variable.
/// \param[in] params is the parameters object.
/// \param DimensionNumber denotes the dimension which you are adding a scale to.
///   Counts start at zero.
/// \param[in] scale is the dimension scale that will be attached.
/// \returns true on success.
///   Success implies that the parameters have added the scale to the internal list. It does not
///   imply that scale attachment will be successful when a variable is created using these
///   parameters.
/// \returns false on failure.
/// \pre params and scale must both be valid.
/// \pre DimensionNumber must be within the variable's intended dimensionality.
/// \pre scale must not already be attached at the same dimension number.
/// \pre scale must share the same backend instance with any new variable created
///   with these params.
IODA_DL bool ioda_variable_creation_parameters_attachDimensionScale(
  struct ioda_variable_creation_parameters* params, unsigned int DimensionNumber,
  const struct ioda_variable* scale);

/// \brief Convenience function to set a sequence of scales on a variable.
/// \param[in] params is the parameters object.
/// \param n_dims is the size of the dims array.
/// \param[in] dims is a sequence of dimension scales that will be attached to var.
///   dims[0] will be attached to var along dimension 0,
///   dims[1] will be attached to var along dimension 1, and so on.
/// \returns true on success. Success implies that the parameters have added the scale to the
///   internal list. It does not imply that scale attachment will be successful when a
///   variable is created using these parameters.
/// \returns false on failure.
/// \pre params must be valid.
/// \pre dims must be non-null, and each scale in dims must be valid and share the same backend
///   instance as every variable created using these parameters.
/// \pre The scales in dims should not already be attached to var at their expected places.
/// \pre n_dims must be less than or equal to the dimensionality of any new variable created
///   using these parameters.
IODA_DL bool ioda_variable_creation_parameters_setDimScale(
  struct ioda_variable_creation_parameters*, size_t n_dims, const struct ioda_variable** dims);

/// \brief Check if variables created using these creation params will act as dimension scales.
/// \param[in] params is the parameters object.
/// \returns 1 if yes.
/// \returns 0 if no.
/// \returns -1 on error.
/// \pre params must be valid.
IODA_DL int ioda_variable_creation_parameters_isDimensionScale(
  const struct ioda_variable_creation_parameters* params);

/// \brief Specify that variables created using these creation params will be dimension scales.
/// \param[in] params is the parameters object.
/// \param sz_name is strlen(dimensionScaleName). Fortran compatability.
/// \param[in] dimensionScaleName is the "name" of the dimension scale. This name does not
///   need to correspond to the variable's name, and acts as a convenience label when
///   reading data.
/// \returns true on success, which does not imply that the scale operation will be valid when
///   actually creating a variable using these pcreation parameters.
/// \returns false on failure.
/// \pre params must be valid.
/// \pre dimensionScaleName must be valid. If unused, it should be set to an empty string
///   and not NULL.
IODA_DL bool ioda_variable_creation_parameters_setIsDimensionScale(
  struct ioda_variable_creation_parameters*, size_t sz_name, const char* dimensionScaleName);

/**
 * \brief Get the intended name of the dimension scale.
 * \param[in] params is the parameters object.
 * \param[out] out is the output buffer that will hold the name of the dimension scale.
 *   This will always be a null-terminated string. If len_out is smaller than the length
 *   of the dimension scale's name, then out will be truncated to fit.
 * \param len_out is the length (size) of the output buffer, in bytes.
 * \returns The minimum size of an output buffer needed to fully read the scale name.
 *   Callers should check that the return value is less than len_out. If it is not, then
 *   the output buffer is too small and should be expanded. The output buffer is
 *   always at least one byte in size (the null byte).
 * \returns 0 if an error has occurred.
 * \pre params must be valid.
 * \pre out must be valid (not null!)
 **/
IODA_DL size_t ioda_variable_creation_parameters_getDimensionScaleName(
  const struct ioda_variable_creation_parameters* params, size_t len_out, char* out);

/// @}
/// @name Attributes
/// @todo Implement these!
/// @todo Attribute_Creator_Store should derive from Has_Attributes.

/*!
 * @brief Class-like encapsulation of C variable creation parameters functions.
 * @see c_ioda for an example.
 * @see use_c_ioda for an example.
 * @todo Add attributes!
 */
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
  bool (*setIsDimensionScale)(struct ioda_variable_creation_parameters*, size_t, const char*);
  size_t (*getDimensionScaleName)(const struct ioda_variable_creation_parameters*, size_t, char*);

  // Attributes TODO
};

#ifdef __cplusplus
}
#endif

/// @} // End Doxygen block
