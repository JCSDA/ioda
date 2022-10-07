#pragma once
/*
 * (C) Copyright 2020-2022 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \defgroup ioda_string Strings
 * \brief Provides the C-style interface for variable-length strings and string arrays.
 * \ingroup ioda_c_api
 *
 * @{
 * \file String_c.h
 * \brief @link ioda_string C bindings @endlink. Needed for reads.
 */
#include "../defs.h"
#include "./c_binding_macros.h"

#ifdef __cplusplus
extern "C" {
#endif

/// \brief Opague blob holding a std::string.
struct c_ioda_string;

/// \brief An encapsulated version of std::string.
struct ioda_string {
  /// \brief Construct a new string.
  /// \see ioda::strings::construct for more details.
  struct ioda_string* (*const construct)();

  /// \brief Construct a new string by copying a C-style null-terminated string.
  /// \see ioda::strings::construct for more details.
  struct ioda_string* (*const constructFromCstr)(const char* buf);

  /// \brief Destroy a string
  /// \param[in,out] str is the string to be destroyed.
  void (*const destruct)(struct ioda_string* str);

  /// \brief Clear a string.
  /// \param[in,out] str is the string to be cleared.
  void (*const clear)(struct ioda_string* str);

  /// \brief Read a string.
  /// \param str[in,out] is the string to read from.
  /// \param outstr[out] is a pointer to the character array to write to.
  /// \param outstr_len is the size of outstr. This function writes up to
  ///   outstr_len bytes, including a null character that terminates
  ///   the string. If the output buffer is not large enough to accommodate
  ///   the string, then it is truncated to outstr_len-1 bytes (plus 
  ///   one byte to store a trailing NULL). Must be nonzero.
  /// \returns the number of bytes actually written, including the trailing
  ///   null character. This is NOT the length of the string as reported
  ///   by strlen(), which would be one byte less.
  size_t (*const get)(const struct ioda_string* str, char* outstr, size_t outstr_len);

  /// \brief Determine the length of a string. Alias of size.
  /// \param[in] str is the object.
  /// \returns the length of the string (as strlen or std::string::size()).
  size_t (*const length)(const struct ioda_string* str);

  /// \brief Write a string.
  /// \param[in,out] str is the object.
  /// \param[in] instr is the source string.
  /// \param instr_len is the number of bytes in the string, excluding a trailing
  ///   null byte (as strlen(instr)).
  /// \returns instr_len on success
  /// \returns 0 on failure.
  size_t (*const set)(struct ioda_string* str, const char* instr, size_t instr_len);

  /// \brief Determine the length of a string. Alias of length.
  /// \param[in] str is the object.
  /// \returns the length of the string (as strlen or std::string::size()).
  size_t (*const size)(const struct ioda_string* str);

  /// \brief Make a copy of a string
  /// \param[in] from is the source string.
  /// \returns A new string, which must be destroyed when no longer used.
  struct ioda_string* (*const copy)(const struct ioda_string* from);

  /// \brief Private opaque data object. Do not access directly.
  struct c_ioda_string *data_;
};





/// \brief Return type when arrays of strings are encountered.
struct ioda_string_ret_t {
  size_t n;
  char** strings;
};

/// \brief Deallocate a returned string object.
IODA_DL void ioda_string_ret_t_destruct(struct ioda_string_ret_t*);

/// \brief Namespace encapsulation for string functions.
struct c_strings {
  void (*destruct)(struct ioda_string_ret_t*);
};

#ifdef __cplusplus
}
#endif

/// @} // End Doxygen block
