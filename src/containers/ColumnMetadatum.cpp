/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/containers/ColumnMetadatum.h"

#include <stdexcept>

#include "ioda/containers/Constants.h"
#include "ioda/Exception.h"

osdf::ColumnMetadatum::ColumnMetadatum(const std::string& name, const std::int8_t type,
                                       const std::int8_t permission):
  name_(name), width_(static_cast<std::int16_t>(name.size())), type_(validateType(type)),
  permission_(validatePermission(permission)) {}

osdf::ColumnMetadatum::ColumnMetadatum(const std::string& name, const std::int8_t type):
  name_(name), width_(static_cast<std::int16_t>(name.size())), type_(validateType(type)),
  permission_(consts::eReadWrite) {}

const std::string& osdf::ColumnMetadatum::getName() const {
  return name_;
}

const std::int16_t osdf::ColumnMetadatum::getWidth() const {
  return width_;
}

const std::int8_t osdf::ColumnMetadatum::getType() const {
  return type_;
}

const std::int8_t osdf::ColumnMetadatum::getPermission() const {
  return permission_;
}

void osdf::ColumnMetadatum::setWidth(const std::int16_t width) {
  width_ = width;
}

std::int8_t osdf::ColumnMetadatum::validateType(const std::int8_t type) {
  switch (type) {
    case consts::eInt8: break;
    case consts::eInt16: break;
    case consts::eInt32: break;
    case consts::eInt64: break;
    case consts::eFloat: break;
    case consts::eDouble: break;
    case consts::eString: break;
    default: throw ioda::Exception("ERROR: Type set not recognised.", ioda_Here());
  }
  return type;
}

std::int8_t osdf::ColumnMetadatum::validatePermission(const std::int8_t permission) {
  switch (permission) {
    case consts::eReadOnly: break;
    case consts::eReadWrite: break;
    default: throw ioda::Exception("ERROR: Permission set not recognised.", ioda_Here());
  }
  return permission;
}
