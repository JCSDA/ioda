/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/containers/FrameRowsData.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

#include "ioda/containers/Constants.h"
#include "ioda/Exception.h"

osdf::FrameRowsData::FrameRowsData(const FunctionsRows& funcs) :
    IFrameData(), IRowsData(), funcs_(funcs) {}

osdf::FrameRowsData::FrameRowsData(const FunctionsRows& funcs, const ColumnMetadata& columnMetadata,
                                   const std::vector<DataRow>& dataRows) :
        IFrameData(), IRowsData(), funcs_(funcs),
        columnMetadata_(columnMetadata), dataRows_(dataRows) {}

void osdf::FrameRowsData::configColumns(const std::vector<ColumnMetadatum> cols) {
  if (columnMetadata_.add(std::move(cols)) == consts::kErrorReturnValue) {
    throw ioda::Exception("ERROR: Column names cannot repeat.", ioda_Here());
  }
}

void osdf::FrameRowsData::configColumns(const std::initializer_list<ColumnMetadatum> initList) {
  std::vector<ColumnMetadatum> cols;
  std::copy(std::begin(initList), std::end(initList), std::back_inserter(cols));
  configColumns(std::move(cols));
}

void osdf::FrameRowsData::appendNewRow(const DataRow& newRow) {
  std::string rowStr = std::to_string(newRow.getId());
  columnMetadata_.updateMaxId(newRow.getId());
  dataRows_.push_back(newRow);  // May need to update column widths based on new data
}

void osdf::FrameRowsData::appendNewColumn(const std::string& name, const std::int8_t type,
                                          const std::int8_t permission) {
  if (columnMetadata_.add(ColumnMetadatum(name, type, permission)) == consts::kErrorReturnValue) {
    throw ioda::Exception("ERROR: Column names cannot repeat.", ioda_Here());
  }
}

void osdf::FrameRowsData::removeColumn(const std::int32_t index) {
  columnMetadata_.remove(index);
  for (DataRow& dataRow : dataRows_) {
    dataRow.remove(index);
  }
}

void osdf::FrameRowsData::removeRow(const std::int64_t index) {
  dataRows_.erase(std::next(dataRows_.begin(), index));
}

void osdf::FrameRowsData::updateColumnWidth(const std::int32_t columnIndex,
                                            const std::int16_t width) {
  columnMetadata_.updateColumnWidth(columnIndex, width);
}

const std::int32_t osdf::FrameRowsData::getSizeCols() const {
  return columnMetadata_.getSizeCols();
}

const std::int64_t osdf::FrameRowsData::getSizeRows() const {
  return static_cast<std::int64_t>(dataRows_.size());
}

const std::int64_t osdf::FrameRowsData::getMaxId() const {
  return columnMetadata_.getMaxId();
}

const std::int32_t osdf::FrameRowsData::getIndex(const std::string& name) const {
  return columnMetadata_.getIndex(name);
}

const std::string& osdf::FrameRowsData::getName(const std::int32_t index) const {
  return columnMetadata_.getName(index);
}

const std::int8_t osdf::FrameRowsData::getType(const std::int32_t index) const {
  return columnMetadata_.getType(index);
}

const std::int8_t osdf::FrameRowsData::getPermission(const std::int32_t index) const {
  return columnMetadata_.getPermission(index);
}

const std::int8_t osdf::FrameRowsData::columnExists(const std::string& name) const {
  return columnMetadata_.exists(name);
}

osdf::DataRow& osdf::FrameRowsData::getDataRow(const std::int64_t index) {
  return dataRows_.at(static_cast<std::size_t>(index));
}

const osdf::DataRow& osdf::FrameRowsData::getDataRow(const std::int64_t index) const {
  return dataRows_.at(static_cast<std::size_t>(index));
}

const osdf::ColumnMetadata& osdf::FrameRowsData::getColumnMetadata() const {
  return columnMetadata_;
}

const std::vector<osdf::DataRow>& osdf::FrameRowsData::getDataRows() const {
  return dataRows_;
}

void osdf::FrameRowsData::initialise(const std::int64_t sizeRows) {
  dataRows_.clear();
  dataRows_.reserve(static_cast<std::size_t>(sizeRows));
  for (auto _ = sizeRows; _--;) {
    // Give row an ID that is used for printing
    DataRow dataRow(static_cast<std::int64_t>(dataRows_.size()));
    dataRows_.push_back(std::move(dataRow));
  }
  columnMetadata_.updateMaxId(static_cast<std::int64_t>(dataRows_.size() - 1));
}

void osdf::FrameRowsData::print() const {
  if (dataRows_.size() > 0) {
    const std::string maxRowIdString = std::to_string(columnMetadata_.getMaxId());
    const std::int32_t maxRowIdStringSize = static_cast<std::int32_t>(maxRowIdString.size());
    columnMetadata_.print(funcs_, maxRowIdStringSize);
    for (const DataRow& dataRow : dataRows_) {
      dataRow.print(funcs_, columnMetadata_, maxRowIdStringSize);
    }
  }
}

void osdf::FrameRowsData::clear() {
  for (DataRow& dataRow : dataRows_) {
    dataRow.clear();
  }
  dataRows_.clear();
  columnMetadata_.clear();
}
