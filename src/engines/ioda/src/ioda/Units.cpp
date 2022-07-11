/*
 * (C) Copyright 2021-2022 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! @file Units.cpp
* @brief UDUNITS-2 bindings and wrappers
*/

#include "ioda/Units.h"

#include <udunits2.h>

#include <exception>
#include <iostream>
#include <map>
#include <memory>
#include <string>

#include "ioda/Exception.h"

namespace ioda {
namespace udunits {

namespace detail {
class Converter_impl : public Converter {
  std::shared_ptr<cv_converter> converter_;

public:
  Converter_impl(std::shared_ptr<cv_converter> converter) : converter_(converter) {}
  virtual ~Converter_impl() = default;
  float* convert(const float* in, size_t N, float* out) const final {
    return cv_convert_floats(converter_.get(), in, N, out);
  }
  double* convert(const double* in, size_t N, double* out) const final {
    return cv_convert_doubles(converter_.get(), in, N, out);
  }
};

struct udunits_units_impl {
  std::shared_ptr<ut_unit> unit;
  udunits_units_impl(std::shared_ptr<ut_unit> u) : unit(u) {}
  ~udunits_units_impl() = default;
};

struct udunits_interface_impl {
  std::shared_ptr<ut_system> utsys_;
  /*
static int handle_log(const char* fmt, va_list args) {
  const int sz = 2048;
  char buffer[sz];  // NOLINT: cppcheck mistakenly warns about variable-length arrays.
  vsnprintf(buffer, sz, fmt, args);  // Always null-terminates.
  std::string msg(buffer);
  Log::log(Severity::Trace) << msg << "\n";
  return 0;
}
*/

  static void _static_init() {
    // Unified logging can wait on jckit's introduction.
    ut_set_error_message_handler(ut_ignore);
  }
  udunits_interface_impl(std::shared_ptr<ut_system> utsys) : utsys_{utsys} { _static_init(); }
  ~udunits_interface_impl() = default;
};

}  // end namespace detail

Converter::~Converter() = default;

Units::~Units() = default;
Units::Units(std::shared_ptr<detail::udunits_units_impl> impl) : impl_{impl} {}
Units::Units(const std::string& units_str) { *this = UnitsInterface::instance().units(units_str); }

bool Units::operator==(const Units& rhs) const {
  if (!isValid() || !rhs.isValid()) return false;
  return ut_compare(impl_->unit.get(), rhs.impl_->unit.get()) == 0 ? true : false;
}

bool Units::isConvertibleWith(const Units& rhs) const {
  if (!isValid() || !rhs.isValid()) return false;
  return ut_are_convertible(impl_->unit.get(), rhs.impl_->unit.get()) ? true : false;
}

std::shared_ptr<Converter> Units::getConverterTo(const Units& to) const {
  std::shared_ptr<cv_converter> cnv(ut_get_converter(impl_->unit.get(), to.impl_->unit.get()),
                                    cv_free);
  return std::make_shared<detail::Converter_impl>(cnv);
}

bool Units::isValid() const { return impl_->unit.get() ? true : false; }

void Units::print(std::ostream& out) const {
  char buf[256];
  unsigned opts = UT_UTF8 | UT_DEFINITION;
  int len       = ut_format(impl_->unit.get(), buf, sizeof(buf), opts);
  if (len == -1) {
    out << "Error: couldn't get units string. ";
  } else if (len == sizeof(buf)) {
    out << "Error: units string too long. ";
  } else {  // We have a string with a terminating NUL character
    out << std::string(buf);
  }
}

Units Units::operator*(const Units& rhs) const {
  return Units{std::make_shared<detail::udunits_units_impl>(
    std::shared_ptr<ut_unit>(ut_multiply(impl_->unit.get(), rhs.impl_->unit.get()), ut_free))};
}
Units Units::operator/(const Units& rhs) const {
  return Units{std::make_shared<detail::udunits_units_impl>(
    std::shared_ptr<ut_unit>(ut_divide(impl_->unit.get(), rhs.impl_->unit.get()), ut_free))};
}
Units Units::raise(int power) const {
  return Units{std::make_shared<detail::udunits_units_impl>(
    std::shared_ptr<ut_unit>(ut_raise(impl_->unit.get(), power), ut_free))};
}
Units Units::root(int power) const {
  return Units{std::make_shared<detail::udunits_units_impl>(
    std::shared_ptr<ut_unit>(ut_root(impl_->unit.get(), power), ut_free))};
}

UnitsInterface::UnitsInterface(const std::string& xmlpath) {
  detail::udunits_interface_impl::_static_init();
  const char* path = (xmlpath.size()) ? xmlpath.c_str() : nullptr;
  std::shared_ptr<ut_system> utsys(ut_read_xml(path), ut_free_system);
  if (!utsys) throw Exception("Cannot open the unit system.", ioda_Here());
  impl_ = std::make_unique<detail::udunits_interface_impl>(utsys);
}
UnitsInterface::~UnitsInterface() = default;

const UnitsInterface& UnitsInterface::instance(const std::string& xmlpath) {
  static std::map<std::string, std::shared_ptr<const UnitsInterface> > instances;
  if (!instances.count(xmlpath))
    instances[xmlpath] = std::shared_ptr<const UnitsInterface>(new UnitsInterface(xmlpath));
  const UnitsInterface* iface = instances.at(xmlpath).get();
  return *iface;
}

Units UnitsInterface::units(const std::string& units_str) const {
  std::shared_ptr<ut_unit> inunit(ut_parse(impl_->utsys_.get(), units_str.c_str(), UT_UTF8),
                                  ut_free);
  return Units{std::make_shared<detail::udunits_units_impl>(inunit)};
}


}  // end namespace udunits
}  // end namespace ioda
