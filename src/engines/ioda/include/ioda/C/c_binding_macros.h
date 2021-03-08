#pragma once
/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \defgroup ioda_c_binding_macros C++/C binding macros
 * \brief Provides the C-style bindings for ioda's templated C++ classes and functions.
 * \ingroup ioda_c_api
 *
 * @{
 * \file c_binding_macros.h
 * \brief @link ioda_c_binding_macros C bindings interface @endlink to templated C++ ioda classes
 * and functions.
 */

#ifndef __cplusplus
#  include <stdint.h>
#else
#  include <cstdint>
#  include <iostream>
#  include <stdexcept>
#  include <string>
#endif  // ifndef __CPLUSPLUS

/*! \def C_TRY
 *  \brief Goes with C_CATCH_AND_TERMINATE.
 */
#define C_TRY try {
/*! \def C_CATCH_AND_TERMINATE
 * \brief Catch C++ exceptions before they go across code boundaries.
 * \details
 * This is needed because exceptions are not supposed to propagate across language
 * boundaries. Undefined behavior, and at best the program would terminate cleanly.
 * This macro ensures that we call std::terminate, which calls std::abort, and
 * prints an error indicating where this occurs.
 *
 * \see C_TRY for the other end of the block.
 */
#define C_CATCH_AND_TERMINATE                                                                      \
  }                                                                                                \
  catch (std::exception & e) {                                                                     \
    std::cerr << e.what() << std::endl;                                                            \
    std::terminate();                                                                              \
  }                                                                                                \
  catch (...) {                                                                                    \
    std::terminate();                                                                              \
  }

/*! \def C_CATCH_AND_RETURN
 *  \brief This macro catches C++ exceptions. If they are recoverable, then return the
 *   error value. If nonrecoverable, behave as C_CATCH_AND_TERMINATE.
 */
#define C_CATCH_AND_RETURN(retval_on_success, retval_on_error)                                     \
  return (retval_on_success);                                                                      \
  }                                                                                                \
  catch (std::exception & e) {                                                                     \
    std::cerr << e.what() << std::endl;                                                            \
    return retval_on_error;                                                                        \
  }                                                                                                \
  catch (...) {                                                                                    \
    std::terminate();                                                                              \
  }

/*! \def C_CATCH_RETURN_FREE
 *  \brief Like C_CATCH_AND_RETURN, but free any in-function allocated C resource before returning
 *   to avoid memory leaks.
 */
#define C_CATCH_RETURN_FREE(retval_on_success, retval_on_error, freeable)                          \
  return (retval_on_success);                                                                      \
  }                                                                                                \
  catch (std::exception & e) {                                                                     \
    std::cerr << e.what() << std::endl;                                                            \
    delete freeable;                                                                               \
    return retval_on_error;                                                                        \
  }                                                                                                \
  catch (...) {                                                                                    \
    std::terminate();                                                                              \
  }

/**
 * \def C_TEMPLATE_FUNCTION_DEFINITION
 * \brief Used to expand templates to provide bindings for
 *   template-deprived languages (C, Fortran).
 * \note There is a similar macro in the ioda python interface.
 *
 * \def C_TEMPLATE_FUNCTION_DECLARATION
 * \brief Used to expand templates to provide bindings for
 *   template-deprived languages (C, Fortran).
 * \note There is a similar macro in the ioda python interface.
 **/

#define C_TEMPLATE_FUNCTION_DEFINITION_NOSTR(funcname, PATTERN)                                    \
  PATTERN(funcname##_float, float)                                                                 \
  PATTERN(funcname##_double, double)                                                               \
  PATTERN(funcname##_ldouble, long double)                                                         \
  PATTERN(funcname##_char, char)                                                                   \
  PATTERN(funcname##_short, short int)                                                             \
  PATTERN(funcname##_ushort, unsigned short int)                                                   \
  PATTERN(funcname##_int, int)                                                                     \
  PATTERN(funcname##_uint, unsigned int)                                                           \
  PATTERN(funcname##_lint, long int)                                                               \
  PATTERN(funcname##_ulint, unsigned long int)                                                     \
  PATTERN(funcname##_llint, long long int)                                                         \
  PATTERN(funcname##_ullint, unsigned long long int)                                               \
  PATTERN(funcname##_int32, int32_t)                                                               \
  PATTERN(funcname##_uint32, uint32_t)                                                             \
  PATTERN(funcname##_int16, int16_t)                                                               \
  PATTERN(funcname##_uint16, uint16_t)                                                             \
  PATTERN(funcname##_int64, int64_t)                                                               \
  PATTERN(funcname##_uint64, uint64_t)
/*
Problems:
PATTERN(funcname ## _bool, bool)   \
*/

#define C_TEMPLATE_FUNCTION_DECLARATION_4_NOSTR(shortname, basename, PATTERN)                      \
  PATTERN(shortname##_float, basename##_float, float);                                             \
  PATTERN(shortname##_double, basename##_double, double);                                          \
  PATTERN(shortname##_ldouble, basename##_ldouble, long double);                                   \
  PATTERN(shortname##_char, basename##_char, char);                                                \
  PATTERN(shortname##_short, basename##_short, short);                                             \
  PATTERN(shortname##_ushort, basename##_ushort, unsigned short);                                  \
  PATTERN(shortname##_int, basename##_int, int);                                                   \
  PATTERN(shortname##_uint, basename##_uint, unsigned);                                            \
  PATTERN(shortname##_lint, basename##_lint, long);                                                \
  PATTERN(shortname##_ulint, basename##_ulint, unsigned long);                                     \
  PATTERN(shortname##_llint, basename##_llint, long long);                                         \
  PATTERN(shortname##_ullint, basename##_ullint, unsigned long long);                              \
  PATTERN(shortname##_int32, basename##_int32, int32_t);                                           \
  PATTERN(shortname##_uint32, basename##_uint32, uint32_t);                                        \
  PATTERN(shortname##_int16, basename##_int16, int16_t);                                           \
  PATTERN(shortname##_uint16, basename##_uint16, uint16_t);                                        \
  PATTERN(shortname##_int64, basename##_int64, int64_t);                                           \
  PATTERN(shortname##_uint64, basename##_uint64, uint64_t);

#define C_TEMPLATE_FUNCTION_DECLARATION_3_NOSTR(shortname, basename, PATTERN)                      \
  PATTERN(shortname##_float, basename##_float);                                                    \
  PATTERN(shortname##_double, basename##_double);                                                  \
  PATTERN(shortname##_ldouble, basename##_ldouble);                                                \
  PATTERN(shortname##_char, basename##_char);                                                      \
  PATTERN(shortname##_short, basename##_short);                                                    \
  PATTERN(shortname##_ushort, basename##_ushort);                                                  \
  PATTERN(shortname##_int, basename##_int);                                                        \
  PATTERN(shortname##_uint, basename##_uint);                                                      \
  PATTERN(shortname##_lint, basename##_lint);                                                      \
  PATTERN(shortname##_ulint, basename##_ulint);                                                    \
  PATTERN(shortname##_llint, basename##_llint);                                                    \
  PATTERN(shortname##_ullint, basename##_ullint);                                                  \
  PATTERN(shortname##_int32, basename##_int32);                                                    \
  PATTERN(shortname##_uint32, basename##_uint32);                                                  \
  PATTERN(shortname##_int16, basename##_int16);                                                    \
  PATTERN(shortname##_uint16, basename##_uint16);                                                  \
  PATTERN(shortname##_int64, basename##_int64);                                                    \
  PATTERN(shortname##_uint64, basename##_uint64);

#define C_TEMPLATE_FUNCTION_DECLARATION_3(shortname, basename, PATTERN)                            \
  C_TEMPLATE_FUNCTION_DECLARATION_3_NOSTR(shortname, basename, PATTERN)                            \
  PATTERN(shortname##_str, basename##_str);

#define C_TEMPLATE_FUNCTION_DECLARATION_NOSTR(funcname, PATTERN)                                   \
  C_TEMPLATE_FUNCTION_DECLARATION_3_NOSTR(funcname, funcname, PATTERN)

#define C_TEMPLATE_FUNCTION_DECLARATION(funcname, PATTERN)                                         \
  C_TEMPLATE_FUNCTION_DECLARATION_3_NOSTR(funcname, funcname, PATTERN)                             \
  PATTERN(funcname##_str, funcname##_str)

#define C_TEMPLATE_FUNCTION_DEFINITION(funcname, PATTERN)                                          \
  C_TEMPLATE_FUNCTION_DEFINITION_NOSTR(funcname, PATTERN)                                          \
  PATTERN(funcname##_str, std::string)

/// @} // End Doxygen block
