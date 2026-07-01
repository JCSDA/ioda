/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/containers/ColumnMetadatum.h"

#include "ioda/containers/Constants.h"
#include "ioda/containers/FrameUtils.h"
#include "eckit/exception/Exceptions.h"
#include "oops/util/missingValues.h"

osdf::ColumnMetadatum::ColumnMetadatum(const std::string& name, const consts::eDataTypes type,
                                       const consts::ePermissions permission)
    : name_(name),
      unit_(util::missingValue<std::string>()),
      width_(static_cast<std::int16_t>(name.size())),
      type_(validateType(type)),
      permission_(validatePermission(permission)) {}

osdf::ColumnMetadatum::ColumnMetadatum(const std::string& name, const std::string& unit,
                                       const consts::eDataTypes type,
                                       const consts::ePermissions permission)
    : name_(name),
      unit_(unit),
      width_(static_cast<std::int16_t>(name.size())),
      type_(validateType(type)),
      permission_(validatePermission(permission)) {}

osdf::ColumnMetadatum::ColumnMetadatum(const std::string& name, const consts::eDataTypes type)
    : name_(name),
      unit_(util::missingValue<std::string>()),
      width_(static_cast<std::int16_t>(name.size())),
      type_(validateType(type)),
      permission_(consts::eReadWrite) {}

osdf::ColumnMetadatum::ColumnMetadatum(const std::string& name, const std::string& unit,
                                       const consts::eDataTypes type)
    : name_(name),
      unit_(unit),
      width_(static_cast<std::int16_t>(name.size())),
      type_(validateType(type)),
      permission_(consts::eReadWrite) {}

const std::string& osdf::ColumnMetadatum::getName() const {
  return name_;
}

const std::string& osdf::ColumnMetadatum::getUnit() const {
  return unit_;
}

const std::int16_t osdf::ColumnMetadatum::getWidth() const {
  return width_;
}

const osdf::consts::eDataTypes osdf::ColumnMetadatum::getType() const {
  return type_;
}

const osdf::consts::ePermissions osdf::ColumnMetadatum::getPermission() const {
  return permission_;
}

void osdf::ColumnMetadatum::setWidth(const std::int16_t width) {
  width_ = width;
}

void osdf::ColumnMetadatum::setUnit(const std::string& unit) {
  if (permission_ == consts::eReadWrite) {
    unit_ = unit;
  } else {
    throw eckit::BadValue(
      "Unable to set unit for column " + name_ + " with readOnly permissions.", Here());
  }
}

osdf::consts::eDataTypes osdf::ColumnMetadatum::validateType(const consts::eDataTypes type) {
  return osdf::FrameUtils::callWithSupportedType(
    type,
    [&](auto typeDiscriminator) {
      return type;
    });
}

osdf::consts::ePermissions osdf::ColumnMetadatum::validatePermission(
  const consts::ePermissions permission) {
  switch (permission) {
    case consts::eReadOnly: break;
    case consts::eReadWrite: break;
    default: throw eckit::BadParameter("ERROR: Permission set not recognised.", Here());
  }
  return permission;
}
