/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/containers/FrameColsData.h"

#include <algorithm>
#include <utility>

#include "oops/util/Logger.h"
#include "ioda/containers/Constants.h"
#include "ioda/Exception.h"

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
  for (const ColumnMetadatum& column : columns) {
    std::shared_ptr<DataBase> data;
    switch (column.getType()) {
      case consts::eInt8: {
        std::vector<std::int8_t> values;
        data = funcs_.createData<std::int8_t>(values);
        break;
      }
      case consts::eInt16: {
        std::vector<std::int16_t> values;
        data = funcs_.createData<std::int16_t>(values);
        break;
      }
      case consts::eInt32: {
        std::vector<std::int32_t> values;
        data = funcs_.createData<std::int32_t>(values);
        break;
      }
      case consts::eInt64: {
        std::vector<std::int64_t> values;
        data = funcs_.createData<std::int64_t>(values);
        break;
      }
      case consts::eFloat: {
        std::vector<float> values;
        data = funcs_.createData<float>(values);
        break;
      }
      case consts::eDouble: {
        std::vector<double> values;
        data = funcs_.createData<double>(values);
        break;
      }
      case consts::eString: {
        std::vector<std::string> values;
        data = funcs_.createData<std::string>(values);
        break;
      }
      default: throw ioda::Exception("ERROR: Data type misconfiguration...", ioda_Here());
    }
    dataColumns_.push_back(data);
  }
  if (columnMetadata_.add(std::move(columns)) == consts::kErrorReturnValue) {
    throw ioda::Exception("ERROR: Column names cannot repeat.", ioda_Here());
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
    std::shared_ptr<DataBase>& data = dataColumns_.at(static_cast<std::size_t>(columnIndex));
    const std::int16_t datumSize = static_cast<std::int16_t>(datum->getValueStr().size());
    columnMetadata_.updateColumnWidth(columnIndex, datumSize);
    switch (datum->getType()) {  // Previously checked for type compatibility
      case consts::eInt8: {
        funcs_.addDatumValue<std::int8_t>(data, datum);
        break;
      }
      case consts::eInt16: {
        funcs_.addDatumValue<std::int16_t>(data, datum);
        break;
      }
      case consts::eInt32: {
        funcs_.addDatumValue<std::int32_t>(data, datum);
        break;
      }
      case consts::eInt64: {
        funcs_.addDatumValue<std::int64_t>(data, datum);
        break;
      }
      case consts::eFloat: {
        funcs_.addDatumValue<float>(data, datum);
        break;
      }
      case consts::eDouble: {
        funcs_.addDatumValue<double>(data, datum);
        break;
      }
      case consts::eString: {
        funcs_.addDatumValue<std::string>(data, datum);
        break;
      }
    }
  }
}

void osdf::FrameColsData::appendNewColumn(const std::shared_ptr<DataBase>& data,
    const std::string& name, const std::int8_t type, const std::int8_t permission) {
  if (columnMetadata_.add(ColumnMetadatum(name, type, permission)) == consts::kErrorReturnValue) {
    throw ioda::Exception("ERROR: Column names cannot repeat.", ioda_Here());
  }
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
    switch (data->getType()) {
      case consts::eInt8: {
        funcs_.removeDatum<std::int8_t>(data, index);
        break;
      }
      case consts::eInt16: {
        funcs_.removeDatum<std::int16_t>(data, index);
        break;
      }
      case consts::eInt32: {
        funcs_.removeDatum<std::int32_t>(data, index);
        break;
      }
      case consts::eInt64: {
        funcs_.removeDatum<std::int64_t>(data, index);
        break;
      }
      case consts::eFloat: {
        funcs_.removeDatum<float>(data, index);
        break;
      }
      case consts::eDouble: {
        funcs_.removeDatum<double>(data, index);
        break;
      }
      case consts::eString: {
        funcs_.removeDatum<std::string>(data, index);
        break;
      }
    }
  }
}

const std::int32_t osdf::FrameColsData::getSizeCols() const {
  return static_cast<std::int32_t>(dataColumns_.size());
}

const std::int64_t osdf::FrameColsData::getSizeRows() const {
  return static_cast<std::int64_t>(ids_.size());
}

const std::int64_t osdf::FrameColsData::getMaxId() const {
  return columnMetadata_.getMaxId();
}

const std::int32_t osdf::FrameColsData::getIndex(const std::string& name) const {
  return columnMetadata_.getIndex(name);
}

const std::string& osdf::FrameColsData::getName(const std::int32_t index) const {
  return columnMetadata_.getName(index);
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

void osdf::FrameColsData::initialise(const std::int64_t sizeRows) {
  ids_.clear();
  for (std::int64_t rowIndex = 0; rowIndex < sizeRows; ++rowIndex) {
    ids_.push_back(rowIndex);
  }
  columnMetadata_.updateMaxId(static_cast<std::int64_t>(ids_.size() - 1));
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
    switch (data->getType()) {
      case consts::eInt8: {
        funcs_.clearData<std::int8_t>(data);
        break;
      }
      case consts::eInt16: {
        funcs_.clearData<std::int16_t>(data);
        break;
      }
      case consts::eInt32: {
        funcs_.clearData<std::int32_t>(data);
        break;
      }
      case consts::eInt64: {
        funcs_.clearData<std::int64_t>(data);
        break;
      }
      case consts::eFloat: {
        funcs_.clearData<float>(data);
        break;
      }
      case consts::eDouble: {
        funcs_.clearData<double>(data);
        break;
      }
      case consts::eString: {
        funcs_.clearData<std::string>(data);
        break;
      }
    }
  }
  dataColumns_.clear();
  ids_.clear();
  columnMetadata_.clear();
}

/// Private functions
