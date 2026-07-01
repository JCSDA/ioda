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
#include "ioda/containers/Constants.h"

namespace osdf {
class ColumnMetadatum {
 public:
  // Non-explicit. Can be used with initlializer_list
  ColumnMetadatum(const std::string&, const consts::eDataTypes, const consts::ePermissions);
  // Non-explicit. Can be used with initlializer_list
  ColumnMetadatum(const std::string &, const std::string &, const consts::eDataTypes,
                  const consts::ePermissions);
  // Non-explicit. Can be used with initlializer_list
  ColumnMetadatum(const std::string &, const consts::eDataTypes);
  // Non-explicit. Can be used with initlializer_list
  ColumnMetadatum(const std::string&, const std::string&, const consts::eDataTypes);

  // This class uses move and copy constructor and assignment operators.
  ColumnMetadatum() = delete;

  const std::string& getName() const;
  const std::string& getUnit() const;
  const std::int16_t getWidth() const;
  const consts::eDataTypes getType() const;
  const consts::ePermissions getPermission() const;

  void setWidth(const std::int16_t);
  void setUnit(const std::string&);

 private:
  consts::eDataTypes validateType(const consts::eDataTypes);
  consts::ePermissions validatePermission(const consts::ePermissions);

  std::string name_;
  std::string unit_;
  std::int16_t width_;
  consts::eDataTypes type_;
  consts::ePermissions permission_;
};
}  // namespace osdf

#endif  // CONTAINERS_COLUMNMETADATUM_H_
