#pragma once
/*
 * (C) Copyright 2017-2020 Ryan Honeyager (ryan@honeyager.info)
 *
 * This software is licensed under the terms of the BSD 2-Clause License.
 * For details, see the LICENSE file.
 */
#ifndef HH_H_DEFS
#  define HH_H_DEFS

#  ifndef __STDC_WANT_LIB_EXT1__
#    define __STDC_WANT_LIB_EXT1__ 1
#  endif
#  include <cstddef>

#  ifdef _MSC_FULL_VER
#    define HH_DEBUG_FSIG __FUNCSIG__
#  endif
#  if defined(__GNUC__) || defined(__clang__)
#    if defined(__PRETTY_FUNCTION__)
#      define HH_DEBUG_FSIG __PRETTY_FUNCTION__
#    elif defined(__func__)
#      define HH_DEBUG_FSIG __func__
#    elif defined(__FUNCTION__)
#      define HH_DEBUG_FSIG __FUNCTION__
#    else
#      define HH_DEBUG_FSIG ""
#    endif
#  endif

/* Global error codes. */
#  if defined(__STDC_LIB_EXT1__) || defined(__STDC_SECURE_LIB__)
#    define HH_USING_SECURE_STRINGS 1
#    define HH_DEFS_COMPILER_HAS_FPRINTF_S
#    define HH_DEFS_COMPILER_HAS_FPUTS_S
#  else
#    define _CRT_SECURE_NO_WARNINGS
#  endif

// Compiler interface warning suppression
#  if defined _MSC_FULL_VER
//#  include <CppCoreCheck/Warnings.h>
//#  pragma warning(disable: CPPCORECHECK_DECLARATION_WARNINGS)
#    pragma warning(push)
#    pragma warning(disable : 4003)  // Bug in boost with VS2016.3
#    pragma warning(disable : 4251)  // DLL interface
#    pragma warning(disable : 4275)  // DLL interface
// Avoid unnamed objects. Buggy in VS / does not respect attributes telling the compiler to ignore the check.
#    pragma warning(disable : 26444)
#  endif

/// Pointer to an object that is modfied by a function
#  define HH_OUT
/// Denotes an 'optional' parameter (one which can be replaced with a NULL or nullptr)
#  define HH_OPTIONAL

// Errata:

#  ifndef HH_NO_ERRATA
// Boost bug with C++17 requires this define. See
// https://stackoverflow.com/questions/41972522/c2143-c2518-when-trying-to-compile-project-using-boost-multiprecision
#    ifndef _HAS_AUTO_PTR_ETC
#      define _HAS_AUTO_PTR_ETC (!_HAS_CXX17)
#    endif /* _HAS_AUTO_PTR_ETC */
#  endif

// C++ language standard __cplusplus
#  if defined _MSC_FULL_VER
#    if (_MSVC_LANG >= 201703L)
#      define HH_CPP17
#    elif (_MSVC_LANG >= 201402L)
#      define HH_CPP14
#    endif
#  else
#    if (__cplusplus >= 201703L)
#      define HH_CPP17
#    elif (__cplusplus >= 201402L)
#      define HH_CPP14
#    endif
#  endif

// Use the [[nodiscard]] attribute
#  ifdef HH_CPP17
#    define HH_NODISCARD [[nodiscard]]
#  else
#    ifdef _MSC_FULL_VER
#      define HH_NODISCARD _Check_return_
#    else
#      define HH_NODISCARD __attribute__((warn_unused_result))
#    endif
#  endif

// Use the [[maybe_unused]] attribute
#  ifdef HH_CPP17
#    ifdef _MSC_FULL_VER
#      define HH_MAYBE_UNUSED                                                                                \
        [[maybe_unused]] [[gsl::suppress(es .84)]] [[gsl::suppress(expr .84)]] [[gsl::suppress(26444)]]
#    else
#      define HH_MAYBE_UNUSED [[maybe_unused]]
#    endif
#  else
#    ifdef _MSC_FULL_VER
#      define HH_MAYBE_UNUSED [[gsl::suppress(es .84)]]
#    else
#      define HH_MAYBE_UNUSED __attribute__((unused))
#    endif
#  endif

// Error inheritance
#  ifndef HH_ERROR_INHERITS_FROM
#    define HH_ERROR_INHERITS_FROM std::exception
#  endif

#  ifdef __unix__
#    ifdef __linux__
#      define HH_OS_LINUX
#    endif
#    if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__bsdi__)             \
      || defined(__DragonFly__)
#      define HH_OS_UNIX
#    endif
#  endif
#  if (defined(__APPLE__) || defined(__MACH__))
#    define HH_OS_MACOS
#  endif
#  ifdef _WIN32
#    define HH_OS_WINDOWS
#  endif
#  if !defined(HH_OS_WINDOWS) && !defined(HH_OS_UNIX) && !defined(HH_OS_LINUX)
#    define HH_OS_UNSUPPORTED
#    pragma message("HH defs.h warning: operating system is unrecognized.")
#  endif

/* Symbol export / import macros */

/**
 * \defgroup HH_Symbols_Shared HDFforHumans - Symbol export / import.
 * @{
 *
 * \def HH_COMPILER_EXPORTS_VERSION
 * 0 indicates an unknown version. 1 when compiler is MSVC. 2 if like GNU, Intel or Clang.
 */

#  if defined(_MSC_FULL_VER)
#    define HH_COMPILER_EXPORTS_VERSION 1
#  elif defined(__INTEL_COMPILER) || defined(__GNUC__) || defined(__MINGW32__) || defined(__clang__)
#    define HH_COMPILER_EXPORTS_VERSION 2
#  else
#    define HH_COMPILER_EXPORTS_VERSION 0
#  endif

// Defaults for static libraries

/**
 * \def HH_SHARED_EXPORT
 * \brief A tag used to tell the compiler that a symbol should be exported.
 *
 * \def HH_SHARED_IMPORT
 * \brief A tag used to tell the compiler that a symbol should be imported.
 *
 * \def HH_HIDDEN
 * \brief A tag used to tell the compiler that a symbol should not be listed,
 * but it may be referenced from other code modules.
 *
 * \def HH_PRIVATE
 * \brief A tag used to tell the compiler that a symbol should not be listed,
 * and it may not be referenced from other code modules.
 **/

#  if HH_COMPILER_EXPORTS_VERSION == 1
#    define HH_SHARED_EXPORT __declspec(dllexport)
#    define HH_SHARED_IMPORT __declspec(dllimport)
#    define HH_HIDDEN
#    define HH_PRIVATE
#  elif HH_COMPILER_EXPORTS_VERSION == 2
#    define HH_SHARED_EXPORT __attribute__((visibility("default")))
#    define HH_SHARED_IMPORT __attribute__((visibility("default")))
#    define HH_HIDDEN __attribute__((visibility("hidden")))
#    define HH_PRIVATE __attribute__((visibility("internal")))
#  else
#    pragma message(                                                                                         \
      "HH defs.h warning: compiler is unrecognized. Shared libraries may not export their symbols properly.")
#    define HH_SHARED_EXPORT
#    define HH_SHARED_IMPORT
#    define HH_HIDDEN
#    define HH_PRIVATE
#  endif

/**
 * \def HH_DL
 * \brief A preprocessor tag that indicates that a symbol is to be exported/imported.
 *
 * If (libname)_SHARED is defined, then the target library both
 * exports and imports. If not defined, then it is a static library.
 **/

#  if HH_SHARED
#    ifdef HH_EXPORTING
#      define HH_DL HH_SHARED_EXPORT
#    else
#      define HH_DL HH_SHARED_IMPORT
#    endif
#  else
#    define HH_DL
#  endif

/// @}
//
// Closing include guard
#endif
