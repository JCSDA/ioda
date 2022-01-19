/*
 * (C) Copyright 2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! @file Log.cpp
* @brief Encapsulated logging extensions for ioda-validate.
*/

#include <iostream>
#include "eckit/log/Colour.h"
#include "./Log.h"

namespace Log {

const ioda_validate::Severity LogThreshold = ioda_validate::Severity::Info;

std::ostream &LogStream = std::clog;
std::ostringstream junk;

size_t IndentLevel = 0;

LogContext::LogContext(const std::string &s) {
  if (!s.empty()) {
    eckit::Colour::reset(LogStream);
    LogStream << std::string(IndentLevel, ' ') << s << "\n";
  }
  IndentLevel++;
}

LogContext::~LogContext() { IndentLevel--; }

std::ostream &log(ioda_validate::Severity s) {
  std::string messagePrefix;
  if (s >= LogThreshold) {
    eckit::Colour::reset(LogStream);
    switch (s) {
    case ioda_validate::Severity::Error:
      eckit::Colour::bold(LogStream);
      eckit::Colour::red(LogStream);
      messagePrefix = "ERROR: ";
      break;
    case ioda_validate::Severity::Warn:
      eckit::Colour::bold(LogStream);
      eckit::Colour::blue(LogStream);
      messagePrefix = "Warning: ";
      break;
    default:
      break;
    }
    return LogStream << std::string(IndentLevel, ' ') << messagePrefix;
  }
  return junk;
}

std::ostream &log(ioda_validate::Severity s, Results &res) {
  if (s >= ioda_validate::Severity::Error)
    res.numErrors++;
  else if (s >= ioda_validate::Severity::Warn)
    res.numWarnings++;

  return log(s);
}

}  // end namespace Log
