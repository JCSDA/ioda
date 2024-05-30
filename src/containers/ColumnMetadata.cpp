/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ColumnMetadata.h"

#include <algorithm>

#include "Constants.h"

#include "oops/util/Logger.h"

namespace {
  std::string padString(std::string str, const std::int16_t width) {
    std::int32_t diff = width - str.size();
    if (diff > 0) {
      str.insert(str.end(), diff, consts::kSpace[0]);
    }
    return str;
  }
}  // anonymous namespace

ColumnMetadata::ColumnMetadata(): maxId_(0) {}

std::int8_t ColumnMetadata::exists(const std::string& name) {
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

const std::int64_t ColumnMetadata::add(const ColumnMetadatum col) {
  if (exists(col.getName()) == false) {
    const std::int64_t index = columnMetadata_.size();
    columnMetadata_.push_back(col);
    return index;
  } else {
    // Column with same name already exists. Return value checked by caller.
    return consts::kErrorValue;
  }
}

const std::int64_t ColumnMetadata::add(const std::vector<ColumnMetadatum> columnMetadatums) {
  const std::int64_t index = columnMetadata_.size();
  for (const ColumnMetadatum& columnMetadatum : columnMetadatums) {
    add(columnMetadatum);
  }
  return index;
}

const std::vector<ColumnMetadatum>& ColumnMetadata::get() const {
  return columnMetadata_;
}

void ColumnMetadata::resetMaxId() {
  maxId_ = 0;
}

void ColumnMetadata::updateMaxId(const std::int64_t& id) {
  if (id > maxId_) {
    maxId_ = id;
  }
}

void ColumnMetadata::updateColumnWidth(const std::string& name, const std::int16_t& valueWidth) {
  const std::int32_t index = getIndex(name);
  if (index != -1) {
    updateColumnWidth(index, valueWidth);
  }
}

void ColumnMetadata::updateColumnWidth(const std::int32_t& index, const std::int16_t& valueWidth) {
  const std::int16_t currentWidth = columnMetadata_[index].getWidth();
  if (valueWidth > currentWidth) {
    columnMetadata_[index].setWidth(valueWidth);
  }
}

void ColumnMetadata::remove(const std::int32_t& index) {
  columnMetadata_.erase(std::next(columnMetadata_.begin(), index));
}

const std::string& ColumnMetadata::getName(const std::int32_t& index) const {
  return columnMetadata_[index].getName();
}

const std::int8_t ColumnMetadata::getType(const std::int32_t& index) const {
  return columnMetadata_[index].getType();
}

const std::int8_t ColumnMetadata::getPermission(const std::int32_t& index) const {
  return columnMetadata_[index].getPermission();
}

const std::int32_t ColumnMetadata::getIndex(const std::string& name) const {
  const auto it = std::find_if(std::begin(columnMetadata_), std::end(columnMetadata_),
                               [&](ColumnMetadatum const& col) {
    return col.getName() == name;
  });
  if (it != columnMetadata_.end()) {
    return std::distance(std::begin(columnMetadata_), it);
  } else {
    return consts::kErrorValue;
  }
}

const std::int32_t ColumnMetadata::getNumCols() const {
  return columnMetadata_.size();
}

const std::int64_t ColumnMetadata::getMaxId() const {
  return maxId_;
}

void ColumnMetadata::print(const std::int32_t& rowStringSize) {
  oops::Log::info() << padString(consts::kSpace, rowStringSize) << consts::kBigSpace;
  for (const ColumnMetadatum& columnMetadatum : columnMetadata_) {
    const std::string name = columnMetadatum.getName();
    const std::int16_t width = columnMetadatum.getWidth();
    oops::Log::info() << padString(name, width) << consts::kBigSpace;
  }
  oops::Log::info() << std::endl;
}
