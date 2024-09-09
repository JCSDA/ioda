/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef CONTAINERS_DATUMBASE_H_
#define CONTAINERS_DATUMBASE_H_

#include <cstdint>
#include <string>

namespace osdf {
class DatumBase {
 public:
  explicit DatumBase(const std::int8_t type) : type_(type) {}
  virtual ~DatumBase() = default;

  DatumBase()                            = delete;
  DatumBase(DatumBase&&)                 = delete;
  DatumBase(const DatumBase&)            = delete;
  DatumBase& operator=(DatumBase&&)      = delete;
  DatumBase& operator=(const DatumBase&) = delete;

  virtual const std::string getValueStr() const = 0;

  const std::int8_t getType() const {
    return type_;
  }

 protected:
  std::int8_t type_;
};
}  // namespace osdf

#endif  // CONTAINERS_DATUMBASE_H_
