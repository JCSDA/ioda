#pragma once
/*
 * (C) Copyright 2021-2022 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! @file Units.h
* @brief UDUNITS-2 bindings and wrappers
*/
#include <memory>
#include <string>
#include <vector>

namespace ioda {

namespace udunits {

namespace detail {
struct udunits_interface_impl;
struct udunits_units_impl;
}  // end namespace detail

class Converter {
public:
  virtual ~Converter();
  virtual float* convert(const float* in, size_t N, float* out) const    = 0;
  virtual double* convert(const double* in, size_t N, double* out) const = 0;

  template <class T>
  T* tconvert(const T* val, size_t N, T* out) const {
    std::vector<double> val_d(N), out_d(N);
    for (size_t i = 0; i < N; ++i) val_d[i] = static_cast<double>(val[i]);
    convert(val_d.data(), N, out_d.data());
    for (size_t i = 0; i < N; ++i) out[i] = static_cast<T>(out_d[i]);
    return out;
  }
};

class UnitsInterface;

class Units {
private:
  std::shared_ptr<detail::udunits_units_impl> impl_;

public:
  Units(std::shared_ptr<detail::udunits_units_impl>);
  Units(const std::string& units_str = "1");  // Units from string. Defaults to no units.
  ~Units();
  Units operator*(const Units& rhs) const;
  Units operator/(const Units& rhs) const;
  Units raise(int power) const;
  Units root(int power) const;

  bool operator==(const Units& rhs) const;
  inline bool operator!=(const Units& rhs) const { return !operator==(rhs); }
  bool isConvertibleWith(const Units& rhs) const;
  std::shared_ptr<Converter> getConverterTo(const Units& to) const;

  bool isValid() const;
  //double value() const;

  void print(std::ostream&) const;
};

class UnitsInterface {
private:
  std::unique_ptr<detail::udunits_interface_impl> impl_;
  UnitsInterface(const std::string& xmlpath = "");

public:
  static const UnitsInterface& instance(const std::string& xmlpath = "");
  ~UnitsInterface();

  /// Convert a UTF-8 string into units.
  Units units(const std::string& units_str) const;
};

inline Units RegularUnits(const std::string& units_str) {
  return UnitsInterface::instance().units(units_str);
}

}  // end namespace udunits
}  // end namespace ioda

inline std::ostream& operator<<(std::ostream& out, const ioda::udunits::Units& rhs) {
  rhs.print(out);
  return out;
}
