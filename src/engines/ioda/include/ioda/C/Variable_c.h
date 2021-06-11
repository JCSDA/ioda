#pragma once
/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \defgroup ioda_variable Variables
 * \brief Provides the C-style interface for ioda::Variable objects.
 * \ingroup ioda_c_api
 *
 * @{
 * \file Variable_c.h
 * \brief @link ioda_variable C bindings @endlink for ioda::Variable
 */

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

/// \brief Deallocates an variable.
/// \param var is the variable to be destructed.
IODA_DL void ioda_variable_destruct(struct ioda_variable* var);

/// \brief Access a variable's attributes.
/// \see ioda::Group::atts.
/// \param[in] var is the variable whose attributes are being accessed.
/// \returns A handle to the Has_Attributes container on success (you must free after use).
/// \returns NULL on failure.
IODA_DL struct ioda_has_attributes* ioda_variable_atts(const struct ioda_variable* var);

/// \brief Gets an variable's dimensions.
/// \param[in] var is the variable.
/// \returns A dimension object that contains the variable's dimensions. Must be freed when no
///   longer used.
IODA_DL struct ioda_dimensions* ioda_variable_get_dimensions(const struct ioda_variable*);

/// \brief Resize a variable.
/// \param[in] var is the variable.
/// \param N is the dimensionality of newDims. Must match current dimensionality.
/// \param[in] newDims is a sequence of dimension sizes.
/// \returns true on success.
/// \returns false on failure.
/// \pre var must be valid.
/// \pre N and newDims must match the variable's current dimensionality. There is no way to add
///   more dimensions to an existing variable.
IODA_DL bool ioda_variable_resize(struct ioda_variable* var, size_t N, const long* newDims);

/// \brief Attach a dimension scale to a variable.
/// \param[in] var is the variable.
/// \param DimensionNumber denotes the dimension which you are adding a scale to.
///   Counts start at zero.
/// \param[in] scale is the dimension scale that will be attached.
/// \returns true on success.
/// \returns false on failure.
/// \pre var and scale must both be valid.
/// \pre DimensionNumber must be within the variable's dimensionality.
/// \pre scale must not already be attached at the same dimension number.
/// \pre var and scale must share the same backend instance.
IODA_DL bool ioda_variable_attachDimensionScale(struct ioda_variable* var,
                                                unsigned int DimensionNumber,
                                                const struct ioda_variable* scale);

/// \brief Detach a dimension scale from a variable.
/// \param[in] var is the variable.
/// \param DimensionNumber denotes the dimension of var which contains the scale that will be
///   detached.
/// \param[in] scale is the dimension scale that will be attached.
/// \returns true on success.
/// \returns false on failure.
/// \pre var and scale must both be valid.
/// \pre DimensionNumber must be within the variable's dimensionality.
/// \pre scale must already be attached at the specified dimension number.
/// \pre var and scale must share the same backend instance.
IODA_DL bool ioda_variable_detachDimensionScale(struct ioda_variable* var,
                                                unsigned int DimensionNumber,
                                                const struct ioda_variable* scale);

/// \brief Convenience function to set a sequence of scales on a variable.
/// \param[in] var is the variable.
/// \param n_dims is the size of the dims array.
/// \param[in] dims is a sequence of dimension scales that will be attached to var.
///   dims[0] will be attached to var along dimension 0,
///   dims[1] will be attached to var along dimension 1, and so on.
/// \returns true on success.
/// \returns false on failure.
/// \pre var must be valid.
/// \pre dims must be non-null, and each scale in dims must be valid and share the same backend
///   instance as var.
/// \pre The scales in dims should not already be attached to var at their expected places.
/// \pre n_dims must be less than or equal to the dimensionality of var.
IODA_DL bool ioda_variable_setDimScale(struct ioda_variable* var, size_t n_dims,
                                       const struct ioda_variable* const* dims);

/// \brief Check if a variable acts as a dimension scale.
/// \param[in] var is the variable.
/// \returns 1 if yes.
/// \returns 0 if no.
/// \returns -1 on error.
/// \pre var must be valid.
IODA_DL int ioda_variable_isDimensionScale(const struct ioda_variable* var);

/// \brief Convert a variable into a dimension scale.
/// \param[in] var is the variable that will become a scale.
/// \param sz_name is strlen(dimensionScaleName). Fortran compatability.
/// \param[in] dimensionScaleName is the "name" of the dimension scale. This name does
///   not need to correspond to the variable's name, and acts as a convenience label
///   when reading data.
/// \returns true on success.
/// \returns false on failure.
/// \pre var must be valid.
/// \pre dimensionScaleName must be valid. If unused, it should be set to an empty string and
///   not NULL.
IODA_DL bool ioda_variable_setIsDimensionScale(struct ioda_variable* var, size_t sz_name,
                                               const char* dimensionScaleName);

/**
 * \brief Get the name of the dimension scale.
 * \param[in] var is the dimension scale.
 * \param[out] out is the output buffer that will hold the name of the dimension scale.
 *   This will always be a null-terminated string. If len_out is smaller than the length
 *   of the dimension scale's name, then out will be truncated to fit.
 * \param len_out is the length (size) of the output buffer, in bytes.
 * \returns The minimum size of an output buffer needed to fully read the scale name.
 *   Callers should check that the return value is less than len_out. If it is not, then
 *   the output buffer is too small and should be expanded. The output buffer is
 *   always at least one byte in size (the null byte).
 * \returns 0 if an error has occurred.
 * \pre var must be valid.
 * \pre out must be valid (not null!)
 **/
IODA_DL size_t ioda_variable_getDimensionScaleName(const struct ioda_variable* var, size_t len_out,
                                                   char* out);

/**
 * \brief Is the variable "scale" attached as dimension "DimensionNumber" to variable "var"?
 * \param[in] var is the variable.
 * \param DimensionNumber is the dimension number. Numbering starts at zero.
 * \param[in] scale is the candidate dimension scale.
 * \returns 1 if attached.
 * \returns 0 if not attached.
 * \returns -1 on error.
 * \pre var and scale must both be valid and from the same backend engine instance.
 * \pre DimensionNumber must be less than the variable's dimensionality.
 **/
IODA_DL int ioda_variable_isDimensionScaleAttached(const struct ioda_variable* var,
                                                   unsigned int DimensionNumber,
                                                   const struct ioda_variable* scale);

/*!
 * \defgroup ioda_variable_isa ioda_variable_isa
 * \brief Checks a variable's type.
 * \details This is documentation for a series of functions in C that attempt to emulate C++
 *   templates using macro magic. The template parameter SUFFIX is written into the function
 *   name. Ex:, to check if a variable is an integer, call
 *   ```ioda_variable_isa_int```. To check if a variable is a float, try
 *   ```ioda_variable_isa_float```.
 * \tparam SUFFIX is the type (int, long, int64_t) that is appended
 *   to this function name in the C interface.
 * \param var is the variable.
 * \return an integer denoting yes (> 0), no (== 0), or failure (< 0).
 * @{
 */

// isA - int ioda_variable_isa_char(const ioda_variable*);
/// \def IODA_VARIABLE_ISA_TEMPLATE
/// \brief See @link ioda_variable_isa ioda_variable_isa @endlink
/// \see ioda_variable_isa
#define IODA_VARIABLE_ISA_TEMPLATE(funcnamestr, junk)                                              \
  IODA_DL int funcnamestr(const struct ioda_variable* var);
C_TEMPLATE_FUNCTION_DECLARATION(ioda_variable_isa, IODA_VARIABLE_ISA_TEMPLATE);

/*!
 * @}
 * \defgroup ioda_variable_write_full ioda_variable_write_full
 * \brief Write an entire variable.
 * \details This is documentation for a series of functions in C that attempt to emulate C++
 *   templates using macro magic. The template parameter SUFFIX is written into the function
 *   name. Ex:, to write an integer variable, call
 *   ```ioda_variable_write_full_int```. To write a float variable, try
 *   ```ioda_variable_write_full_float```.
 * \tparam SUFFIX is the type (int, long, int64_t) that is appended to this function name
 *   in the C interface.
 * \param var is the variable.
 * \return true on success, false on failure.
 *  @{
 */

// write_full - bool ioda_variable_write_full_char(ioda_variable*, size_t, const char*);
/// \def IODA_VARIABLE_WRITE_FULL_TEMPLATE
/// \brief See @link ioda_variable_write_full ioda_variable_write_full @endlink
/// \see ioda_variable_write_full
#define IODA_VARIABLE_WRITE_FULL_TEMPLATE(funcnamestr, Type)                                       \
  IODA_DL bool funcnamestr(struct ioda_variable* var, size_t sz, const Type* vals);
C_TEMPLATE_FUNCTION_DEFINITION_NOSTR(ioda_variable_write_full, IODA_VARIABLE_WRITE_FULL_TEMPLATE);

/// \brief Write a sequence of strings to a variable.
/// \param[in] var is the variable.
/// \param sz is the number of strings to write. For ioda_variable_write_full_str, this
///   must match var's current number of elements.
/// \param[in] vals is a sequence of sz NULL-terminated strings.
/// \returns true on success.
/// \returns false on failure.
/// \pre var must be a valid string-typed variable.
/// \pre sz must equal the variable's current number of elements.
/// \pre vals must be valid and have at least length sz.
/// \pre Each val in vals must be a NULL-terminated string. Each val must not be NULL itself.
IODA_DL bool ioda_variable_write_full_str(struct ioda_variable* var, size_t sz,
                                          const char* const* vals);

/*!
 * @}
 * \defgroup ioda_variable_read_full ioda_variable_read_full
 * \brief Read an entire variable.
 * \details This is documentation for a series of functions in C that attempt to emulate C++
 *   templates using macro magic. The template parameter SUFFIX is written into the function
 *   name. Ex:, to read an integer variable, call
 *   ```ioda_variable_read_full_int```. To read a float variable, try
 *   ```ioda_variable_read_full_float```.
 * \tparam SUFFIX is the type (int, long, int64_t) that is appended to this function name
 *   in the C interface.
 * \param[in] var is the variable.
 * \param sz is the size of the output buffer vals. Must match the number of elements in var.
 * \param[out] vals is the output buffer.
 * \return true on success, false on failure.
 * \pre sz must match the current number of elements in the variable.
 * \pre var and vals must be valid.
 * @{
 */

// read_full - void ioda_variable_read_full_char(const ioda_variable*, size_t, char*);
/// \def IODA_VARIABLE_READ_FULL_TEMPLATE
/// \brief See @link ioda_variable_read_full ioda_variable_read_full @endlink
/// \see ioda_variable_read_full
#define IODA_VARIABLE_READ_FULL_TEMPLATE(funcnamestr, Type)                                        \
  IODA_DL bool funcnamestr(const struct ioda_variable* var, size_t sz, Type* vals);
C_TEMPLATE_FUNCTION_DEFINITION_NOSTR(ioda_variable_read_full, IODA_VARIABLE_READ_FULL_TEMPLATE);

/// \brief Read strings from a variable.
/// \param[in] var is the variable.
/// \returns a sequence of strings. These should be freed by the caller.
/// \returns NULL on failure.
/// \pre var must be valid.
/// \pre var must contain strings. Other types cannot be transparently converted.
IODA_DL struct ioda_string_ret_t* ioda_variable_read_full_str(const struct ioda_variable* var);

/*! @}
 * @brief Class-like encapsulation of C variable functions.
 * @see c_ioda for an example.
 * @see use_c_ioda for an example.
 */
struct c_variable {
  void (*destruct)(struct ioda_variable*);
  struct ioda_has_attributes* (*getAtts)(const struct ioda_variable*);
  struct ioda_dimensions* (*getDimensions)(const struct ioda_variable*);
  bool (*resize)(struct ioda_variable*, size_t, const long*);
  bool (*attachDimensionScale)(struct ioda_variable*, unsigned int, const struct ioda_variable*);
  bool (*detachDimensionScale)(struct ioda_variable*, unsigned int, const struct ioda_variable*);
  bool (*setDimScale)(struct ioda_variable*, size_t, const struct ioda_variable* const*);
  int (*isDimensionScale)(const struct ioda_variable*);
  bool (*setIsDimensionScale)(struct ioda_variable*, size_t, const char*);
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

/// @} // End Doxygen block
