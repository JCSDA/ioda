/*
 * (C) Copyright 2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! @file Units.cpp
* @brief UDUNITS-2 bindings and wrappers
*/
#include "Units.h"

#include <udunits2.h>

#include <memory>

#include "Log.h"
#include "ioda/Exception.h"

namespace ioda_validate {
namespace units {

struct udunits_interface_impl {
  std::shared_ptr<ut_system> utsys;
};

int handle_log(const char* fmt, va_list args) {
  const int sz = 2048;
  char buffer[sz];  // NOLINT: cppcheck mistakenly warns about variable-length arrays.
  vsnprintf(buffer, sz, fmt, args);  // Always null-terminates.
  std::string msg(buffer);
  Log::log(Severity::Trace) << msg << "\n";
  return 0;
}

void udunits_interface::_static_init() { ut_set_error_message_handler(handle_log); }

udunits_interface::udunits_interface() {
  impl_        = std::make_unique<udunits_interface_impl>();
  impl_->utsys = std::shared_ptr<ut_system>(ut_read_xml(nullptr), ut_free_system);
  if (!impl_->utsys) throw ioda::Exception("Cannot find udunits XML file.", ioda_Here());
}
udunits_interface::~udunits_interface() = default;

const udunits_interface& udunits_interface::instance() {
  static bool inited = false;
  if (!inited) {
    _static_init();
    inited = true;
  }
  static const udunits_interface res;
  return res;
}

ConvertResult udunits_interface::canConvert(const std::string& inUnits,
                                            const std::string& outUnits) const {
  using std::shared_ptr;
  ConvertResult res;

  shared_ptr<ut_unit> inunit(ut_parse(impl_->utsys.get(), inUnits.c_str(), UT_UTF8), ut_free);
  shared_ptr<ut_unit> outunit(ut_parse(impl_->utsys.get(), outUnits.c_str(), UT_UTF8), ut_free);
  if (inunit.get()) res.validInputUnits = true;
  if (outunit.get()) res.validOutputUnits = true;
  if (!inunit || !outunit) return res;  // Units are invalid.

  int can_convert     = ut_are_convertible(inunit.get(), outunit.get());
  res.canConvert      = (can_convert) ? true : false;
  int compare         = ut_compare(inunit.get(), outunit.get());
  res.equivalentUnits = (compare) ? false : true;

  return res;
}

}  // end namespace units
}  // end namespace ioda_validate
