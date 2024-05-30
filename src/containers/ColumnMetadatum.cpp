/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ColumnMetadatum.h"

#include "Constants.h"

#include "ioda/Exception.h"
#include "ioda/Group.h"

ColumnMetadatum::ColumnMetadatum(std::string name, std::int8_t type, std::int8_t permission):
  name_(name), width_(name.size()), type_(validateType(type)),
  permission_(validatePermission(permission)) {}

ColumnMetadatum::ColumnMetadatum(std::string name, std::int8_t type):
  name_(name), width_(name.size()), type_(validateType(type)), permission_(consts::eReadWrite) {}

const std::string& ColumnMetadatum::getName() const {
  return name_;
}

const std::int16_t& ColumnMetadatum::getWidth() const {
  return width_;
}

const std::int8_t& ColumnMetadatum::getType() const {
  return type_;
}

const std::int8_t& ColumnMetadatum::getPermission() const {
  return permission_;
}

bool ColumnMetadatum::setName(const std::string& name) {
  if (permission_ == consts::eReadWrite) {
    name_ = name;
    return true;
  }
  return false;
}

bool ColumnMetadatum::setPermission(const std::int8_t& permission) {
  if (permission_ == consts::eReadWrite) {
    permission_ = permission;
    return true;
  }
  return false;
}

void ColumnMetadatum::setWidth(const std::int16_t& width) {
  width_ = width;
}

std::int8_t ColumnMetadatum::validateType(const std::int8_t& type) {
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

std::int8_t ColumnMetadatum::validatePermission(const std::int8_t& permission) {
  switch (permission) {
    case consts::eReadOnly: break;
    case consts::eReadWrite: break;
    default: throw ioda::Exception("ERROR: Permission set not recognised.", ioda_Here());
  }
  return permission;
}
