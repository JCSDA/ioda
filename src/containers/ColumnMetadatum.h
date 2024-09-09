/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef CONTAINERS_COLUMNMETADATUM_H_
#define CONTAINERS_COLUMNMETADATUM_H_

#include <cstdint>
#include <string>

namespace osdf {
class ColumnMetadatum {
 public:
  //  Non-explicit. Can be used with initlializer_list
  ColumnMetadatum(const std::string&, const std::int8_t, const std::int8_t);
  // Non-explicit. Can be used with initlializer_list
  ColumnMetadatum(const std::string&, const std::int8_t);

  // This class uses move and copy constructor and assignment operators.
  ColumnMetadatum() = delete;

  const std::string& getName() const;
  const std::int16_t getWidth() const;
  const std::int8_t getType() const;
  const std::int8_t getPermission() const;

  void setWidth(const std::int16_t);

 private:
  std::int8_t validateType(const std::int8_t);
  std::int8_t validatePermission(const std::int8_t);

  std::string name_;
  std::int16_t width_;
  std::int8_t type_;
  std::int8_t permission_;
};
}  // namespace osdf

#endif  // CONTAINERS_COLUMNMETADATUM_H_
