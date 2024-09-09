/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/containers/ViewColsData.h"

#include "oops/util/Logger.h"

#include "ioda/containers/Constants.h"

osdf::ViewColsData::ViewColsData(const Functions& funcs,
    const ColumnMetadata& columnMetadata, const std::vector<std::int64_t>& ids,
    const std::vector<std::shared_ptr<DataBase>>& dataColumns) :
    IColsData(), funcs_(funcs), columnMetadata_(columnMetadata),
    ids_(ids), dataColumns_(dataColumns) {}

const std::int32_t osdf::ViewColsData::getSizeCols() const {
  return static_cast<std::int32_t>(dataColumns_.size());
}

const std::int64_t osdf::ViewColsData::getSizeRows() const {
  return static_cast<std::int64_t>(ids_.size());
}

const std::int64_t osdf::ViewColsData::getMaxId() const {
  return columnMetadata_.getMaxId();
}

const std::int32_t osdf::ViewColsData::getIndex(const std::string& name) const {
  return columnMetadata_.getIndex(name);
}

const std::string& osdf::ViewColsData::getName(const std::int32_t index) const {
  return columnMetadata_.getName(index);
}

const std::int8_t osdf::ViewColsData::getType(const std::int32_t index) const {
  return columnMetadata_.getType(index);
}

const std::int8_t osdf::ViewColsData::columnExists(const std::string& name) const {
  return columnMetadata_.exists(name);
}

const std::shared_ptr<osdf::DataBase>& osdf::ViewColsData::getDataColumn(
                                                           const std::int32_t index) const {
  return dataColumns_.at(static_cast<std::size_t>(index));
}

const osdf::ColumnMetadata& osdf::ViewColsData::getColumnMetadata() const {
  return columnMetadata_;
}

const std::vector<std::int64_t>& osdf::ViewColsData::getIds() const {
  return ids_;
}

const std::vector<std::shared_ptr<osdf::DataBase>>& osdf::ViewColsData::getDataCols() const {
  return dataColumns_;
}

void osdf::ViewColsData::print() const {
  if (dataColumns_.size() > 0) {
    const std::string maxRowIdString = std::to_string(columnMetadata_.getMaxId());
    const std::int32_t maxRowIdStringSize = static_cast<std::int32_t>(maxRowIdString.size());
    columnMetadata_.print(funcs_, maxRowIdStringSize);
    const std::int32_t numColumns = static_cast<std::int32_t>(dataColumns_.size());
    for (std::int64_t rowIndex = 0; rowIndex < static_cast<std::int64_t>(ids_.size()); ++rowIndex) {
      const std::size_t rowIdx = static_cast<std::size_t>(rowIndex);
      oops::Log::info() <<
        funcs_.padString(std::to_string(ids_.at(rowIdx)), maxRowIdStringSize);
      for (std::int32_t columnIndex = 0; columnIndex < numColumns; ++columnIndex) {
        const std::size_t colIdx = static_cast<std::size_t>(columnIndex);
        oops::Log::info() << consts::kBigSpace <<
          funcs_.padString(dataColumns_.at(colIdx)->getValueStr(rowIndex),
          columnMetadata_.get(columnIndex).getWidth());
      }
      oops::Log::info() << std::endl;
    }
  }
}
