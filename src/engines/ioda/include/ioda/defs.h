#pragma once
/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// \file defs.h
/// \brief Common preprocessor definitions used throughout IODA.

#ifdef __cplusplus
#include <cstddef>
#else
#include <stddef.h>
#endif

// Compiler interface warning suppression
#ifdef _MSC_FULL_VER
//# include <CppCoreCheck/Warnings.h>
//# pragma warning(disable: CPPCORECHECK_DECLARATION_WARNINGS)
#pragma warning(push)
#pragma warning(disable : 4003)  // Bug in boost with VS2016.3
#pragma warning(disable : 4251)  // DLL interface
#pragma warning(disable : 4275)  // DLL interface
// Avoid unnamed objects. Buggy in VS / does not respect attributes telling the compiler to ignore the check.
#pragma warning(disable : 26444)
#pragma warning(disable : 4661)  // Template definition
#pragma warning(disable : 4554)  // Eigen Tensor warning
#pragma warning(disable : 4996)  // Old versions of Eigen may use C++17-deprecated functions.
#endif

/* Symbol export / import macros */

/**
 * \defgroup IODA_Symbols_Shared ioda - Symbol export / import.
 * @{
 *
 * \def IODA_COMPILER_EXPORTS_VERSION
 * 0 - when we have no idea what the compiler is.
 * 1 - when the compiler is like MSVC.
 * 2 - when the compiler is like GNU, Intel, or Clang.
 */

#if defined(_MSC_FULL_VER)
#define IODA_COMPILER_EXPORTS_VERSION 1
#elif defined(__INTEL_COMPILER) || defined(__GNUC__) || defined(__MINGW32__) || defined(__clang__)
#define IODA_COMPILER_EXPORTS_VERSION 2
#else
#define IODA_COMPILER_EXPORTS_VERSION 0
#endif

// Defaults for static libraries

/**
 * \def IODA_SHARED_EXPORT
 * \brief A tag used to tell the compiler that a symbol should be exported.
 *
 * \def IODA_SHARED_IMPORT
 * \brief A tag used to tell the compiler that a symbol should be imported.
 *
 * \def IODA_HIDDEN
 * \brief A tag used to tell the compiler that a symbol should not be listed,
 * but it may be referenced from other code modules.
 *
 * \def IODA_PRIVATE
 * \brief A tag used to tell the compiler that a symbol should not be listed,
 * and it may not be referenced from other code modules.
 **/

#if IODA_COMPILER_EXPORTS_VERSION == 1
#define IODA_SHARED_EXPORT __declspec(dllexport)
#define IODA_SHARED_IMPORT __declspec(dllimport)
#define IODA_HIDDEN
#define IODA_PRIVATE
#elif IODA_COMPILER_EXPORTS_VERSION == 2
#define IODA_SHARED_EXPORT __attribute__((visibility("default")))
#define IODA_SHARED_IMPORT __attribute__((visibility("default")))
#define IODA_HIDDEN __attribute__((visibility("hidden")))
#define IODA_PRIVATE __attribute__((visibility("internal")))
#else
#pragma message( \
  "ioda - defs.h warning: compiler is unrecognized. Shared libraries may not export their symbols properly.")
#define IODA_SHARED_EXPORT
#define IODA_SHARED_IMPORT
#define IODA_HIDDEN
#define IODA_PRIVATE
#endif

/**
 * \def IODA_DL
 * \brief A preprocessor tag that indicates that a symbol is to be exported/imported.
 *
 * If ioda_SHARED is defined, then the target library both
 * exports and imports. If not defined, then it is a static library.
 *
 *
 **/

#if ioda_SHARED
#ifdef ioda_EXPORTING
#define IODA_DL IODA_SHARED_EXPORT
#else
#define IODA_DL IODA_SHARED_IMPORT
#endif
#else
#define IODA_DL
#endif

#ifdef __cplusplus

/// \brief The root-level namespace for the ioda library. All the important stuff has classes here.
namespace ioda {
/// \brief Implementation details. Regular users should not use objects in this class
/// directly, though several user-intended classes inherit from objects here.
namespace detail {}

/// Defines the dimensions for a bunch of objects.
typedef ptrdiff_t Dimensions_t;

/// Used in classification of objects into Groups, Variables, et cetera.
enum class ObjectType {
  /// Used as an input value for filtering.
  /// Specify this if you want to examine everything.
  Ignored,
  /// Support for this object type is unimplemented in this version of ioda-engines.
  Unimplemented,
  /// Object is a Group
  Group,
  /// Object is a Variable
  Variable
};
}  // namespace ioda

#endif  // __cplusplus
