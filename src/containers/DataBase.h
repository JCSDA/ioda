/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef DATABASE_H
#define DATABASE_H

#include <cstdint>
#include <string>
#include <vector>

namespace osdf {
class DataBase {
 public:
  DataBase(const std::int8_t type, const std::int64_t size) : type_(type), size_(size) {}
  virtual ~DataBase() = default;

  DataBase()                            = delete;  //!< Deleted default constructor
  DataBase(DataBase&&)                  = delete;  //!< Deleted move constructor
  DataBase(const DataBase&)             = delete;  //!< Deleted copy constructor
  DataBase& operator=(DataBase&&)       = delete;  //!< Deleted move assignment
  DataBase& operator=(const DataBase&)  = delete;  //!< Deleted copy assignment

  virtual const std::string getDatumStr(const std::int64_t) const = 0;

  const std::int8_t getType() const {
    return type_;
  }

  const std::int64_t getSize() const {
    return size_;
  }

 protected:
  std::int8_t type_;
  std::int64_t size_;
};
}  // namespace osdf

#endif  // DATABASE_H
