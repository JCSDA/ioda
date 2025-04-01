/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/containers/FrameCols.h"

#include "eckit/exception/Exceptions.h"

#include "ioda/containers/Constants.h"
#include "ioda/containers/Data.h"
#include "ioda/containers/DatumBase.h"
#include "ioda/containers/FrameRows.h"

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
                                      const std::vector<std::int8_t>& values) {
  appendNewColumn(name, values, consts::eInt8);
}

void osdf::FrameCols::appendNewColumn(const std::string& name,
                                      const std::vector<std::int16_t>& values) {
  appendNewColumn(name, values, consts::eInt16);
}

void osdf::FrameCols::appendNewColumn(const std::string& name,
                                      const std::vector<std::int32_t>& values) {
  appendNewColumn(name, values, consts::eInt32);
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
                                      const std::vector<double>& values) {
  appendNewColumn(name, values, consts::eDouble);
}

void osdf::FrameCols::appendNewColumn(const std::string& name,
                                      const std::vector<char>& values) {
  appendNewColumn(name, values, consts::eChar);
}

void osdf::FrameCols::appendNewColumn(const std::string& name,
                                      const std::vector<std::string>& values) {
  appendNewColumn(name, values, consts::eString);
}

void osdf::FrameCols::getColumn(const std::string& name, std::vector<std::int8_t>& values) const {
  getColumn<std::int8_t>(name, values, consts::eInt8);
}

void osdf::FrameCols::getColumn(const std::string& name, std::vector<std::int16_t>& values) const {
  getColumn<std::int16_t>(name, values, consts::eInt16);
}

void osdf::FrameCols::getColumn(const std::string& name, std::vector<std::int32_t>& values) const {
  getColumn<std::int32_t>(name, values, consts::eInt32);
}

void osdf::FrameCols::getColumn(const std::string& name, std::vector<std::int64_t>& values) const {
  getColumn<std::int64_t>(name, values, consts::eInt64);
}

void osdf::FrameCols::getColumn(const std::string& name, std::vector<float>& values) const {
  getColumn<float>(name, values, consts::eFloat);
}

void osdf::FrameCols::getColumn(const std::string& name, std::vector<double>& values) const {
  getColumn<double>(name, values, consts::eDouble);
}

void osdf::FrameCols::getColumn(const std::string& name, std::vector<char>& values) const {
  getColumn<char>(name, values, consts::eChar);
}

void osdf::FrameCols::getColumn(const std::string& name, std::vector<std::string>& values) const {
  getColumn<std::string>(name, values, consts::eString);
}

void osdf::FrameCols::setColumn(const std::string& name,
                                const std::vector<std::int8_t>& values) const {
  setColumn<std::int8_t>(name, values, consts::eInt8);
}

void osdf::FrameCols::setColumn(const std::string& name,
                                const std::vector<std::int16_t>& values) const {
  setColumn<std::int16_t>(name, values, consts::eInt16);
}

void osdf::FrameCols::setColumn(const std::string& name,
                                const std::vector<std::int32_t>& values) const {
  setColumn<std::int32_t>(name, values, consts::eInt32);
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
                                const std::vector<double>& values) const {
  setColumn<double>(name, values, consts::eDouble);
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
  if (data_.columnExists(name) == true) {
    const std::int32_t index = data_.getIndex(name);
    const std::int8_t permission = data_.getPermission(index);
    if (permission == consts::eReadWrite) {
      data_.removeColumn(index);
      notify();
    } else {
      oops::Log::error() << "ERROR: Column named \"" << name
                         << "\" is set to read-only." << std::endl;
    }
  } else {
    oops::Log::error() << "ERROR: Column named \"" << name
                       << "\" not found in current data frame." << std::endl;
  }
}

void osdf::FrameCols::removeColumn(const std::int32_t index) {
  const std::string& name = data_.getName(index);
  if (name == consts::kErrorReturnString) {
    const std::int8_t permission = data_.getPermission(index);
    if (permission == consts::eReadWrite) {
      data_.removeColumn(index);
      notify();
    } else {
      oops::Log::error() << "ERROR: Column at index \"" << index
                         << "\" is set to read-only." << std::endl;
    }
  } else {
    oops::Log::error() << "ERROR: Column at index \"" << index
                       << "\" not found in current data frame." << std::endl;
  }
}

std::int8_t osdf::FrameCols::getColumnType(const std::string& name) const {
  return data_.getType(data_.getIndex(name));
}

void osdf::FrameCols::removeRow(const std::int64_t index) {
  if (index >= 0 && index < data_.getSizeRows()) {
    std::int8_t canRemove = true;
    for (std::int32_t colIndex = 0; colIndex < data_.getSizeCols(); ++colIndex) {
      const std::int8_t permission = data_.getPermission(colIndex);
      if (permission == consts::eReadOnly) {
        oops::Log::error() << "ERROR: Cannot remove row. Column \"" << data_.getName(colIndex)
                           << "\" is set to read-only." << std::endl;
        canRemove = false;
        break;
      }
    }
    if (canRemove == true) {
      data_.removeRow(index);
      notify();
    }
  } else {
    oops::Log::error() << "ERROR: Row index \"" << index
                       << "\"is incompatible with current data frame." << std::endl;
  }
}

void osdf::FrameCols::removeRows(const std::vector<bool> & keepRows) {
  if (keepRows.size() == data_.getSizeRows()) {
    for (std::int64_t i = (keepRows.size() - 1); i >= 0; --i) {
      if (!keepRows[i]) {
          removeRow(i);
      }
    }
  } else {
    const std::string errMsg = std::string("keepRows vector size does not match ") +
        std::string("the number of rows in the current data frame.");
    throw eckit::BadValue(errMsg, Here());
  }
}

void osdf::FrameCols::sortRows(const std::string& columnName, const std::int8_t order) {
  if (data_.columnExists(columnName) == true) {
    std::int8_t canSort = true;
    for (std::int32_t colIndex = 0; colIndex < data_.getSizeCols(); ++colIndex) {
      const std::int8_t permission = data_.getPermission(colIndex);
      if (permission == consts::eReadOnly) {
        oops::Log::error() << "ERROR: Column named \"" << data_.getName(colIndex)
                           << "\" is set to read-only." << std::endl;
        canSort = false;
        break;
      }
    }
    if (canSort == true) {
      // Build list of ordered indices.
      const std::int32_t index = data_.getIndex(columnName);
      const std::int64_t sizeRows = data_.getSizeRows();
      std::vector<std::int64_t> indices(static_cast<std::size_t>(sizeRows), 0);
      std::iota(std::begin(indices), std::end(indices), 0);  // Initial sequential list of indices.
      const std::shared_ptr<osdf::DataBase>& dataColRead = data_.getDataColumn(index);
      switch (dataColRead->getType()) {
        case consts::eInt8: {
          const std::vector<std::int8_t>& values = funcs_.getDataValues<std::int8_t>(dataColRead);
          funcs_.sequenceIndices<std::int8_t>(indices, values, order);
          break;
        }
        case consts::eInt16: {
          const std::vector<std::int16_t>& values = funcs_.getDataValues<std::int16_t>(dataColRead);
          funcs_.sequenceIndices<std::int16_t>(indices, values, order);
          break;
        }
        case consts::eInt32: {
          const std::vector<std::int32_t>& values = funcs_.getDataValues<std::int32_t>(dataColRead);
          funcs_.sequenceIndices<std::int32_t>(indices, values, order);
          break;
        }
        case consts::eInt64: {
          const std::vector<std::int64_t>& values = funcs_.getDataValues<std::int64_t>(dataColRead);
          funcs_.sequenceIndices<std::int64_t>(indices, values, order);
          break;
        }
        case consts::eFloat: {
          const std::vector<float>& values = funcs_.getDataValues<float>(dataColRead);
          funcs_.sequenceIndices<float>(indices, values, order);
          break;
        }
        case consts::eDouble: {
          const std::vector<double>& values = funcs_.getDataValues<double>(dataColRead);
          funcs_.sequenceIndices<double>(indices, values, order);
          break;
        }
        case consts::eChar: {
          const std::vector<char>& values = funcs_.getDataValues<char>(dataColRead);
          funcs_.sequenceIndices<char>(indices, values, order);
          break;
        }
        case consts::eString: {
          const std::vector<std::string>& values = funcs_.getDataValues<std::string>(dataColRead);
          funcs_.sequenceIndices<std::string>(indices, values, order);
          break;
        }
      }
      // Swap data values for each individual column
      funcs_.reorderValues(indices, data_.getIds());
      for (std::int32_t colIndex = 0; colIndex < data_.getSizeCols(); ++colIndex) {
        std::shared_ptr<osdf::DataBase>& dataColWrite = data_.getDataColumn(colIndex);
        switch (dataColWrite->getType()) {
          case consts::eInt8: {
            std::vector<std::int8_t>& values = funcs_.getDataValues<std::int8_t>(dataColWrite);
            funcs_.reorderValues<std::int8_t>(indices, values);
            break;
          }
          case consts::eInt16: {
            std::vector<std::int16_t>& values = funcs_.getDataValues<std::int16_t>(dataColWrite);
            funcs_.reorderValues<std::int16_t>(indices, values);
            break;
          }
          case consts::eInt32: {
            std::vector<std::int32_t>& values = funcs_.getDataValues<std::int32_t>(dataColWrite);
            funcs_.reorderValues<std::int32_t>(indices, values);
            break;
          }
          case consts::eInt64: {
            std::vector<std::int64_t>& values = funcs_.getDataValues<std::int64_t>(dataColWrite);
            funcs_.reorderValues<std::int64_t>(indices, values);
            break;
          }
          case consts::eFloat: {
            std::vector<float>& values = funcs_.getDataValues<float>(dataColWrite);
            funcs_.reorderValues<float>(indices, values);
            break;
          }
          case consts::eDouble: {
            std::vector<double>& values = funcs_.getDataValues<double>(dataColWrite);
            funcs_.reorderValues<double>(indices, values);
            break;
          }
          case consts::eChar: {
            std::vector<char>& values = funcs_.getDataValues<char>(dataColWrite);
            funcs_.reorderValues<char>(indices, values);
            break;
          }
          case consts::eString: {
            std::vector<std::string>& values = funcs_.getDataValues<std::string>(dataColWrite);
            funcs_.reorderValues<std::string>(indices, values);
            break;
          }
        }
      }
      notify();
    }
  } else {
    oops::Log::error() << "ERROR: Column named \"" << columnName
                       << "\" not found in current data frame." << std::endl;
  }
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
                                           const std::int8_t threshold) const {
  return sliceRows<std::int8_t>(name, comparison, threshold);
}

osdf::FrameCols osdf::FrameCols::sliceRows(const std::string& name, const std::int8_t comparison,
                                           const std::int16_t threshold) const {
  return sliceRows<std::int16_t>(name, comparison, threshold);
}

osdf::FrameCols osdf::FrameCols::sliceRows(const std::string& name, const std::int8_t comparison,
                                           const std::int32_t threshold) const {
  return sliceRows<std::int32_t>(name, comparison, threshold);
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
                                           const double threshold) const {
  return sliceRows<double>(name, comparison, threshold);
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
  if (data_.columnExists(name) == false) {
    if (values.size() != 0) {
      const std::int64_t valuesSize = static_cast<std::int64_t>(values.size());
      if (data_.getSizeRows() == 0) {
        data_.initialise(valuesSize);
      }
      if (valuesSize == data_.getSizeRows()) {
        const std::shared_ptr<DataBase> data = funcs_.createData(values);
        const std::int32_t columnIndex = data_.getSizeCols();
        data_.appendNewColumn(data, name, type);
        data_.updateColumnWidth(columnIndex, funcs_.getSize<T>(data));
        notify();
      } else {
        oops::Log::error() << "ERROR: Number of rows in new column incompatible "
                              "with current FrameCols." << std::endl;
      }
    } else {
      oops::Log::error() << "ERROR: No values present in data vector." << std::endl;
    }
  } else {
    oops::Log::error() << "ERROR: A column named \"" + name + "\" already exists." << std::endl;
  }
}

template<typename T>
void osdf::FrameCols::getColumn(const std::string& name, std::vector<T>& values,
                                const std::int8_t type) const {
  if (data_.columnExists(name) == true)  {
    const std::int32_t columnIndex = data_.getIndex(name);
    const std::int8_t columnType = data_.getType(columnIndex);
    if (type == columnType) {
      const std::shared_ptr<DataBase>& dataCol = data_.getDataColumn(columnIndex);
      values = funcs_.getDataValues<T>(dataCol);
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
void osdf::FrameCols::setColumn(const std::string& name, const std::vector<T>& values,
                                const std::int8_t type) const {
  if (data_.columnExists(name) == true)  {
    const std::int32_t columnIndex = data_.getIndex(name);
    const std::int8_t permission = data_.getPermission(columnIndex);
    if (permission == consts::eReadWrite) {
      std::int8_t columnType = data_.getType(columnIndex);
      if (type == columnType) {
        const std::int64_t valuesSize = static_cast<std::int64_t>(values.size());
        if (valuesSize == data_.getSizeRows()) {
          const std::shared_ptr<DataBase>& data = data_.getDataColumn(columnIndex);
          funcs_.setDataValues(data, values);
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
osdf::FrameCols osdf::FrameCols::sliceRows(const std::string& name, const std::int8_t comparison,
                                           const T threshold) const {
  std::vector<std::shared_ptr<DataBase>> newDataColumns;
  std::vector<std::int64_t> newIds;
  ColumnMetadata newColumnMetadata;
  if (data_.columnExists(name) == true)  {
    funcs_.sliceRows(&data_, newDataColumns, newColumnMetadata,
                     newIds, name, comparison, threshold);
  } else {
    oops::Log::error() << "ERROR: Column named \"" << name
                       << "\" not found in current data frame." << std::endl;
  }
  return FrameCols(newColumnMetadata, newIds, newDataColumns);
}
