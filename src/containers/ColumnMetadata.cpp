/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/containers/ColumnMetadata.h"

#include <algorithm>

#include "ioda/containers/Constants.h"
#include "oops/util/Logger.h"

osdf::ColumnMetadata::ColumnMetadata(): maxId_(0) {}

const std::int8_t osdf::ColumnMetadata::exists(const std::string& name) const {
  auto it = std::find_if(columnMetadata_.begin(), columnMetadata_.end(),
                         [&] (ColumnMetadatum const& col) {
    return col.getName() == name;
  });
  if (it != end(columnMetadata_)) {
    return true;
  } else {
    return false;
  }
}

const std::int32_t osdf::ColumnMetadata::add(const ColumnMetadatum col) {
  if (exists(col.getName()) == false) {
    const std::int32_t index = static_cast<std::int32_t>(columnMetadata_.size());
    columnMetadata_.push_back(col);
    return index;
  } else {
    return consts::kErrorReturnValue;
  }
}

const std::int32_t osdf::ColumnMetadata::add(const std::vector<ColumnMetadatum> columnMetadatums) {
  std::int32_t index = static_cast<std::int32_t>(columnMetadata_.size());
  for (const ColumnMetadatum& columnMetadatum : columnMetadatums) {
    index = add(columnMetadatum);
    if (index == consts::kErrorReturnValue) {
      break;
    }
  }
  return index;
}

const std::vector<osdf::ColumnMetadatum>& osdf::ColumnMetadata::get() const {
  return columnMetadata_;
}

const osdf::ColumnMetadatum& osdf::ColumnMetadata::get(const std::int32_t columnIndex) const {
  return columnMetadata_.at(static_cast<std::size_t>(columnIndex));
}

void osdf::ColumnMetadata::resetMaxId() {
  maxId_ = 0;
}

void osdf::ColumnMetadata::updateMaxId(const std::int64_t id) {
  if (id > maxId_) {
    maxId_ = id;
  }
}

void osdf::ColumnMetadata::updateColumnWidth(const std::int32_t index,
                                             const std::int16_t valueWidth) {
  const std::int16_t currentWidth = columnMetadata_.at(static_cast<std::size_t>(index)).getWidth();
  if (valueWidth > currentWidth) {
    columnMetadata_.at(static_cast<std::size_t>(index)).setWidth(valueWidth);
  }
}

void osdf::ColumnMetadata::remove(const std::int32_t index) {
  columnMetadata_.erase(std::next(columnMetadata_.begin(), index));
}

const std::string& osdf::ColumnMetadata::getName(const std::int32_t index) const {
  if (index >= 0 && index < static_cast<std::int32_t>(columnMetadata_.size())) {
    return columnMetadata_.at(static_cast<std::size_t>(index)).getName();
  } else {
    return consts::kErrorReturnString;
  }
}

const std::int8_t osdf::ColumnMetadata::getType(const std::int32_t index) const {
  if (index >= 0 && index < static_cast<std::int32_t>(columnMetadata_.size())) {
    return columnMetadata_.at(static_cast<std::size_t>(index)).getType();
  } else {
    return static_cast<std::int8_t>(consts::kErrorReturnValue);
  }
}

const std::int8_t osdf::ColumnMetadata::getPermission(const std::int32_t index) const {
  if (index >= 0 && index < static_cast<std::int32_t>(columnMetadata_.size())) {
    return columnMetadata_.at(static_cast<std::size_t>(index)).getPermission();
  } else {
    return static_cast<std::int8_t>(consts::kErrorReturnValue);
  }
}

const std::int32_t osdf::ColumnMetadata::getIndex(const std::string& name) const {
  const auto it = std::find_if(std::begin(columnMetadata_), std::end(columnMetadata_),
                  [&](ColumnMetadatum const& col) {
    return col.getName() == name;
  });
  if (it != columnMetadata_.end()) {
    return static_cast<std::int32_t>(std::distance(std::begin(columnMetadata_), it));
  } else {
    return consts::kErrorReturnValue;
  }
}

const std::int32_t osdf::ColumnMetadata::getSizeCols() const {
  return static_cast<std::int32_t>(columnMetadata_.size());
}

const std::int64_t osdf::ColumnMetadata::getMaxId() const {
  return maxId_;
}

void osdf::ColumnMetadata::print(const Functions& funcs, const std::int32_t rowStringSize) const {
  oops::Log::info() << funcs.padString(consts::kSpace, rowStringSize) << consts::kBigSpace;
  for (const ColumnMetadatum& columnMetadatum : columnMetadata_) {
    const std::string name = columnMetadatum.getName();
    const std::int16_t width = columnMetadatum.getWidth();
    oops::Log::info() << funcs.padString(name, width) << consts::kBigSpace;
  }
  oops::Log::info() << std::endl;
}

void osdf::ColumnMetadata::clear() {
  columnMetadata_.clear();
}
