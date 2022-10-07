#pragma once
/*
 * (C) Copyright 2022 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \defgroup ioda_vecstring Vectors of strings
 * \brief Provides the C-style interface for vectors of strings
 * \ingroup ioda_c_api
 *
 * @{
 * \file VecString_c.h
 * \brief @link ioda_vecstring C bindings @endlink. Needed for reads.
 */
#include "../defs.h"
#include "./c_binding_macros.h"

#ifdef __cplusplus
extern "C" {
#endif

/// \brief Opague blob holding a std::vector<std::string>.
struct c_ioda_VecString;

/// \brief An encapsulated version of std::vector<std::string>.
struct ioda_VecString {
  /// \brief Destroy a vector<string>
  /// \param[in,out] vec is the object to be destroyed.
  void (*const destruct)(struct ioda_VecString* vec);

  /// \brief Construct a new vector<string>.
  struct ioda_VecString* (*const construct)();

  /// \brief Make a copy of a vector<string>
  /// \param[in] from is the source object.
  /// \returns A new vector<string>, which must be destroyed when no longer used.
  struct ioda_VecString* (*const copy)(const struct ioda_VecString* from);

  /// \brief Clear a vector<string>.
  /// \param[in,out] vec is the object to be cleared.
  void (*const clear)(struct ioda_VecString* vec);

  /// \brief Read a string.
  /// \param vec[in] is the object to read from.
  /// \param n is the nth string in the vector.
  /// \param outstr[out] is a pointer to the character array to write to.
  /// \param outstr_len is the size of outstr. This function writes up to
  ///   outstr_len bytes, including padding null characters that terminate
  ///   the string. If the output buffer is not large enough to accommodate
  ///   the string, then it is truncated to outstr_len-1 bytes (plus 
  ///   one byte to store a trailing NULL). Must be nonzero.
  /// \returns the number of bytes actually written, which is an integer from zero to outstr_len.
  ///   This function always ensures that outstr is null-terminated. To get the length of the
  ///   output string, you can also use strlen or strnlen.
  size_t (*const getAsCharArray)(const struct ioda_VecString* vec, size_t n, char* outstr, size_t outstr_len);

  /// \brief Read a string into a buffer with custom termination. Fortran compatibility function.
  /// \param vec[in] is the object to read from.
  /// \param n is the nth string in the vector.
  /// \param outstr[out] is a pointer to the character array to write to.
  /// \param outstr_len is the size of outstr. This function writes up to
  ///   outstr_len bytes.
  /// \param empty_char is the character used to fill space in the output buffer that is beyond
  ///   the length of the string. Fortran callers should use ' '. C callers should use '\0'.
  /// \returns the number of bytes actually written, which is outstr_len on success and
  ///   zero on failure. This function always fills the output buffer with empty_char
  ///   before writing and does not ensure that outstr is null-terminated. To get the length of
  ///   the output string, use min(elementSize, outstr_len).
  size_t (*const getAsCharArray2)(const struct ioda_VecString* vec, size_t n,
    char* outstr, size_t outstr_len, char empty_char);

  /// \brief Write a string.
  /// \param[in,out] vec is the object.
  /// \param n is the nth string in the vector.
  /// \param[in] instr is the source string.
  /// \param instr_len is the number of bytes in the string, excluding any trailing
  ///   null bytes (as strlen(instr)).
  /// \returns instr_len on success
  /// \returns 0 on failure.
  size_t (*const setFromCharArray)(struct ioda_VecString* vec, size_t n, const char* instr, size_t instr_len);

  /// \brief Determine the length of the nth string in the vector.
  /// \param[in] vec is the object.
  /// \param n is the nth string in the vector.
  /// \returns the length of the vector (as strlen or std::string::length()).
  size_t (*const elementSize)(const struct ioda_VecString* vec, size_t n);

  /// \brief Determine the number of elements of a vector<string>.
  /// \param[in] vec is the object.
  /// \returns the length of the vector (as strlen or std::vector::size()).
  size_t (*const size)(const struct ioda_VecString* vec);

  /// \brief Resize the number of elements in the vector<string>.
  /// \param[in,out] vec is the object.
  /// \param newSz is the new size.
  void (*const resize)(struct ioda_VecString* vec, size_t newSz);

  /// \brief Private opaque data object. Do not access directly.
  struct c_ioda_VecString *data_;
};

#ifdef __cplusplus
}
#endif

/// @} // End Doxygen block
