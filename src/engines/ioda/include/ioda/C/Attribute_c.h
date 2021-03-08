#pragma once
/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \defgroup ioda_attribute Attributes
 * \brief Provides the C-style interface for ioda::Attribute objects.
 * \ingroup ioda_c_api
 *
 * @{
 * \file Attribute_c.h
 * \brief @link ioda_attribute C bindings @endlink for ioda::Attribute
 */

#include <stdbool.h>

#include "../defs.h"
#include "./String_c.h"
#include "./c_binding_macros.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ioda_attribute;
struct ioda_dimensions;

/// \brief Deallocates an attribute.
/// \param att is the attribute to be destructed.
IODA_DL void ioda_attribute_destruct(struct ioda_attribute* att);

/// \brief Gets an attribute's dimensions.
/// \param att is the attribute.
/// \returns A dimension object that contains the attribute's dimensions. Must be freed when no
///   longer used.
IODA_DL struct ioda_dimensions* ioda_attribute_get_dimensions(const struct ioda_attribute* att);

/*! \defgroup ioda_attribute_isa ioda_attribute_isa
 *  \brief Checks an attribute's type.
 *  \details This is documentation for a series of functions in C that attempt to emulate C++
 * templates using macro magic. The template parameter SUFFIX is written into the function name.
 * Ex:, to check if an attribute is an integer, call
 *      ```ioda_attribute_isa_int```. To check if an attribute is a float, try
 * ```ioda_attribute_isa_float```.
 * \tparam SUFFIX is the type (int, long, int64_t) that is appended
 *   to this function name in the C interface.
 * \param att is the attribute.
 * \return an integer denoting yes (> 0), no (== 0), or failure (< 0).
 *  @{
 */

// isA - int ioda_attribute_isa_char(const ioda_attribute*);
/// \def IODA_ATTRIBUTE_ISA_TEMPLATE
/// \brief See @link ioda_attribute_isa ioda_attribute_isa @endlink
/// \see ioda_attribute_isa
#define IODA_ATTRIBUTE_ISA_TEMPLATE(funcnamestr, junk)                                             \
  IODA_DL int funcnamestr(const struct ioda_attribute* att);
C_TEMPLATE_FUNCTION_DECLARATION(ioda_attribute_isa, IODA_ATTRIBUTE_ISA_TEMPLATE);

/*! @}
 * \defgroup ioda_attribute_write ioda_attribute_write
 * \brief Write data to an attribute.
 * \details This is documentation for a series of functions in C that attempt to emulate C++
 * templates using macro magic. The template parameter SUFFIX is written into the function name.
 * Ex:, to write character data to an attribute, call
 *      ```ioda_attribute_write_char```. To write a string, try ```ioda_attribute_write_str```.
 * \tparam SUFFIX is the type (int, long, int64_t) that is appended to this function name
 *   in the C interface.
 * \param att is the attribute.
 * \param sz is the size (in elements) of the data to write.
 * \param vals is the data to write.
 * \return true on success, false on failure.
 * @{
 */

// write - bool ioda_attribute_write_char(ioda_attribute*, size_t, const char*);
/// \def IODA_ATTRIBUTE_WRITE_TEMPLATE
/// \brief See @link ioda_attribute_write ioda_attribute_write @endlink
/// \see ioda_attribute_write

#define IODA_ATTRIBUTE_WRITE_TEMPLATE(funcnamestr, Type)                                           \
  IODA_DL bool funcnamestr(struct ioda_attribute* att, size_t sz, const Type* vals);
C_TEMPLATE_FUNCTION_DEFINITION_NOSTR(ioda_attribute_write, IODA_ATTRIBUTE_WRITE_TEMPLATE);
IODA_DL bool ioda_attribute_write_str(struct ioda_attribute* att, size_t sz,
                                      const char* const* vals);

/*! @}
 * \defgroup ioda_attribute_read ioda_attribute_read
 * \brief Read data from an attribute.
 * \details This is documentation for a series of functions in C that attempt to emulate C++
 *   templates using macro magic. The template parameter SUFFIX is appended into the
 *   function name. Ex:, to read character data from an attribute, call
 *      ```ioda_attribute_read_char```. To read a string, try ```ioda_attribute_read_str```.
 * \tparam SUFFIX is the type (int, long, int64_t) that is appended to this function name
 *   in the C interface.
 * \param att is the attribute.
 * \param sz is the size (in elements) of the data to read.
 * \param vals is the data to read.
 * \return true on success, false on failure.
 * @{
 */

// read - void ioda_attribute_read_char(const ioda_attribute*, size_t, char*);
/// \def IODA_ATTRIBUTE_READ_TEMPLATE
/// \brief See @link ioda_attribute_read ioda_attribute_read @endlink
/// \see ioda_attribute_read
#define IODA_ATTRIBUTE_READ_TEMPLATE(funcnamestr, Type)                                            \
  IODA_DL bool funcnamestr(const struct ioda_attribute* att, size_t sz, Type* vals);
C_TEMPLATE_FUNCTION_DEFINITION_NOSTR(ioda_attribute_read, IODA_ATTRIBUTE_READ_TEMPLATE);
IODA_DL struct ioda_string_ret_t* ioda_attribute_read_str(const struct ioda_attribute* att);

/*! @}
 * @brief Class-like encapsulation of C attribute functions.
 * @see c_ioda for an example.
 * @see use_c_ioda for an example.
 */
struct c_attribute {
  void (*destruct)(struct ioda_attribute*);
  struct ioda_dimensions* (*getDimensions)(const struct ioda_attribute*);

// int isA_char(const ioda_attribute*);
#define IODA_ATTRIBUTE_ISA_FUNC_TEMPLATE(shortnamestr, basenamestr)                                \
  int (*shortnamestr)(const struct ioda_attribute*);
  C_TEMPLATE_FUNCTION_DECLARATION_3(isA, ioda_attribute_isa, IODA_ATTRIBUTE_ISA_FUNC_TEMPLATE);

// void write_char(ioda_attribute*, size_t, const Type*);
#define IODA_ATTRIBUTE_WRITE_FUNC_TEMPLATE(shortnamestr, basenamestr, Type)                        \
  bool (*shortnamestr)(struct ioda_attribute*, size_t, const Type*);
  C_TEMPLATE_FUNCTION_DECLARATION_4_NOSTR(write, ioda_attribute_write,
                                          IODA_ATTRIBUTE_WRITE_FUNC_TEMPLATE);
  bool (*write_str)(struct ioda_attribute*, size_t, const char* const*);

// bool read_char(const ioda_attribute*, size_t, char*);
#define IODA_ATTRIBUTE_READ_FUNC_TEMPLATE(shortnamestr, basenamestr, Type)                         \
  bool (*shortnamestr)(const struct ioda_attribute*, size_t, Type*);
  C_TEMPLATE_FUNCTION_DECLARATION_4_NOSTR(read, ioda_attribute_read,
                                          IODA_ATTRIBUTE_READ_FUNC_TEMPLATE);
  struct ioda_string_ret_t* (*read_str)(const struct ioda_attribute*);
};

#ifdef __cplusplus
}
#endif

/*! @}   // end Doxygen block
 */
