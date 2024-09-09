/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef CONTAINERS_DATUM_H_
#define CONTAINERS_DATUM_H_

#include <string>

#include "ioda/containers/DatumBase.h"

namespace osdf {
template<class T>
class Datum : public DatumBase {
 public:
  explicit Datum(const T&);

  virtual ~Datum() = default;

  Datum()                        = delete;
  Datum(Datum&&)                 = delete;
  Datum(const Datum&)            = delete;
  Datum& operator=(Datum&&)      = delete;
  Datum& operator=(const Datum&) = delete;

  const T getValue() const {
    return value_;
  }

  T getValue() {
    return value_;
  }

  const std::string getValueStr() const;

  void setValue(const T value) {
    value_ = value;
  }

 private:
  T value_;
};
}  // namespace osdf

#endif  // CONTAINERS_DATUM_H_
