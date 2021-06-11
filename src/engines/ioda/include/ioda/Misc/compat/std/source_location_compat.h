#pragma once
/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! @file source_location_compat.h
* @brief Implements ioda::source_location.
*/

#include <cstdint>

#if __cplusplus >= 202002L
#  include <source_location>
#endif

namespace ioda {
namespace detail {
/// @brief Compatibility functions
namespace compat {
/// @brief This namespace is imported into the top-level ioda namespace if the
/// C++ language standard version is less than C++20.
namespace source_location {
/*! @brief This is a placeholder implementation for std::source_location, which
* is part of C++20. We backport this to older compilers as needed.
*/
struct source_location {
  /// @brief Determine current source location.
  /// @note Does not quite match the standard since we really need compiler support
  ///   to implement this outside of a macro. For now, ioda users should use ioda_Here instead.
  /// @see ioda_Here for the standard implementation.
  inline static source_location current() noexcept { return source_location(); }
  /// @brief Default constructor. Implementation-defined behavior.
  source_location() noexcept : line_(0), column_(0), file_name_(""), function_name_("") {}

  source_location(uint_least32_t line, uint_least32_t column, const char* file, const char* func)
      : line_(line), column_(column), file_name_(file), function_name_(func) {}

  // source location field access
  uint_least32_t line() const noexcept { return line_; }
  uint_least32_t column() const noexcept { return column_; }
  const char* file_name() const noexcept { return file_name_; }
  const char* function_name() const noexcept { return function_name_; }

private:
  uint_least32_t line_;
  uint_least32_t column_;
  const char* file_name_;
  const char* function_name_;
};
}  // namespace source_location
}  // namespace compat
}  // namespace detail

// __func__ is a more standard option, but this ignores function parameters and only
// displays the function name. Bad for distinguishing overrides.
#if defined(_MSC_FULL_VER)
#  define IODA_FUNCSIG __FUNCSIG__
#else
// gcc, clang, and Apple clang all define this.
#  define IODA_FUNCSIG __PRETTY_FUNCTION__
#endif

/*! @def ioda_Here is a macro to determine the current location in source code.
* @details
* This macro is still needed in pre-C++20 compilers to accurately and portably
* determine source filenames, functions, and line numbers.
* 
* Until C++20-conformant compilers are available, ioda_Here needs to be passed in
* every ioda Exception to get accurate line number information.
* 
* gcc, clang, and Intel do have nonstandard builtin intrinsic functions to do this,
* but let's aim for maximum portability. A feature test macro in CMake could help
* detect where these intrinsics are available: __builtin_LINE, __builtin_FUNCTION,
* __builtin_FILE.
*/

// NOLINTs because cpplint complains about "using" declarations outside of functions,
// particularly in headers.

#if __cplusplus >= 202002L
using std::source_location;  // NOLINT
#  define ioda_Here() ::std::source_location::current()
#else
using detail::compat::source_location::source_location;  // NOLINT
#  define ioda_Here() ::ioda::source_location(__LINE__, 0, __FILE__, IODA_FUNCSIG)
#endif

}  // namespace ioda
