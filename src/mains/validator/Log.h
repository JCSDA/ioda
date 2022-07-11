#pragma once
/*
 * (C) Copyright 2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! @file Log.h
* @brief Encapsulated logging extensions for ioda-validate.
*/

#include "./Params.h"

struct Results {
  size_t numErrors   = 0;
  size_t numWarnings = 0;
};

namespace Log {

struct LogContext {
 public:
  explicit LogContext(const std::string &s = "");
  ~LogContext();
};

std::ostream &log(ioda_validate::Severity s);
std::ostream &log(ioda_validate::Severity s, Results &res);

}  // end namespace Log
