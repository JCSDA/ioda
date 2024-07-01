/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/containers/DataRow.h"

#include <string>

#include "ioda/containers/Constants.h"
#include "oops/util/Logger.h"

namespace {
  std::string padString(std::string str, const std::int32_t columnWidth) {
    const std::int32_t diff = columnWidth - str.size();
    if (diff > 0) {
      str.insert(str.end(), diff, osdf::consts::kSpace[0]);
    }
    return str;
  }
}  // anonymous namespace

osdf::DataRow::DataRow(std::int64_t id) : id_(id) {}

const std::int64_t osdf::DataRow::getId() const {
  return id_;
}

const std::int32_t osdf::DataRow::getSize() const {
  return dataColumns_.size();
}

const std::shared_ptr<osdf::DatumBase> osdf::DataRow::getColumn(const std::int32_t& index) const {
  if (index < (std::int32_t)dataColumns_.size()) {
    return dataColumns_.at(index);
  } else {
    return nullptr;
  }
}

std::shared_ptr<osdf::DatumBase> osdf::DataRow::getColumn(const std::int32_t& index) {
  if (index < (std::int32_t)dataColumns_.size()) {
    return dataColumns_.at(index);
  } else {
    return nullptr;
  }
}

void osdf::DataRow::insert(const std::shared_ptr<osdf::DatumBase> datum) {
  dataColumns_.push_back(datum);
}

void osdf::DataRow::remove(const std::int32_t& index) {
  dataColumns_.erase(std::next(dataColumns_.begin(), index));
}

void osdf::DataRow::print(const ColumnMetadata& columnMetadata,
                          const std::int32_t rowStringSize) const {
  oops::Log::info() << padString(std::to_string(id_), rowStringSize);
  std::int32_t columnIndex = 0;
  for (const std::shared_ptr<DatumBase>& datum : dataColumns_) {
    oops::Log::info() << consts::kBigSpace << padString(datum->getDatumStr(),
                                                        columnMetadata.get(columnIndex).getWidth());
    columnIndex++;
  }
  oops::Log::info() << std::endl;
}

