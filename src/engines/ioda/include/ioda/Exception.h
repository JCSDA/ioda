#pragma once
/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! @file Exception.h
* @brief IODA's error system
*/
#include <exception>
#include <iostream>
#include <list>
#include <sstream>
#include <string>
#include <utility>

#include "./defs.h"
#include "Misc/Options.h"
#include "Misc/compat/std/source_location_compat.h"

namespace ioda {
/*! @brief The ioda exception class.
* 
* @details
* 
* IODA is used to ingest data from external sources (ioda-converters), provides the runtime
* interface for JEDI's data access, and is used to analyze files for offline diagnostics.
* As a result, ioda is perhaps the one jedi component that must build everywhere with minimal
* dependencies. We do not want to deploy the full stack in all situations, particularly in
* difficult-to-configure Python environments (Anaconda), so we do not want to use eckit
* exceptions in the core ioda-engines code.
* 
* This class is the default exception class for ioda. All ioda errors should use this class.
* 
* <h3>USAGE EXAMPLES:</h3>
* 
* 1. To throw a regular error:
*    ```cpp
*    throw ioda::Exception();
*    ```
* 
* 2. To throw a basic error with a single-line message:
*    ```cpp
*    throw ioda::Exception("This is an error");
*    ```
* 
* 3. To throw an error with some data:
*    ```cpp
*    throw ioda::Exception().add<std::string>("Reason", "Some descriptive error goes here.")
*                           .add<int>("status-code", 42);
*    ```
*/
class IODA_DL Exception : virtual public std::exception {
  Options opts_;
  mutable std::string emessage_;
  void invalidate();
  void add_source_location(const ::ioda::source_location& loc);
  void add_call_stack();

public:
  Exception(const ::ioda::source_location& loc = source_location::current(),
            const Options& opts                = Options{});
  explicit Exception(const char* msg,
                     const ::ioda::source_location& loc = source_location::current(),
                     const Options& opts                = Options{});
  explicit Exception(std::string msg,
                     const ::ioda::source_location& loc = source_location::current(),
                     const Options& opts                = Options{});
  virtual ~Exception() noexcept;
  /// \brief Print the error message.
  /// \todo Make this truly noexcept. Should never throw unless a
  /// memory corruption error has occurred, in which case the
  /// program will terminate anyways.
  virtual const char* what() const noexcept;

  /// Add a key-value pair to the error message.
  template <class T>
  Exception& add(const std::string& key, const T value) {
    opts_.add<T>(key, value);
    invalidate();
    return *this;
  }
};

/// Convenience function for unwinding an exception stack.
IODA_DL void unwind_exception_stack(const std::exception& e, std::ostream& out = std::cerr,
                                    int level = 0);

}  // namespace ioda
