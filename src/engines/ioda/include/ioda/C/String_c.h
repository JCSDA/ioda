#pragma once
/*
 * (C) Copyright 2020-2021 UCAR
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
