/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/containers/FrameCols.h"
#include <string>

#include "ColumnMetadata.h"
#include "eckit/exception/Exceptions.h"
#include "ioda/containers/Constants.h"
#include "ioda/containers/FrameRows.h"
#include "ioda/containers/FrameUtils.h"

osdf::FrameCols::FrameCols(const ColumnMetadata& columnMetadata,
                           const std::vector<std::int64_t>& ids,
                           const std::vector<std::shared_ptr<DataBase>>& dataColumns) :
      IFrame(), data_(funcs_, columnMetadata, ids, dataColumns) {}

osdf::FrameCols::FrameCols() :
      IFrame(), data_(funcs_) {}


osdf::FrameCols::FrameCols(const FrameRows& frameRows) :
      IFrame(), data_(funcs_) {
  const std::int64_t sizeRows = frameRows.getData().getSizeRows();
  // Create metadata - columns are read-write and do not inherit any read-only permissions
  const ColumnMetadata& columnMetadata = frameRows.getData().getColumnMetadata();
  std::vector<osdf::ColumnMetadatum> columnVector;
  columnVector.reserve(static_cast<std::size_t>(columnMetadata.getSizeCols()));
  for (const ColumnMetadatum& columnMetadatum : columnMetadata.get()) {
    const std::string name = columnMetadatum.getName();
    const std::int8_t type = columnMetadatum.getType();
    ColumnMetadatum thisColumnMetadatum(name, type);
    columnVector.push_back(thisColumnMetadatum);
  }
  data_.configColumns(columnVector);
  // Create data
  for (const DataRow& dataRow : frameRows.getData().getDataRows()) {
    data_.appendNewRow(dataRow);
  }
  data_.getColumnMetadata().resetMaxId();
  data_.initialise(sizeRows);
}

osdf::FrameCols::~FrameCols() {
  clear();
}

/// Interface overrides

void osdf::FrameCols::configColumns(const std::vector<ColumnMetadatum> cols) {
  data_.configColumns(cols);
}

void osdf::FrameCols::configColumns(const std::initializer_list<ColumnMetadatum> initList) {
  data_.configColumns(initList);
}

void osdf::FrameCols::appendNewColumn(const std::string& name,
                                      const std::vector<int>& values) {
  appendNewColumn(name, values, consts::eInt);
}

void osdf::FrameCols::appendNewColumn(const std::string& name,
                                      const std::vector<std::int64_t>& values) {
  appendNewColumn(name, values, consts::eInt64);
}

void osdf::FrameCols::appendNewColumn(const std::string& name,
                                      const std::vector<float>& values) {
  appendNewColumn(name, values, consts::eFloat);
}

void osdf::FrameCols::appendNewColumn(const std::string& name,
                                      const std::vector<char>& values) {
  appendNewColumn(name, values, consts::eChar);
}

void osdf::FrameCols::appendNewColumn(const std::string& name,
                                      const std::vector<std::string>& values) {
  appendNewColumn(name, values, consts::eString);
}

void osdf::FrameCols::getColumn(const std::string& name, std::vector<int>& values) const {
  getColumn<int>(name, values, consts::eInt);
}

void osdf::FrameCols::getColumn(const std::string& name, std::vector<std::int64_t>& values) const {
  getColumn<std::int64_t>(name, values, consts::eInt64);
}

void osdf::FrameCols::getColumn(const std::string& name, std::vector<float>& values) const {
  getColumn<float>(name, values, consts::eFloat);
}

void osdf::FrameCols::getColumn(const std::string& name, std::vector<char>& values) const {
  getColumn<char>(name, values, consts::eChar);
}

void osdf::FrameCols::getColumn(const std::string& name, std::vector<std::string>& values) const {
  getColumn<std::string>(name, values, consts::eString);
}

void osdf::FrameCols::setColumn(const std::string& name,
                                const std::vector<int>& values) const {
  setColumn<int>(name, values, consts::eInt);
}

void osdf::FrameCols::setColumn(const std::string& name,
                                const std::vector<std::int64_t>& values) const {
  setColumn<std::int64_t>(name, values, consts::eInt64);
}

void osdf::FrameCols::setColumn(const std::string& name,
                                const std::vector<float>& values) const {
  setColumn<float>(name, values, consts::eFloat);
}

void osdf::FrameCols::setColumn(const std::string& name,
                                const std::vector<char>& values) const {
  setColumn<char>(name, values, consts::eChar);
}

void osdf::FrameCols::setColumn(const std::string& name,
                                const std::vector<std::string>& values) const {
  setColumn<std::string>(name, values, consts::eString);
}

void osdf::FrameCols::removeColumn(const std::string& name) {
  // no need to check if column with name exists, getIndex throws exception if not.
  const std::int32_t index = data_.getIndex(name);
  const std::int8_t permission = data_.getPermission(index);
  if (permission != consts::eReadWrite) {
    const std::string errMsg
      = std::string("ERROR: Column named ") + name + std::string(" is set to read-only.");
    throw eckit::BadParameter(errMsg, Here());
  }
  data_.removeColumn(index);
  notify();
}

void osdf::FrameCols::removeColumn(const std::int32_t index) {
  const std::int8_t permission = data_.getPermission(index);
  if (permission != consts::eReadWrite) {
    const std::string errMsg = std::string("ERROR: Column at index ") + std::to_string(index)
                               + std::string(" is set to read-only.");
    throw eckit::BadParameter(errMsg, Here());
  }
  data_.removeColumn(index);
  notify();
}

std::int8_t osdf::FrameCols::getColumnType(const std::string& name) const {
  return data_.getType(data_.getIndex(name));
}

void osdf::FrameCols::removeRow(const std::int64_t index) {
  if (index < 0 || index >= data_.getSizeRows()) {
    const std::string errMsg = std::string("Error: Row index ") + std::to_string(index)
                               + std::string(" is incompatible with current data frame");
    throw eckit::OutOfRange(errMsg, Here());
  }

  for (std::int32_t colIndex = 0; colIndex < data_.getSizeCols(); ++colIndex) {
    const std::int8_t permission = data_.getPermission(colIndex);
    if (permission == consts::eReadOnly) {
      const std::string errMsg = std::string("ERROR: Cannot remove row. Column ")
                                 + data_.getName(colIndex)
                                 + std::string("is set to read-only.");
      throw eckit::BadParameter(errMsg, Here());
    }
  }
  data_.removeRow(index);
  notify();
}

void osdf::FrameCols::removeRows(const std::vector<bool>& keepRows) {
  if (keepRows.size() != static_cast<std::size_t>(data_.getSizeRows())) {
    const std::string errMsg = std::string("keepRows vector size does not match ")
                               + std::string("the number of rows in the current data frame.");
    throw eckit::BadParameter(errMsg, Here());
  }

  for (std::int64_t i = (keepRows.size() - 1); i >= 0; --i) {
    if (!keepRows[i]) {
        removeRow(i);
    }
  }
}

void osdf::FrameCols::sortRows(const std::string& columnName, const std::int8_t order) {
  if (data_.columnExists(columnName) != true) {
    const std::string errMsg = std::string("ERROR: Column named ") + columnName
                               + std::string(" not found in current data frame.");
    throw eckit::BadParameter(errMsg, Here());
  }

  for (std::int32_t colIndex = 0; colIndex < data_.getSizeCols(); ++colIndex) {
    const std::int8_t permission = data_.getPermission(colIndex);
    if (permission == consts::eReadOnly) {
      const std::string errMsg = std::string("ERROR: Column named ") + data_.getName(colIndex)
                                 + std::string(" is set to read-only.");
      throw eckit::BadParameter(errMsg, Here());
    }
  }
  // Build list of ordered indices.
  const std::int32_t index = data_.getIndex(columnName);
  const std::int64_t sizeRows = data_.getSizeRows();
  std::vector<std::int64_t> indices(static_cast<std::size_t>(sizeRows), 0);
  std::iota(std::begin(indices), std::end(indices), 0);  // Initial sequential list of indices.
  const std::shared_ptr<osdf::DataBase>& dataColRead = data_.getDataColumn(index);
  osdf::FrameUtils::callWithSupportedType(
    dataColRead->getType(),
    [&](auto typeDiscriminator) {
      using T = decltype(typeDiscriminator);
      const std::vector<T>& values = funcs_.getDataValues<T>(dataColRead);
      funcs_.sequenceIndices<T>(indices, values, order);
    });
  // Swap data values for each individual column
  funcs_.reorderValues(indices, data_.getIds());
  for (std::int32_t colIndex = 0; colIndex < data_.getSizeCols(); ++colIndex) {
    std::shared_ptr<osdf::DataBase>& dataColWrite = data_.getDataColumn(colIndex);
    osdf::FrameUtils::callWithSupportedType(
      dataColWrite->getType(),
      [&](auto typeDiscriminator) {
        using T = decltype(typeDiscriminator);
        std::vector<T>& values = funcs_.getDataValues<T>(dataColWrite);
        funcs_.reorderValues<T>(indices, values);
      });
  }
  notify();
}

std::vector<std::string> osdf::FrameCols::columnNames() const {
  return data_.getColumnMetadata().columnNames();
}

std::string osdf::FrameCols::serializeColumnMetadata() const {
  return FrameUtils::serializeColumnMetadata(data_.getColumnMetadata().get());
}

void osdf::FrameCols::deserializeColumnMetadata(const std::string & columnMetadataTokens) {
  // Don't allow deserialization into non-empty ColumnMetadata
  if (data_.getColumnMetadata().get().size() != 0) {
    const std::string errMsg =
      std::string("ERROR: Column metadata can only be deserialized into an empty container.");
    throw eckit::BadValue(errMsg, Here());
  }
  const std::vector<ColumnMetadatum> columnMetadata =
          FrameUtils::deserializeColumnMetadataTokens(columnMetadataTokens);
  data_.configColumns(columnMetadata);
}

void osdf::FrameCols::append(const std::unique_ptr<IFrame>& srcOsdf) {
  const std::int32_t numParams = data_.getSizeCols();

  // Check if it is possible to append to the current OSDF
  if (!(numParams > 0)) {
    const std::string errMsg
      = std::string("Error: Cannot append to this OSDF without first setting column headings.");
    throw eckit::BadParameter(errMsg, Here());
  }

  const osdf::ColumnMetadata& srcColumnMetadata = srcOsdf->getData().getColumnMetadata();
  const bool canWriteTo = data_.canWriteAllData();
  if (canWriteTo == false) {
    const std::string errMsg = std::string(
      "Error: Unable to append to an OSDF container containing columns with ReadOnly "
      "permissions.");
    throw eckit::BadParameter(errMsg, Here());
  }

  // Check if the srcOSDF is compatible with the current OSDF
  const bool validColumnMetadata = data_.compareColumnMetadata(srcColumnMetadata);
  if (validColumnMetadata == false) {
    const std::string errMsg
      = std::string("Error: Unable to append two OSDF containers with different column metadata.");
    throw eckit::BadParameter(errMsg, Here());
  }

  // Append to the current OSDF
  std::vector<osdf::DataRow> dataRowsToAppend;
  dataRowsToAppend.reserve(srcOsdf->getData().getSizeRows());
  srcOsdf->getData().getDataRows(dataRowsToAppend);

  for (const osdf::DataRow& dataRow : dataRowsToAppend) {
    DataRow newDataRow(data_.getMaxId() + 1);
    for (std::int32_t index = 0; index < dataRow.getSize(); ++index) {
      newDataRow.insert(dataRow.getColumn(index));
    }
    data_.appendNewRow(newDataRow);
  }
  notify();
}

void osdf::FrameCols::print() const {
  data_.print();
}

void osdf::FrameCols::clear() {
  data_.clear();
  notify();
}

/// Other public functions

osdf::FrameCols osdf::FrameCols::sliceRows(const std::string& name, const std::int8_t comparison,
                                           const int threshold) const {
  return sliceRows<int>(name, comparison, threshold);
}

osdf::FrameCols osdf::FrameCols::sliceRows(const std::string& name, const std::int8_t comparison,
                                           const std::int64_t threshold) const {
  return sliceRows<std::int64_t>(name, comparison, threshold);
}

osdf::FrameCols osdf::FrameCols::sliceRows(const std::string& name, const std::int8_t comparison,
                                           const float threshold) const {
  return sliceRows<float>(name, comparison, threshold);
}

osdf::FrameCols osdf::FrameCols::sliceRows(const std::string& name, const std::int8_t comparison,
                                           const std::string threshold) const {
  return sliceRows<std::string>(name, comparison, threshold);
}

osdf::ViewCols osdf::FrameCols::makeView() {
  return ViewCols(data_.getColumnMetadata(), data_.getIds(), data_.getDataCols(), this);
}

void osdf::FrameCols::attach(ViewCols* view) {
  views_.push_back(view);
}

void osdf::FrameCols::detach(ViewCols* view) {
  auto it = std::find(views_.begin(), views_.end(), view);
  if (it != views_.end()) {
    views_.erase(it);
  }
}

const osdf::FrameColsData& osdf::FrameCols::getData() const {
  return data_;
}

/// Private functions

void osdf::FrameCols::notify() {
  for (ViewCols* view : views_) {
    if (view != nullptr) {
      view->setUpdatedObjects(data_.getColumnMetadata(), data_.getIds(), data_.getDataCols());
    }
  }
}

template <typename T>
void osdf::FrameCols::appendNewColumn(const std::string& name, const std::vector<T>& values,
                                      const std::int8_t type) {
  if (data_.columnExists(name) != false) {
    const std::string errMsg = std::string("ERROR: A column named ") + name
                               + std::string(" already exists.");
    throw eckit::BadParameter(errMsg, Here());
  }
  const std::int64_t valuesSize = static_cast<std::int64_t>(values.size());
  if (data_.getSizeRows() == 0 && data_.getColumnMetadata().getSizeCols() == 0) {
    data_.initialise(valuesSize);
  }

  if (valuesSize != data_.getSizeRows()) {
    const std::string errMsg = std::string(
      "ERROR: Number of rows in new column incompatible with current FrameCols.");
    throw eckit::BadParameter(errMsg, Here());
  }
  const std::shared_ptr<DataBase> data = funcs_.createData(values);
  const std::int32_t columnIndex = data_.getSizeCols();
  data_.appendNewColumn(data, name, type);
  if (data_.getSizeRows() > 0) {
    data_.updateColumnWidth(columnIndex, funcs_.getSize<T>(data));
  }
  notify();
}

template <typename T>
void osdf::FrameCols::getColumn(const std::string& name, std::vector<T>& values,
                                const std::int8_t type) const {
  // no need to check if column exists as getIndex throws error otherwise
  const std::int32_t columnIndex = data_.getIndex(name);
  const std::int8_t columnType   = data_.getType(columnIndex);

  if (type != columnType) {
    const std::string errMsg = std::string("ERROR: Input vector for column ")
                               + name
                               + std::string(" is not the required data type.");
    throw eckit::BadParameter(errMsg, Here());
  }
  const std::shared_ptr<DataBase>& dataCol = data_.getDataColumn(columnIndex);
  values = funcs_.getDataValues<T>(dataCol);
}

template<typename T>
void osdf::FrameCols::setColumn(const std::string& name, const std::vector<T>& values,
                                const std::int8_t type) const {
  // no need to check if column with name exists as getIndex throws exception
  const std::int32_t columnIndex = data_.getIndex(name);
  const std::int8_t permission   = data_.getPermission(columnIndex);

  if (permission != consts::eReadWrite) {
    const std::string errMsg = std::string("ERROR: The column ")
                               + name
                               + std::string(" is set to read-only.");
    throw eckit::BadParameter(errMsg, Here());
  }
  std::int8_t columnType = data_.getType(columnIndex);
  if (type != columnType) {
    const std::string errMsg = std::string("ERROR: Input vector for column ") + name
                               + std::string(" is not the required data type.");
    throw eckit::BadParameter(errMsg, Here());
  }
  const std::int64_t valuesSize = static_cast<std::int64_t>(values.size());
  if (valuesSize != data_.getSizeRows()) {
    const std::string errMsg = std::string("ERROR: Input vector for column ")
                               + name
                               + std::string(" is not the required size.");
    throw eckit::BadParameter(errMsg, Here());
  }
  const std::shared_ptr<DataBase>& data = data_.getDataColumn(columnIndex);
  funcs_.setDataValues(data, values);
}

template<typename T>
osdf::FrameCols osdf::FrameCols::sliceRows(const std::string& name, const std::int8_t comparison,
                                           const T threshold) const {
  std::vector<std::shared_ptr<DataBase>> newDataColumns;
  std::vector<std::int64_t> newIds;
  ColumnMetadata newColumnMetadata;
  if (data_.columnExists(name) != true) {
    const std::string errMsg = std::string("ERROR: Column named ")
                               + name
                               + std::string(" not found in current data frame.");
    throw eckit::BadParameter(errMsg, Here());
  }
  funcs_.sliceRows(&data_, newDataColumns, newColumnMetadata, newIds, name, comparison, threshold);
  return FrameCols(newColumnMetadata, newIds, newDataColumns);
}
