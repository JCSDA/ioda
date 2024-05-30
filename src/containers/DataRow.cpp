/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "DataRow.h"

#include <string>

#include "Constants.h"
#include "oops/util/Logger.h"

namespace {
  std::string padString(std::string str, const std::int32_t columnWidth) {
    const std::int32_t diff = columnWidth - str.size();
    if (diff > 0) {
      str.insert(str.end(), diff, consts::kSpace[0]);
    }
    return str;
  }
}  // anonymous namespace

DataRow::DataRow(std::int64_t id) : id_(id) {}

const std::int64_t DataRow::getId() {
  return id_;
}

const std::int32_t DataRow::getSize() {
  return dataColumns_.size();
}

const std::shared_ptr<DatumBase> DataRow::getColumn(const std::int32_t& index) const {
  if (index < dataColumns_.size()) {
    return dataColumns_[index];
  } else {
    return nullptr;
  }
}

std::shared_ptr<DatumBase> DataRow::getColumn(const std::int32_t& index) {
  if (index < dataColumns_.size()) {
    return dataColumns_[index];
  } else {
    return nullptr;
  }
}

void DataRow::insert(const std::shared_ptr<DatumBase> datum) {
  dataColumns_.push_back(datum);
}

void DataRow::remove(const std::int32_t& index) {
  dataColumns_.erase(std::next(dataColumns_.begin(), index));
}

void DataRow::print(const std::vector<ColumnMetadatum>& columnMetadata,
                    const std::int32_t& rowStringSize) const {
  oops::Log::info() << padString(std::to_string(id_), rowStringSize);
  std::int32_t columnIndex = 0;
  for (const std::shared_ptr<DatumBase>& datum : dataColumns_) {
    oops::Log::info() << consts::kBigSpace
                      << padString(datum->getDatumStr(), columnMetadata[columnIndex].getWidth());
    columnIndex++;
  }
  oops::Log::info() << std::endl;
}

