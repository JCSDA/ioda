/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/containers/FrameColsData.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <utility>

#include "DataRow.h"
#include "eckit/exception/Exceptions.h"
#include "ioda/containers/Constants.h"
#include "ioda/containers/Data.h"
#include "ioda/containers/FrameUtils.h"
#include "oops/util/Logger.h"
#include "oops/util/missingValues.h"

osdf::FrameColsData::FrameColsData(const FunctionsCols& funcs,
    const ColumnMetadata& columnMetadata, const std::vector<std::int64_t>& ids,
    const std::vector<std::shared_ptr<DataBase>>& dataColumns) : IFrameData(), IColsData(),
        funcs_(funcs), columnMetadata_(columnMetadata),
        ids_(ids), dataColumns_(dataColumns) {
  const std::int64_t maxId = static_cast<std::int64_t>(ids_.size() - 1);
  columnMetadata_.updateMaxId(maxId);
}

osdf::FrameColsData::FrameColsData(const FunctionsCols& funcs) :
    IFrameData(), IColsData(), funcs_(funcs) {}

void osdf::FrameColsData::configColumns(const std::vector<ColumnMetadatum> columns) {
  if (this->getSizeRows() == 0 && this->getColumnMetadata().getSizeCols() == 0) {
    this->initialise(0);
  }

  // Note that `add` throws an exception if a column of the given name(s) already exists.
  columnMetadata_.add(std::move(columns));

  // Fill columns with missing data if FrameColsData already contains columns of length > 0
  for (const ColumnMetadatum& column : columns) {
    std::shared_ptr<DataBase> data;
    osdf::FrameUtils::callWithSupportedType(
      column.getType(),
      [&](auto typeDiscriminator) {
        using T = decltype(typeDiscriminator);
        std::vector<T> values(this->getSizeRows(), util::missingValue<T>());
        data = funcs_.createData<T>(values);
      });
    dataColumns_.push_back(data);
  }
}

void osdf::FrameColsData::configColumns(const std::initializer_list<ColumnMetadatum> initList) {
  std::vector<ColumnMetadatum> cols;
  std::copy(std::begin(initList), std::end(initList), std::back_inserter(cols));
  configColumns(std::move(cols));
}

void osdf::FrameColsData::appendNewRow(const DataRow& newRow) {
  std::int64_t id = newRow.getId();
  columnMetadata_.updateMaxId(id);
  ids_.push_back(id);

  for (std::int32_t columnIndex = 0; columnIndex < newRow.getSize(); ++columnIndex) {
    const std::shared_ptr<DatumBase>& datum = newRow.getColumn(columnIndex);
    const std::int16_t datumSize = static_cast<std::int16_t>(datum->getValueStr().size());
    std::shared_ptr<DataBase>& data = dataColumns_.at(static_cast<std::size_t>(columnIndex));

    columnMetadata_.updateColumnWidth(columnIndex, datumSize);

    osdf::FrameUtils::callWithSupportedType(
      datum->getType(),
      [&](auto typeDiscriminator) {
        using T = decltype(typeDiscriminator);
        funcs_.addDatumValue<T>(data, datum);
      });
  }
}

void osdf::FrameColsData::appendNewColumn(const std::shared_ptr<DataBase>& data,
                                          const std::string& name, const std::int8_t type,
                                          const std::string& unit,
                                          const std::int8_t permission) {
  // Note that `add` throws an exception if columnMetadata with name already exists
  columnMetadata_.add(ColumnMetadatum(name, unit, type, permission));
  dataColumns_.push_back(data);
}

void osdf::FrameColsData::removeColumn(const std::int32_t index) {
  columnMetadata_.remove(index);
  dataColumns_.erase(std::next(dataColumns_.begin(), index));
}

void osdf::FrameColsData::updateMaxId(const std::int64_t id) {
  columnMetadata_.updateMaxId(id);
}

void osdf::FrameColsData::updateColumnWidth(const std::int32_t columnIndex,
                                            const std::int16_t width) {
  columnMetadata_.updateColumnWidth(columnIndex, width);
}

void osdf::FrameColsData::removeRow(const std::int64_t index) {
  ids_.erase(std::next(ids_.begin(), index));
  for (std::shared_ptr<DataBase>& data : dataColumns_) {
    osdf::FrameUtils::callWithSupportedType(
      data->getType(),
      [&](auto typeDiscriminator) {
        using T = decltype(typeDiscriminator);
        funcs_.removeDatum<T>(data, index);
      });
  }
}

const std::int32_t osdf::FrameColsData::getSizeCols() const {
  return static_cast<std::int32_t>(dataColumns_.size());
}

const std::int64_t osdf::FrameColsData::getSizeRows() const {
  return static_cast<std::int64_t>(ids_.size());
}

const std::int64_t osdf::FrameColsData::getMaxId() const { return columnMetadata_.getMaxId(); }

void osdf::FrameColsData::validateColumnMetadata(
  const osdf::ColumnMetadata& srcColumnMetadata) const {
  return columnMetadata_.validateColumnMetadata(srcColumnMetadata);
}

void osdf::FrameColsData::validateColumnMetadataPermissions(
  const osdf::ColumnMetadata& srcColumnMetadata) const {
  return columnMetadata_.validateColumnMetadataPermissions(srcColumnMetadata);
}

void osdf::FrameColsData::validateCanWriteAllData() const {
  return columnMetadata_.validateCanWriteAllData();
}

const std::int32_t osdf::FrameColsData::getIndex(const std::string& name) const {
  return columnMetadata_.getIndex(name);
}

const std::string& osdf::FrameColsData::getName(const std::int32_t index) const {
  return columnMetadata_.getName(index);
}

const std::string& osdf::FrameColsData::getUnits(const std::int32_t index) const {
  return columnMetadata_.getUnit(index);
}

const std::int8_t osdf::FrameColsData::getType(const std::int32_t index) const {
  return columnMetadata_.getType(index);
}

const std::int8_t osdf::FrameColsData::getPermission(const std::int32_t index) const {
  return columnMetadata_.getPermission(index);
}

const std::int8_t osdf::FrameColsData::columnExists(const std::string& name) const {
  return columnMetadata_.exists(name);
}

std::vector<std::int64_t>& osdf::FrameColsData::getIds() {
  return ids_;
}

const std::vector<std::int64_t>& osdf::FrameColsData::getIds() const {
  return ids_;
}

std::shared_ptr<osdf::DataBase>& osdf::FrameColsData::getDataColumn(const std::int32_t index) {
  return dataColumns_.at(static_cast<std::size_t>(index));
}

const std::shared_ptr<osdf::DataBase>& osdf::FrameColsData::getDataColumn(
                                                            const std::int32_t index) const {
  return dataColumns_.at(static_cast<std::size_t>(index));
}

osdf::ColumnMetadata& osdf::FrameColsData::getColumnMetadata() {
  return columnMetadata_;
}

const osdf::ColumnMetadata& osdf::FrameColsData::getColumnMetadata() const {
  return columnMetadata_;
}

std::vector<std::shared_ptr<osdf::DataBase>>& osdf::FrameColsData::getDataCols() {
  return dataColumns_;
}

const std::vector<std::shared_ptr<osdf::DataBase>>& osdf::FrameColsData::getDataCols() const {
  return dataColumns_;
}

osdf::DataRow osdf::FrameColsData::getDataRow(const std::int64_t index) const {
  DataRow newRow(index);

  const std::int32_t numColumns = static_cast<std::int32_t>(dataColumns_.size());
  for (std::int32_t columnIndex = 0; columnIndex < numColumns; ++columnIndex) {
    const std::shared_ptr<DataBase>& data = getDataColumn(columnIndex);

    osdf::FrameUtils::callWithSupportedType(data->getType(), [&](auto typeDiscriminator) {
      using T = decltype(typeDiscriminator);
      const std::shared_ptr<Data<T>> dataType
        = std::static_pointer_cast<Data<T>>(getDataColumn(columnIndex));
      const T param = dataType->getValues().at(static_cast<std::size_t>(index));
      std::shared_ptr<DatumBase> newDatum = funcs_.createDatum<T>(param);
      newRow.insert(newDatum);
    });
  }

  return newRow;
}

void osdf::FrameColsData::getDataRows(std::vector<DataRow>& dataRowsContainer) const {
  if (dataRowsContainer.empty()
      && (static_cast<std::int64_t> (dataRowsContainer.capacity()) == getSizeRows())) {
    for (std::int32_t rowIndex = 0; rowIndex < getSizeRows(); ++rowIndex) {
      dataRowsContainer.emplace_back(getDataRow(rowIndex));
    }
  } else {
    const std::string errMsg = std::string(
      "ERROR: dataRowsContainer must be empty with capacity equal to the number of rows in the "
      "Frame.");
    throw eckit::BadParameter(errMsg, Here());
  }
}

void osdf::FrameColsData::initialise(const std::int64_t sizeRows) {
  ids_.clear();
  for (std::int64_t rowIndex = 0; rowIndex < sizeRows; ++rowIndex) {
    ids_.push_back(rowIndex);
  }
  if (ids_.empty()) {
    columnMetadata_.resetMaxId();
  } else {
    columnMetadata_.updateMaxId(static_cast<std::int64_t>(ids_.size() - 1));
  }
}

void osdf::FrameColsData::print() const {
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

void osdf::FrameColsData::clear() {
  for (std::shared_ptr<DataBase>& data : dataColumns_) {
    osdf::FrameUtils::callWithSupportedType(
      data->getType(),
      [&](auto typeDiscriminator) {
        using T = decltype(typeDiscriminator);
        funcs_.clearData<T>(data);
      });
  }
  dataColumns_.clear();
  ids_.clear();
  columnMetadata_.clear();
  columnMetadata_.resetMaxId();
}

/// Private functions
