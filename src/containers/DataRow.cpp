/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/containers/DataRow.h"

#include <string>

#include "oops/util/Logger.h"

#include "ioda/containers/ColumnMetadata.h"
#include "ioda/containers/Constants.h"
#include "ioda/containers/Functions.h"

osdf::DataRow::DataRow(std::int64_t id) : id_(id) {}

const std::int64_t osdf::DataRow::getId() const {
  return id_;
}

const std::int32_t osdf::DataRow::getSize() const {
  return static_cast<std::int32_t>(dataColumns_.size());
}

const std::shared_ptr<osdf::DatumBase>& osdf::DataRow::getColumn(const std::int32_t index) const {
  return dataColumns_.at(static_cast<std::size_t>(index));
}

std::shared_ptr<osdf::DatumBase>& osdf::DataRow::getColumn(const std::int32_t index) {
  return dataColumns_.at(static_cast<std::size_t>(index));
}

void osdf::DataRow::insert(const std::shared_ptr<osdf::DatumBase>& datum) {
  dataColumns_.push_back(datum);
}

void osdf::DataRow::remove(const std::int32_t index) {
  dataColumns_.erase(std::next(dataColumns_.begin(), index));
}

void osdf::DataRow::print(const Functions& funcs, const ColumnMetadata& columnMetadata,
                          const std::int32_t rowStringSize) const {
  oops::Log::info() << funcs.padString(std::to_string(id_), rowStringSize);
  std::int32_t columnIndex = 0;
  for (const std::shared_ptr<DatumBase>& datum : dataColumns_) {
    oops::Log::info() << consts::kBigSpace <<
        funcs.padString(datum->getValueStr(), columnMetadata.get(columnIndex).getWidth());
    columnIndex++;
  }
  oops::Log::info() << std::endl;
}

void osdf::DataRow::clear() {
  dataColumns_.clear();
}
