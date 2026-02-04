/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/containers/ColumnMetadata.h"

#include <algorithm>

#include "eckit/exception/Exceptions.h"

#include "ioda/containers/Constants.h"
#include "ioda/core/IodaUtils.h"
#include "oops/util/Logger.h"

osdf::ColumnMetadata::ColumnMetadata(): maxId_(-1) {}

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
  if (exists(col.getName())) {
    const std::string errMsg = std::string("ERROR: Cannot add column named \"") + col.getName() +
          std::string("\" to ColumnMetadata because a column with that name already exists.");
    throw eckit::BadParameter(errMsg, Here());
  }
  const std::int32_t index = static_cast<std::int32_t>(columnMetadata_.size());
  columnMetadata_.push_back(col);
  return index;
}

const std::int32_t osdf::ColumnMetadata::add(const std::vector<ColumnMetadatum> columnMetadatums) {
  std::int32_t index = static_cast<std::int32_t>(columnMetadata_.size());
  for (const ColumnMetadatum& columnMetadatum : columnMetadatums) {
    index = add(columnMetadatum);
  }
  return index;
}

const std::vector<osdf::ColumnMetadatum>& osdf::ColumnMetadata::get() const {
  return columnMetadata_;
}

std::vector<std::string> osdf::ColumnMetadata::columnNames() const {
  std::vector<std::string> names;
  names.reserve(columnMetadata_.size());
  for (const ColumnMetadatum& columnMetadatum : columnMetadata_) {
    names.push_back(columnMetadatum.getName());
  }
  return names;
}

std::string osdf::ColumnMetadata::serialize() const {
  std::string output = "columns:" + std::to_string(columnMetadata_.size());
  for (const ColumnMetadatum& columnMetadatum : columnMetadata_) {
    output += " " + columnMetadatum.getName();
    output += ":" + std::to_string(columnMetadatum.getType());
    output += ":" + std::to_string(columnMetadatum.getPermission());
    output += ":" + std::to_string(columnMetadatum.getWidth());
  }
  return output;
}

void osdf::ColumnMetadata::deserialize(const std::string & columnMetadataTokens) {
  // Don't allow deserialization into non-empty ColumnMetadata
  if (columnMetadata_.size() != 0) {
    const std::string errMsg =
      std::string("ERROR: Column metadata can only be deserialized into an empty container.");
    throw eckit::BadValue(errMsg, Here());
  }

  // Do some basic checks on the input string and extract the tokens
  const std::vector<std::string> tokens = ioda::splitString(columnMetadataTokens, ' ');
  const std::vector<std::string> firstTokenParts = ioda::splitString(tokens[0], ':');
  if (firstTokenParts.size() != 2 || firstTokenParts[0] != "columns") {
    const std::string errMsg = std::string("ERROR: Column metadata string is not valid.");
    throw eckit::BadValue(errMsg, Here());
  }
  const int nCols = std::stoi(firstTokenParts[1]);
  if (nCols != static_cast<int>(tokens.size()) - 1) {
    const std::string errMsg = std::string("ERROR: Column metadata string has inconsistent ") +
          std::string("number of columns.");
    throw eckit::BadValue(errMsg, Here());
  }

  for (int i = 1; i <= nCols; ++i) {
    std::vector<std::string> tokenParts = ioda::splitString(tokens[i], ':');
    if (tokenParts.size() != 4) {
      const std::string errMsg = std::string("ERROR: Column metadata string has invalid ") +
            std::string("column information: ") + tokens[i];
      throw eckit::BadValue(errMsg, Here());
    }

    // Extract the column information and create the ColumnMetadatum
    const std::string name = tokenParts[0];
    const std::int8_t type = static_cast<std::int8_t>(std::stoi(tokenParts[1]));
    const std::int8_t permission = static_cast<std::int8_t>(std::stoi(tokenParts[2]));
    const std::int16_t width = static_cast<std::int16_t>(std::stoi(tokenParts[3]));
    ColumnMetadatum columnMetadatum(name, type, permission);
    columnMetadatum.setWidth(width);
    columnMetadata_.push_back(columnMetadatum);
  }
}

const osdf::ColumnMetadatum& osdf::ColumnMetadata::get(const std::int32_t columnIndex) const {
  return columnMetadata_.at(static_cast<std::size_t>(columnIndex));
}

void osdf::ColumnMetadata::resetMaxId() {
  maxId_ = -1;
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
  if (index < 0 || index >= static_cast<std::int32_t>(columnMetadata_.size())) {
    const std::string errMsg = std::string("ERROR: Column index ") + std::to_string(index) +
          std::string(" is out of bounds.");
    throw eckit::OutOfRange(errMsg, Here());
  }
  return columnMetadata_.at(static_cast<std::size_t>(index)).getName();
}

const std::int8_t osdf::ColumnMetadata::getType(const std::int32_t index) const {
  if (index < 0 || index >= static_cast<std::int32_t>(columnMetadata_.size())) {
    const std::string errMsg = std::string("ERROR: Column index ") + std::to_string(index) +
          std::string(" is out of bounds.");
    throw eckit::OutOfRange(errMsg, Here());
  }
  return columnMetadata_.at(static_cast<std::size_t>(index)).getType();
}

const std::int8_t osdf::ColumnMetadata::getPermission(const std::int32_t index) const {
  if (index < 0 || index >= static_cast<std::int32_t>(columnMetadata_.size())) {
    const std::string errMsg = std::string("ERROR: Column index ") + std::to_string(index) +
          std::string(" is out of bounds.");
    throw eckit::OutOfRange(errMsg, Here());
  }
  return columnMetadata_.at(static_cast<std::size_t>(index)).getPermission();
}

const std::int32_t osdf::ColumnMetadata::getIndex(const std::string& name) const {
  const auto it = std::find_if(std::begin(columnMetadata_), std::end(columnMetadata_),
                  [&](ColumnMetadatum const& col) {
    return col.getName() == name;
  });
  if (it == columnMetadata_.end()) {
    const std::string errMsg = std::string("ERROR: Cannot find column named \"") + name +
          std::string("\" in ColumnMetadata.");
    throw eckit::BadParameter(errMsg, Here());
  }
  return static_cast<std::int32_t>(std::distance(std::begin(columnMetadata_), it));
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
