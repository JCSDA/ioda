/*
 * (C) Copyright 2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// \file Exception.cpp
/// \brief Exception classes for IODA

#include "ioda/Exception.h"
#include "oops/util/Stacktrace.h"  // in oops/util

namespace ioda {

Exception::Exception(const ::ioda::source_location& loc, const Options& opts) : opts_{opts} {
  add_source_location(loc);
  add_call_stack();
}

Exception::Exception(const char* msg, const ::ioda::source_location& loc, const Options& opts)
    : opts_{opts} {
  add("Reason", std::string(msg));
  add_source_location(loc);
  add_call_stack();
}

Exception::Exception(std::string msg, const ::ioda::source_location& loc, const Options& opts)
    : opts_{opts} {
  add("Reason", msg);
  add_source_location(loc);
  add_call_stack();
}

Exception::~Exception() noexcept = default;

void Exception::invalidate() { emessage_ = ""; }

void Exception::add_source_location(const ::ioda::source_location& loc) {
  add("source_filename", loc.file_name());
  add("source_line", loc.line());
  add("source_function", loc.function_name());
  add("source_column", loc.column());
}

void Exception::add_call_stack() {
  add("stacktrace", std::string("\n") + util::stacktrace_current());
}

const char* Exception::what() const noexcept {
  try {
    if (emessage_.size()) return emessage_.c_str();
    std::ostringstream o;
    opts_.enumVals(o);
    emessage_ = o.str();
    return emessage_.c_str();
  } catch (...) {
    static const char errmsg[] = "An unknown / unhandleable exception was encountered in IODA.\n";
    return errmsg;
  }
}

void unwind_exception_stack(const std::exception& e, std::ostream& out, int level) {
  out << "Exception: level: " << level << "\n" << e.what() << std::endl;
  try {
    std::rethrow_if_nested(e);
  } catch (const std::exception& f) {
    unwind_exception_stack(f, out, level + 1);
  } catch (...) {
    out << "exception: level: " << level
        << "\n\tException at this level is not derived from std::exception." << std::endl;
  }
}

}  // namespace ioda
