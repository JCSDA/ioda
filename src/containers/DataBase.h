/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef CONTAINERS_DATABASE_H_
#define CONTAINERS_DATABASE_H_

#include <cstdint>
#include <string>

namespace osdf {
class DataBase {
 public:
  explicit DataBase(const std::int8_t type) : type_(type) {}
  virtual ~DataBase() = default;

  DataBase()                           = delete;
  DataBase(DataBase&&)                 = delete;
  DataBase(const DataBase&)            = delete;
  DataBase& operator=(DataBase&&)      = delete;
  DataBase& operator=(const DataBase&) = delete;

  virtual const std::string getValueStr(const std::int64_t) const = 0;

  const std::int8_t getType() const {
    return type_;
  }

 protected:
  std::int8_t type_;
};
}  // namespace osdf

#endif  // CONTAINERS_DATABASE_H_
