/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef DATUM_H
#define DATUM_H

#include <string>

#include "ioda/containers/DatumBase.h"

namespace osdf {
template<class T>
class Datum : public DatumBase {
 public:
  explicit Datum(const T&);

  virtual ~Datum() = default;

  Datum()                         = delete;  //!< Deleted default constructor
  Datum(Datum&&)                  = delete;  //!< Deleted move constructor
  Datum(const Datum&)             = delete;  //!< Deleted copy constructor
  Datum& operator=(Datum&&)       = delete;  //!< Deleted move assignment
  Datum& operator=(const Datum&)  = delete;  //!< Deleted copy assignment

  const T getDatum() const {
    return value_;
  }

  T getDatum() {
    return value_;
  }

  const std::string getDatumStr() const;

  void setDatum(const T value) {
    value_ = value;
  }

 private:
  T value_;
};
}  // namespace osdf

#endif  // DATUM_H
