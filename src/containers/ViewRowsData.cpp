/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/containers/ViewRowsData.h"

osdf::ViewRowsData::ViewRowsData(const Functions& funcs, const ColumnMetadata& columnMetadata,
                                const std::vector<DataRow*>& dataRows) :
    funcs_(funcs), columnMetadata_(columnMetadata), dataRows_(dataRows) {}

void osdf::ViewRowsData::setDataRow(const std::int64_t index, DataRow* dataRow) {
  dataRows_.at(static_cast<std::size_t>(index)) = dataRow;
}

const std::int32_t osdf::ViewRowsData::getSizeCols() const {
  return columnMetadata_.getSizeCols();
}

const std::int64_t osdf::ViewRowsData::getSizeRows() const {
  return static_cast<std::int64_t>(dataRows_.size());
}

const std::int64_t osdf::ViewRowsData::getMaxId() const {
  return columnMetadata_.getMaxId();
}

const std::int32_t osdf::ViewRowsData::getIndex(const std::string& name) const {
  return columnMetadata_.getIndex(name);
}

const std::string& osdf::ViewRowsData::getName(const std::int32_t index) const {
  return columnMetadata_.getName(index);
}

const osdf::consts::eDataTypes osdf::ViewRowsData::getType(const std::int32_t index) const {
  return columnMetadata_.getType(index);
}

const bool osdf::ViewRowsData::columnExists(const std::string& name) const {
  return columnMetadata_.exists(name);
}

osdf::DataRow* osdf::ViewRowsData::getDataRow(const std::int64_t index) {
  return dataRows_.at(static_cast<std::size_t>(index));
}

const osdf::DataRow* osdf::ViewRowsData::getDataRow(const std::int64_t index) const {
  return dataRows_.at(static_cast<std::size_t>(index));
}

const osdf::ColumnMetadata& osdf::ViewRowsData::getColumnMetadata() const {
  return columnMetadata_;
}

const std::vector<osdf::DataRow*>& osdf::ViewRowsData::getDataRows() const {
  return dataRows_;
}

void osdf::ViewRowsData::print() {
  if (dataRows_.size() > 0) {
    const std::string maxRowIdString = std::to_string(columnMetadata_.getMaxId());
    const std::int32_t maxRowIdStringSize = static_cast<std::int32_t>(maxRowIdString.size());
    columnMetadata_.print(funcs_, maxRowIdStringSize);
    for (const DataRow* dataRow : dataRows_) {
      dataRow->print(funcs_, columnMetadata_, maxRowIdStringSize);
    }
  }
}

void osdf::ViewRowsData::clear() {
  dataRows_.clear();
  columnMetadata_.clear();
  columnMetadata_.resetMaxId();
}

void osdf::ViewRowsData::setColumnMetadata(const ColumnMetadata& columnMetadata) {
  columnMetadata_ = columnMetadata;
}

void osdf::ViewRowsData::setDataRows(const std::vector<DataRow*>& dataRows) {
  dataRows_ = dataRows;
}
