
/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/containers/ObsDataFrameCols.h"

#include <memory>

#include "ioda/containers/Constants.h"
#include "ioda/containers/Data.h"
#include "ioda/containers/Datum.h"
#include "ioda/containers/ObsDataFrameRows.h"

namespace {
  std::string padString(std::string str, const std::int32_t columnWidth) {
    const std::int32_t diff = columnWidth - str.size();
    if (diff > 0) {
      str.insert(str.end(), diff, osdf::consts::kSpace[0]);
    }
    return str;
  }
}  // anonymous namespace

osdf::ObsDataFrameCols::ObsDataFrameCols() : ObsDataFrame(consts::eRowPriority) {}

osdf::ObsDataFrameCols::ObsDataFrameCols(ColumnMetadata columnMetadata,
                                         std::vector<std::shared_ptr<DataBase>> dataColumns) :
      ObsDataFrame(columnMetadata, consts::eRowPriority), dataColumns_(dataColumns) {
  initialise((std::int64_t)dataColumns_.size());
}

osdf::ObsDataFrameCols::ObsDataFrameCols(const ObsDataFrameRows& obsDataFrameRows) :
      ObsDataFrame(consts::eColumnPriority) {
  std::int64_t numRows = obsDataFrameRows.getNumRows();
  initialise(numRows);
  // Create metadata - columns are read-write and do not inherit any read-only permissions
  ColumnMetadata columnMetadata = obsDataFrameRows.getColumnMetadata();
  for (const ColumnMetadatum& columnMetadatum : columnMetadata.get()) {
    std::string name = columnMetadatum.getName();
    std::int8_t type = columnMetadatum.getType();
    // Ignore returned column index
    static_cast<void>(columnMetadata_.add(ColumnMetadatum(name, type)));
  }
  // Create data
  for (const DataRow& dataRow : obsDataFrameRows.getDataRows()) {
    std::int64_t id = dataRow.getId();
    columnMetadata_.updateMaxId(id);

    std::int32_t rowColumns = dataRow.getSize();
    std::int8_t initColumn = false;
    if (dataColumns_.size() == 0) {
      initColumn = true;
    }
    for (std::int32_t columnIndex = 0; columnIndex < rowColumns; ++columnIndex) {
      std::shared_ptr<DatumBase> datum = dataRow.getColumn(columnIndex);
      std::int8_t type = datum->getType();
      switch (type) {
        case consts::eInt8: {
          construct<std::int8_t>(datum, initColumn, numRows, columnIndex);
          break;
        }
        case consts::eInt16: {
          construct<std::int16_t>(datum, initColumn, numRows, columnIndex);
          break;
        }
        case consts::eInt32: {
          construct<std::int32_t>(datum, initColumn, numRows, columnIndex);
          break;
        }
        case consts::eInt64: {
          construct<std::int64_t>(datum, initColumn, numRows, columnIndex);
          break;
        }
        case consts::eFloat: {
          construct<float>(datum, initColumn, numRows, columnIndex);
          break;
        }
        case consts::eDouble: {
          construct<double>(datum, initColumn, numRows, columnIndex);
          break;
        }
        case consts::eString: {
          construct<std::string>(datum, initColumn, numRows, columnIndex);
          break;
        }
      }
    }
  }
}

void osdf::ObsDataFrameCols::appendNewColumn(const std::string& name,
                                             const std::vector<std::int8_t>& values) {
  appendNewColumn<std::int8_t>(name, values, consts::eInt8);
}

void osdf::ObsDataFrameCols::appendNewColumn(const std::string& name,
                                             const std::vector<std::int16_t>& values) {
  appendNewColumn<std::int16_t>(name, values, consts::eInt16);
}

void osdf::ObsDataFrameCols::appendNewColumn(const std::string& name,
                                             const std::vector<std::int32_t>& values) {
  appendNewColumn<std::int32_t>(name, values, consts::eInt32);
}

void osdf::ObsDataFrameCols::appendNewColumn(const std::string& name,
                                             const std::vector<std::int64_t>& values) {
  appendNewColumn<std::int64_t>(name, values, consts::eInt64);
}

void osdf::ObsDataFrameCols::appendNewColumn(const std::string& name,
                                             const std::vector<float>& values) {
  appendNewColumn<float>(name, values, consts::eFloat);
}

void osdf::ObsDataFrameCols::appendNewColumn(const std::string& name,
                                             const std::vector<double>& values) {
  appendNewColumn<double>(name, values, consts::eDouble);
}

void osdf::ObsDataFrameCols::appendNewColumn(const std::string& name,
                                             const std::vector<std::string>& values) {
  appendNewColumn<std::string>(name, values, consts::eString);
}

void osdf::ObsDataFrameCols::appendNewRow(const DataRow& newRow) {
  std::int64_t id = newRow.getId();
  columnMetadata_.updateMaxId(id);
  ids_.push_back(id);
  for (std::int32_t columnIndex = 0; columnIndex < newRow.getSize(); ++columnIndex) {
    std::shared_ptr<DatumBase> datum = newRow.getColumn(columnIndex);
    std::shared_ptr<DataBase> data = dataColumns_.at(columnIndex);
    std::int8_t type = datum->getType();  // Previously checked for type compatibility
    switch (type) {
      case consts::eInt8: {
        addDatum<std::int8_t>(data, datum);
        break;
      }
      case consts::eInt16: {
        addDatum<std::int16_t>(data, datum);
        break;
      }
      case consts::eInt32: {
        addDatum<std::int32_t>(data, datum);
        break;
      }
      case consts::eInt64: {
        addDatum<std::int64_t>(data, datum);
        break;
      }
      case consts::eFloat: {
        addDatum<float>(data, datum);
        break;
      }
      case consts::eDouble: {
        addDatum<double>(data, datum);
        break;
      }
      case consts::eString: {
        addDatum<std::string>(data, datum);
        break;
      }
    }
  }
}

void osdf::ObsDataFrameCols::getColumn(const std::string& name,
                                       std::vector<std::int8_t>& data) const {
  getColumn<std::int8_t>(name, data, consts::eInt8);
}

void osdf::ObsDataFrameCols::getColumn(const std::string& name,
                                       std::vector<std::int16_t>& data) const {
  getColumn<std::int16_t>(name, data, consts::eInt16);
}

void osdf::ObsDataFrameCols::getColumn(const std::string& name,
                                       std::vector<std::int32_t>& data) const {
  getColumn<std::int32_t>(name, data, consts::eInt32);
}

void osdf::ObsDataFrameCols::getColumn(const std::string& name,
                                       std::vector<std::int64_t>& data) const {
  getColumn<std::int64_t>(name, data, consts::eInt64);
}

void osdf::ObsDataFrameCols::getColumn(const std::string& name,
                                       std::vector<float>& data) const {
  getColumn<float>(name, data, consts::eFloat);
}

void osdf::ObsDataFrameCols::getColumn(const std::string& name,
                                       std::vector<double>& data) const {
  getColumn<double>(name, data, consts::eDouble);
}

void osdf::ObsDataFrameCols::getColumn(const std::string& name,
                                       std::vector<std::string>& data) const {
  getColumn<std::string>(name, data, consts::eString);
}

void osdf::ObsDataFrameCols::setColumn(const std::string& name,
                                       const std::vector<std::int8_t>& data) const {
  setColumn<std::int8_t>(name, data, consts::eInt8);
}

void osdf::ObsDataFrameCols::setColumn(const std::string& name,
                                       const std::vector<std::int16_t>& data) const {
  setColumn<std::int16_t>(name, data, consts::eInt16);
}

void osdf::ObsDataFrameCols::setColumn(const std::string& name,
                                       const std::vector<std::int32_t>& data) const {
  setColumn<std::int32_t>(name, data, consts::eInt32);
}

void osdf::ObsDataFrameCols::setColumn(const std::string& name,
                                       const std::vector<std::int64_t>& data) const {
  setColumn<std::int64_t>(name, data, consts::eInt64);
}

void osdf::ObsDataFrameCols::setColumn(const std::string& name,
                                       const std::vector<float>& data) const {
  setColumn<float>(name, data, consts::eFloat);
}

void osdf::ObsDataFrameCols::setColumn(const std::string& name,
                                       const std::vector<double>& data) const {
  setColumn<double>(name, data, consts::eDouble);
}

void osdf::ObsDataFrameCols::setColumn(const std::string& name,
                                       const std::vector<std::string>& data) const {
  setColumn<std::string>(name, data, consts::eString);
}

template<typename T>
void osdf::ObsDataFrameCols::getDataValue(std::shared_ptr<DataBase> data,
                                          std::vector<T>& vars) const {
  std::shared_ptr<Data<T>> dataType = std::static_pointer_cast<Data<T>>(data);
  vars = dataType->getData();
}

template<typename T>
void osdf::ObsDataFrameCols::setDataValue(std::shared_ptr<DataBase> data,
                                          const std::vector<T>& vars) const {
  std::shared_ptr<Data<T>> dataType = std::static_pointer_cast<Data<T>>(data);
  dataType->setData(vars);
}

void osdf::ObsDataFrameCols::removeColumn(const std::string& name) {
  std::int32_t columnIndex = columnMetadata_.getIndex(name);
  if (columnIndex != consts::kErrorValue)  {
    std::int8_t permission = columnMetadata_.getPermission(columnIndex);
    if (permission == consts::eReadWrite) {
      columnMetadata_.remove(columnIndex);
      dataColumns_.erase(std::next(dataColumns_.begin(), columnIndex));
    } else {
      oops::Log::error() << "ERROR: The column \"" << name
                         << "\" is set to read-only." << std::endl;
    }
  } else {
    oops::Log::error() << "ERROR: Column named \"" << name << "\" not found in current data frame."
                       << std::endl;
  }
}

void osdf::ObsDataFrameCols::removeRow(const std::int64_t& index) {
  if (index >= 0 && index < (std::int64_t)ids_.size()) {
    std::int8_t canRemove = true;
    for (const ColumnMetadatum& columnMetadatum : columnMetadata_.get()) {
      std::string columnName = columnMetadatum.getName();
      std::int8_t permission = columnMetadatum.getPermission();
      if (permission == consts::eReadOnly) {
        oops::Log::error() << "ERROR: The column \"" << columnName << "\" is set to read-only."
                           << std::endl;
        canRemove = false;
        break;
      }
    }
    if (canRemove == true) {
      ids_.erase(std::next(ids_.begin(), index));
      for (std::shared_ptr<DataBase>& data : dataColumns_) {
        std::int8_t type = data->getType();
        switch (type) {
          case consts::eInt8: {
            removeDatum<std::int8_t>(data, index);
            break;
          }
          case consts::eInt16: {
            removeDatum<std::int16_t>(data, index);
            break;
          }
          case consts::eInt32: {
            removeDatum<std::int32_t>(data, index);
            break;
          }
          case consts::eInt64: {
            removeDatum<std::int64_t>(data, index);
            break;
          }
          case consts::eFloat: {
            removeDatum<float>(data, index);
            break;
          }
          case consts::eDouble: {
            removeDatum<double>(data, index);
            break;
          }
          case consts::eString: {
            removeDatum<std::string>(data, index);
            break;
          }
        }
      }
    }
  } else {
    oops::Log::error() << "ERROR: Row index is incompatible with current data frame." << std::endl;
  }
}

void osdf::ObsDataFrameCols::sort(const std::string& columnName, const std::int8_t order) {
  std::int8_t columnsAreWriteable = true;
  for (const ColumnMetadatum& columnMetadatum : columnMetadata_.get()) {
    columnsAreWriteable = columnMetadatum.getPermission();
    if (columnsAreWriteable == false) {
      break;
    }
  }
  if (columnsAreWriteable == true) {
    std::int32_t columnIndex = columnMetadata_.getIndex(columnName);
    if (columnIndex != consts::kErrorValue)  {
      std::int64_t numberOfRows = ids_.size();
      std::vector<std::int64_t> indices(numberOfRows, 0);
      std::iota(std::begin(indices), std::end(indices), 0);  // Build initial list of indices.

      std::shared_ptr<DataBase> data = dataColumns_.at(columnIndex);
      switch (data->getType()) {
        case consts::eInt8: {
          populateIndices<std::int8_t>(indices, funcs::getData<std::int8_t>(data), order);
          break;
        }
        case consts::eInt16: {
          populateIndices<std::int16_t>(indices, funcs::getData<std::int16_t>(data), order);
          break;
        }
        case consts::eInt32: {
          populateIndices<std::int32_t>(indices, funcs::getData<std::int32_t>(data), order);
          break;
        }
        case consts::eInt64: {
          populateIndices<std::int64_t>(indices, funcs::getData<std::int64_t>(data), order);
          break;
        }
        case consts::eFloat: {
          populateIndices<float>(indices, funcs::getData<float>(data), order);
          break;
        }
        case consts::eDouble: {
          populateIndices<double>(indices, funcs::getData<double>(data), order);
          break;
        }
        case consts::eString: {
          populateIndices<std::string>(indices, funcs::getData<std::string>(data), order);
          break;
        }
      }
      for (std::shared_ptr<DataBase>& data : dataColumns_) {
        switch (data->getType()) {
          case consts::eInt8: {
            swapData<std::int8_t>(indices, getData<std::int8_t>(data));
            break;
          }
          case consts::eInt16: {
            swapData<std::int16_t>(indices, getData<std::int16_t>(data));
            break;
          }
          case consts::eInt32: {
            swapData<std::int32_t>(indices, getData<std::int32_t>(data));
            break;
          }
          case consts::eInt64: {
            swapData<std::int64_t>(indices, getData<std::int64_t>(data));
            break;
          }
          case consts::eFloat: {
            swapData<float>(indices, getData<float>(data));
            break;
          }
          case consts::eDouble: {
            swapData<double>(indices, getData<double>(data));
            break;
          }
          case consts::eString: {
            swapData<std::string>(indices, getData<std::string>(data));
            break;
          }
        }
      }
    } else {
      oops::Log::error() << "ERROR: Column named \"" << columnName
                         << "\" not found in current data frame." << std::endl;
    }
  } else {
    oops::Log::error() << "ERROR: One or more columns in the current "
                          "data table are set to read-only." << std::endl;
  }
}

std::shared_ptr<osdf::ObsDataFrame> osdf::ObsDataFrameCols::slice(const std::string& columnName,
                                                                  const std::int8_t& comparison,
                                                                  const std::int8_t& threshold) {
  return slice(columnName, comparison, threshold, consts::eInt8);
}

std::shared_ptr<osdf::ObsDataFrame> osdf::ObsDataFrameCols::slice(const std::string& columnName,
                                                                  const std::int8_t& comparison,
                                                                  const std::int16_t& threshold) {
  return slice(columnName, comparison, threshold, consts::eInt16);
}

std::shared_ptr<osdf::ObsDataFrame> osdf::ObsDataFrameCols::slice(const std::string& columnName,
                                                                  const std::int8_t& comparison,
                                                                  const std::int32_t& threshold) {
  return slice(columnName, comparison, threshold, consts::eInt32);
}

std::shared_ptr<osdf::ObsDataFrame> osdf::ObsDataFrameCols::slice(const std::string& columnName,
                                                                  const std::int8_t& comparison,
                                                                  const std::int64_t& threshold) {
  return slice(columnName, comparison, threshold, consts::eInt64);
}

std::shared_ptr<osdf::ObsDataFrame> osdf::ObsDataFrameCols::slice(const std::string& columnName,
                                                                  const std::int8_t& comparison,
                                                                  const float& threshold) {
  return slice(columnName, comparison, threshold, consts::eFloat);
}

std::shared_ptr<osdf::ObsDataFrame> osdf::ObsDataFrameCols::slice(const std::string& columnName,
                                                                  const std::int8_t& comparison,
                                                                  const double& threshold) {
  return slice(columnName, comparison, threshold, consts::eDouble);
}

std::shared_ptr<osdf::ObsDataFrame> osdf::ObsDataFrameCols::slice(const std::string& columnName,
                                                                  const std::int8_t& comparison,
                                                                  const std::string& threshold) {
  return slice(columnName, comparison, threshold, consts::eString);
}

template <typename T>
void osdf::ObsDataFrameCols::appendNewColumn(const std::string& name, const std::vector<T>& values,
                                             const std::int8_t type) {
  std::int8_t columnExists = columnMetadata_.exists(name);
  if (columnExists == false) {
    if (values.size() != 0) {
      if (ids_.size() == 0) {
        initialise(values.size());
      }
      if (ids_.size() == values.size()) {
        std::int32_t columnIndex = columnMetadata_.add(ColumnMetadatum(name, type));
        std::shared_ptr<DataBase> data = osdf::funcs::createData(columnIndex, values);
        dataColumns_.push_back(data);
      } else {
        oops::Log::error() << "ERROR: Number of rows in new column incompatible "
                              "with current ObsDataFrameCols." << std::endl;
      }
    } else {
      oops::Log::error() << "ERROR: No values present in data vector." << std::endl;
    }
  } else {
    oops::Log::error() << "ERROR: A column named \"" + name + "\" already exists." << std::endl;
  }
}

template<typename T>
void osdf::ObsDataFrameCols::addColumnToRow(DataRow& row, std::int8_t& isValid, const T param) {
  if (isValid == true) {
    std::int32_t columnIndex = row.getSize();
    std::string name = columnMetadata_.getName(columnIndex);
    std::int8_t permission = columnMetadata_.getPermission(columnIndex);
    if (permission == consts::eReadWrite) {
      std::int8_t type = columnMetadata_.getType(columnIndex);
      std::shared_ptr<DatumBase> newDatum = funcs::createDatum(columnIndex, param);
      columnMetadata_.updateColumnWidth(columnIndex, newDatum->getDatumStr().size());
      if (newDatum->getType() == type) {
        row.insert(newDatum);
      } else {
        oops::Log::error() << "ERROR: Data type for \"" << param
                           << "\" is incompatible with the column \"" <<
                     name << "\" of current ObsDataFrameCols" << std::endl;
        isValid = false;
      }
    } else {
      oops::Log::error() << "ERROR: The column \"" << name
                         << "\" is set to read-only." << std::endl;
      isValid = false;
    }
  }
}

template void osdf::ObsDataFrameCols::addColumnToRow<std::int8_t>(
              DataRow&, std::int8_t&, const std::int8_t);
template void osdf::ObsDataFrameCols::addColumnToRow<std::int16_t>(
              DataRow&, std::int8_t&, const std::int16_t);
template void osdf::ObsDataFrameCols::addColumnToRow<std::int32_t>(
              DataRow&, std::int8_t&, const std::int32_t);
template void osdf::ObsDataFrameCols::addColumnToRow<std::int64_t>(
              DataRow&, std::int8_t&, const std::int64_t);
template void osdf::ObsDataFrameCols::addColumnToRow<float>(
              DataRow&, std::int8_t&, const float);
template void osdf::ObsDataFrameCols::addColumnToRow<double>(
              DataRow&, std::int8_t&, const double);
template void osdf::ObsDataFrameCols::addColumnToRow<std::string>(
              DataRow&, std::int8_t&, const std::string);
template void osdf::ObsDataFrameCols::addColumnToRow<const char*>(
              DataRow&, std::int8_t&, const char*);

template<typename T>
void osdf::ObsDataFrameCols::getColumn(const std::string& name, std::vector<T>& data,
                                       const std::int8_t type) const {
  std::int64_t columnIndex = columnMetadata_.getIndex(name);
  if (columnIndex != consts::kErrorValue)  {
    std::int8_t columnType = columnMetadata_.getType(columnIndex);
    if (type == columnType) {
      std::int64_t numberOfRows = ids_.size();
      data.resize(numberOfRows);
      getDataValue(dataColumns_.at(columnIndex), data);
    } else {
      oops::Log::error() << "ERROR: Input vector for column \"" << name
                         << "\" is not the required data type." << std::endl;
    }
  } else {
    oops::Log::error() << "ERROR: Column named \"" << name
                       << "\" not found in current data frame." << std::endl;
  }
}

template<typename T>
void osdf::ObsDataFrameCols::setColumn(const std::string& name, const std::vector<T>& data,
                                 const std::int8_t type) const {
  std::int64_t columnIndex = columnMetadata_.getIndex(name);
  if (columnIndex != consts::kErrorValue)  {
    std::int8_t permission = columnMetadata_.getPermission(columnIndex);
    if (permission == consts::eReadWrite) {
      std::int8_t columnType = columnMetadata_.getType(columnIndex);
      if (type == columnType) {
        std::int64_t numberOfRows = ids_.size();
        if ((std::int64_t)data.size() == numberOfRows) {
          setDataValue(dataColumns_.at(columnIndex), data);
        } else {
          oops::Log::error() << "ERROR: Input vector for column \"" << name
                             << "\" is not the required size." << std::endl;
        }
      } else {
        oops::Log::error() << "ERROR: Input vector for column \"" << name
                           << "\" is not the required data type." << std::endl;
      }
    } else {
      oops::Log::error() << "ERROR: The column \"" << name
                         << "\" is set to read-only." << std::endl;
    }
  } else {
    oops::Log::error() << "ERROR: Column named \"" << name
                       << "\" not found in current data frame." << std::endl;
  }
}

template<typename T>
std::shared_ptr<osdf::ObsDataFrameCols> osdf::ObsDataFrameCols::slice(const std::string& columnName,
                                                                      const std::int8_t& comparison,
                                                                      const T& threshold,
                                                                      const std::int8_t& type) {
  std::vector<std::shared_ptr<DataBase>> newDataColumns;
  newDataColumns.reserve(dataColumns_.size());
  std::int32_t columnIndex = columnMetadata_.getIndex(columnName);
  ColumnMetadata newColumnMetadata = columnMetadata_;
  newColumnMetadata.resetMaxId();  // Only relevant for column alignment when printing
  if (columnIndex != consts::kErrorValue)  {
    if (type == columnMetadata_.getType(columnIndex)) {
      std::shared_ptr<DataBase> data = dataColumns_.at(columnIndex);
      std::vector<std::int64_t> indices;
      indices.reserve(data->getSize());
      std::vector<T> values;
      getDataValue(data, values);
      for (std::int64_t rowIndex = 0; rowIndex < (std::int64_t)ids_.size(); ++rowIndex) {
        T value = values.at(rowIndex);
        if (compareDatumToThreshold(comparison, threshold, value)) {
          indices.push_back(rowIndex);
        }
      }
      indices.shrink_to_fit();
      for (const std::shared_ptr<DataBase>& data : dataColumns_) {
        switch (data->getType()) {
          case consts::eInt8: {
            sliceData<std::int8_t>(data, indices, newDataColumns);
            break;
          }
          case consts::eInt16: {
            sliceData<std::int16_t>(data, indices, newDataColumns);
            break;
          }
          case consts::eInt32: {
            sliceData<std::int32_t>(data, indices, newDataColumns);
            break;
          }
          case consts::eInt64: {
            sliceData<std::int64_t>(data, indices, newDataColumns);
            break;
          }
          case consts::eFloat: {
            sliceData<float>(data, indices, newDataColumns);
            break;
          }
          case consts::eDouble: {
            sliceData<double>(data, indices, newDataColumns);
            break;
          }
          case consts::eString: {
            sliceData<std::string>(data, indices, newDataColumns);
            break;
          }
        }
      }
    } else {
      oops::Log::error() << "ERROR: Column and threshold data type misconfiguration." << std::endl;
    }
  } else {
    oops::Log::error() << "ERROR: Column named \"" << columnName
                       << "\" not found in current data frame." << std::endl;
  }
  return std::make_shared<ObsDataFrameCols>(newColumnMetadata, newDataColumns);
}

template<typename T> void osdf::ObsDataFrameCols::clearData(std::shared_ptr<DataBase>& data) {
  std::shared_ptr<Data<T>> dataType = std::static_pointer_cast<Data<T>>(data);
  dataType->clear();
}

void osdf::ObsDataFrameCols::clear() {
  for (std::shared_ptr<DataBase>& data : dataColumns_) {
    switch (data->getType()) {
      case consts::eInt8: {
        clearData<std::int8_t>(data);
        break;
      }
      case consts::eInt16: {
        clearData<std::int16_t>(data);
        break;
      }
      case consts::eInt32: {
        clearData<std::int32_t>(data);
        break;
      }
      case consts::eInt64: {
        clearData<std::int64_t>(data);
        break;
      }
      case consts::eFloat: {
        clearData<float>(data);
        break;
      }
      case consts::eDouble: {
        clearData<double>(data);
        break;
      }
      case consts::eString: {
        clearData<std::string>(data);
        break;
      }
    }
  }
  dataColumns_.clear();
  ids_.clear();
  columnMetadata_.clear();
}

void osdf::ObsDataFrameCols::print() {
  if (dataColumns_.size() > 0) {
    std::string maxRowIdString = std::to_string(columnMetadata_.getMaxId());
    std::int32_t maxRowIdStringSize = maxRowIdString.size();
    columnMetadata_.print(maxRowIdStringSize);
    std::int32_t numColumns = (std::int32_t)dataColumns_.size();
    for (std::int64_t rowIndex = 0; rowIndex < (std::int64_t)ids_.size(); ++rowIndex) {
      oops::Log::info() << padString(std::to_string(ids_.at(rowIndex)), maxRowIdStringSize);
      for (std::int32_t columnIndex = 0; columnIndex < numColumns; ++columnIndex) {
        oops::Log::info() << consts::kBigSpace
                          << padString(dataColumns_.at(columnIndex)->getDatumStr(rowIndex),
                                       columnMetadata_.get(columnIndex).getWidth());
      }
      oops::Log::info() << std::endl;
    }
  }
}

const std::int64_t osdf::ObsDataFrameCols::getNumRows() const {
  return ids_.size();
}

const std::vector<std::shared_ptr<osdf::DataBase>>& osdf::ObsDataFrameCols::getDataColumns() const {
  return dataColumns_;
}

template<typename T>
void osdf::ObsDataFrameCols::populateIndices(std::vector<std::int64_t>& indices,
                                       const std::vector<T>& data, const std::int8_t order) {
  if (order == consts::eAscending) {
    std::sort(std::begin(indices), std::end(indices), [&](const std::int64_t& i,
              const std::int64_t& j) {
      return data.at(i) < data.at(j);
    });
  } else if (order == consts::eDescending) {
    std::sort(std::begin(indices), std::end(indices), [&](const std::int64_t& i,
              const std::int64_t& j) {
      return data.at(j) < data.at(i);
    });
  }
}

template<typename T>
void osdf::ObsDataFrameCols::swapData(std::vector<std::int64_t>& indices, std::vector<T>& data) {
  for (std::int64_t i = 0; i < (std::int64_t)ids_.size(); ++i) {
    while (indices.at(i) != indices.at(indices.at(i))) {
      std::swap(data.at(indices.at(i)), data.at(indices.at(indices.at(i))));
      std::swap(indices.at(i), indices.at(indices.at(i)));
    }
  }
}

template<typename T>
const std::int8_t osdf::ObsDataFrameCols::compareDatumToThreshold(const std::int8_t comparison,
                                                                  const T threshold,
                                                                  const T datumValue) const {
  switch (comparison) {
    case consts::eLessThan: return datumValue < threshold;
    case consts::eLessThanOrEqualTo: return datumValue <= threshold;
    case consts::eEqualTo: return datumValue == threshold;
    case consts::eGreaterThan: return datumValue > threshold;
    case consts::eGreaterThanOrEqualTo: return datumValue >= threshold;
    default: throw std::runtime_error("ERROR: Invalid comparison operator specification.");
  }
}

template const std::int8_t osdf::ObsDataFrameCols::compareDatumToThreshold<std::int8_t>(
                           const std::int8_t, const std::int8_t, const std::int8_t) const;
template const std::int8_t osdf::ObsDataFrameCols::compareDatumToThreshold<std::int16_t>(
                           const std::int8_t, const std::int16_t, const std::int16_t) const;
template const std::int8_t osdf::ObsDataFrameCols::compareDatumToThreshold<std::int32_t>(
                           const std::int8_t, const std::int32_t, const std::int32_t) const;
template const std::int8_t osdf::ObsDataFrameCols::compareDatumToThreshold<std::int64_t>(
                           const std::int8_t, const std::int64_t, const std::int64_t) const;
template const std::int8_t osdf::ObsDataFrameCols::compareDatumToThreshold<float>(
                           const std::int8_t, const float, const float) const;
template const std::int8_t osdf::ObsDataFrameCols::compareDatumToThreshold<double>(
                           const std::int8_t, const double, const double) const;
template const std::int8_t osdf::ObsDataFrameCols::compareDatumToThreshold<std::string>(
                           const std::int8_t, const std::string, const std::string) const;

template<typename T> void osdf::ObsDataFrameCols::sliceData(const std::shared_ptr<DataBase>& data,
      std::vector<std::int64_t>& indices, std::vector<std::shared_ptr<DataBase>>& newDataColumns) {
  std::vector<T>& values = getData<T>(data);
  std::vector<T> newValues(indices.size());
  std::transform(indices.begin(), indices.end(), newValues.begin(), [values](size_t index) {
    return values[index];
  });
  std::shared_ptr<Data<T>> newData = std::make_shared<Data<T>>(newValues);
  newDataColumns.push_back(newData);
}

template<typename T> void osdf::ObsDataFrameCols::removeDatum(std::shared_ptr<DataBase>& data,
                                                              const std::int64_t& index) {
  std::shared_ptr<Data<T>> dataType = std::static_pointer_cast<Data<T>>(data);
  dataType->removeDatum(index);
}

template<typename T> void osdf::ObsDataFrameCols::addDatum(std::shared_ptr<DataBase>& data,
                                                           std::shared_ptr<DatumBase>& datum) {
  std::shared_ptr<Data<T>> dataType = std::static_pointer_cast<Data<T>>(data);
  std::shared_ptr<Datum<T>> datumType = std::static_pointer_cast<Datum<T>>(datum);
  dataType->addDatum(datumType->getDatum());
}

template<typename T> void osdf::ObsDataFrameCols::construct(std::shared_ptr<DatumBase>& datum,
                                                            std::int8_t& initColumn,
                                                            std::int64_t numRows,
                                                            std::int32_t columnIndex) {
  std::shared_ptr<Datum<T>> datumType = std::static_pointer_cast<Datum<T>>(datum);
  std::shared_ptr<Data<T>> dataType;
  if (initColumn == false) {
    std::shared_ptr<DataBase> data = dataColumns_.at(columnIndex);
    dataType = std::static_pointer_cast<Data<T>>(data);
  } else {
    std::vector<T> values;
    dataType = std::make_shared<Data<T>>(values);
    dataType->reserve(numRows);
    dataColumns_.push_back(dataType);
  }
  columnMetadata_.updateColumnWidth(columnIndex, datum->getDatumStr().size());
  dataType->addDatum(datumType->getDatum());
}

template<typename T>
std::vector<T>& osdf::ObsDataFrameCols::getData(std::shared_ptr<DataBase> data) const {
  std::shared_ptr<Data<T>> dataType = std::static_pointer_cast<Data<T>>(data);
  return dataType->getData();
}

void osdf::ObsDataFrameCols::initialise(const std::int64_t& numRows) {
  for (std::int64_t index = 0; index < numRows; ++index) {
    ids_.push_back(index);
  }
  columnMetadata_.updateMaxId(ids_.size() - 1);
}
