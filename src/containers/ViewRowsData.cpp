/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/containers/ViewRowsData.h"

#include "ioda/containers/Constants.h"

osdf::ViewRowsData::ViewRowsData(const Functions& funcs, const ColumnMetadata& columnMetadata,
                                const std::vector<std::shared_ptr<DataRow>>& dataRows) :
    IRowsData(), funcs_(funcs), columnMetadata_(columnMetadata), dataRows_(dataRows) {}

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

const std::int8_t osdf::ViewRowsData::getType(const std::int32_t index) const {
  return columnMetadata_.getType(index);
}

const std::int8_t osdf::ViewRowsData::columnExists(const std::string& name) const {
  return columnMetadata_.exists(name);
}

osdf::DataRow& osdf::ViewRowsData::getDataRow(const std::int64_t index) {
  return *dataRows_.at(static_cast<std::size_t>(index));
}

const osdf::DataRow& osdf::ViewRowsData::getDataRow(const std::int64_t index) const {
  return *dataRows_.at(static_cast<std::size_t>(index));
}

const osdf::ColumnMetadata& osdf::ViewRowsData::getColumnMetadata() const {
  return columnMetadata_;
}

const std::vector<std::shared_ptr<osdf::DataRow>>& osdf::ViewRowsData::getDataRows() const {
  return dataRows_;
}

void osdf::ViewRowsData::print() const {
  if (dataRows_.size() > 0) {
    const std::string maxRowIdString = std::to_string(columnMetadata_.getMaxId());
    const std::int32_t maxRowIdStringSize = static_cast<std::int32_t>(maxRowIdString.size());
    columnMetadata_.print(funcs_, maxRowIdStringSize);
    for (const std::shared_ptr<DataRow>& dataRow : dataRows_) {
      dataRow->print(funcs_, columnMetadata_, maxRowIdStringSize);
    }
  }
}
