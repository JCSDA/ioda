/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/containers/FrameRows.h"

#include "ioda/containers/Constants.h"
#include "ioda/containers/Data.h"
#include "ioda/containers/DatumBase.h"
#include "ioda/containers/FrameCols.h"

osdf::FrameRows::FrameRows() :
      IFrame(), data_(funcs_) {}

osdf::FrameRows::FrameRows(const ColumnMetadata columnMetadata,
                           const std::vector<DataRow> dataRows) :
      IFrame(), data_(funcs_, columnMetadata, dataRows) {}

osdf::FrameRows::FrameRows(const FrameCols& frameCols) :
      IFrame(), data_(funcs_) {
  const std::int64_t sizeRows = frameCols.getData().getSizeRows();
  data_.initialise(sizeRows);
  // Create metadata - columns are read-write and do not inherit any read-only permissions
  const ColumnMetadata& columnMetadata = frameCols.getData().getColumnMetadata();
  for (const ColumnMetadatum& columnMetadatum : columnMetadata.get()) {
    std::string columnName = columnMetadatum.getName();
    std::int32_t columnIndex = columnMetadata.getIndex(columnName);
    const std::size_t colIdx = static_cast<std::size_t>(columnIndex);
    const std::shared_ptr<DataBase>& data = frameCols.getData().getDataCols().at(colIdx);
    const std::int8_t type = data->getType();
    switch (type) {
      case consts::eInt8: {
        const std::vector<std::int8_t>& values = funcs_.getDataValues<std::int8_t>(data);
        appendNewColumn<std::int8_t>(columnName, values, type);
        break;
      }
      case consts::eInt16: {
        const std::vector<std::int16_t>& values = funcs_.getDataValues<std::int16_t>(data);
        appendNewColumn<std::int16_t>(columnName, values, type);
        break;
      }
      case consts::eInt32: {
        const std::vector<std::int32_t>& values = funcs_.getDataValues<std::int32_t>(data);
        appendNewColumn<std::int32_t>(columnName, values, type);
        break;
      }
      case consts::eInt64: {
        const std::vector<std::int64_t>& values = funcs_.getDataValues<std::int64_t>(data);
        appendNewColumn<std::int64_t>(columnName, values, type);
        break;
      }
      case consts::eFloat: {
        const std::vector<float>& values = funcs_.getDataValues<float>(data);
        appendNewColumn<float>(columnName, values, type);
        break;
      }
      case consts::eDouble: {
        const std::vector<double>& values = funcs_.getDataValues<double>(data);
        appendNewColumn<double>(columnName, values, type);
        break;
      }
      case consts::eString: {
        const std::vector<std::string>& values = funcs_.getDataValues<std::string>(data);
        appendNewColumn<std::string>(columnName, values, type);
        break;
      }
    }
  }
}

/// Interface overrides

void osdf::FrameRows::configColumns(const std::vector<ColumnMetadatum> cols) {
  data_.configColumns(cols);
}

void osdf::FrameRows::configColumns(const std::initializer_list<ColumnMetadatum> initList) {
  data_.configColumns(initList);
}

void osdf::FrameRows::appendNewColumn(const std::string& name,
                                      const std::vector<std::int8_t>& values) {
  appendNewColumn(name, values, consts::eInt8);
}

void osdf::FrameRows::appendNewColumn(const std::string& name,
                                      const std::vector<std::int16_t>& values) {
  appendNewColumn(name, values, consts::eInt16);
}

void osdf::FrameRows::appendNewColumn(const std::string& name,
                                      const std::vector<std::int32_t>& values) {
  appendNewColumn(name, values, consts::eInt32);
}

void osdf::FrameRows::appendNewColumn(const std::string& name,
                                      const std::vector<std::int64_t>& values) {
  appendNewColumn(name, values, consts::eInt64);
}

void osdf::FrameRows::appendNewColumn(const std::string& name,
                                      const std::vector<float>& values) {
  appendNewColumn(name, values, consts::eFloat);
}

void osdf::FrameRows::appendNewColumn(const std::string& name,
                                      const std::vector<double>& values) {
  appendNewColumn(name, values, consts::eDouble);
}

void osdf::FrameRows::appendNewColumn(const std::string& name,
                                      const std::vector<std::string>& values) {
  appendNewColumn(name, values, consts::eString);
}

void osdf::FrameRows::getColumn(const std::string& name, std::vector<std::int8_t>& values) const {
  getColumn<std::int8_t>(name, values, consts::eInt8);
}

void osdf::FrameRows::getColumn(const std::string& name, std::vector<std::int16_t>& values) const {
  getColumn<std::int16_t>(name, values, consts::eInt16);
}

void osdf::FrameRows::getColumn(const std::string& name, std::vector<std::int32_t>& values) const {
  getColumn<std::int32_t>(name, values, consts::eInt32);
}

void osdf::FrameRows::getColumn(const std::string& name, std::vector<std::int64_t>& values) const {
  getColumn<std::int64_t>(name, values, consts::eInt64);
}

void osdf::FrameRows::getColumn(const std::string& name, std::vector<float>& values) const {
  getColumn<float>(name, values, consts::eFloat);
}

void osdf::FrameRows::getColumn(const std::string& name, std::vector<double>& values) const {
  getColumn<double>(name, values, consts::eDouble);
}

void osdf::FrameRows::getColumn(const std::string& name, std::vector<std::string>& values) const {
  getColumn<std::string>(name, values, consts::eString);
}

void osdf::FrameRows::setColumn(const std::string& name,
                                const std::vector<std::int8_t>& data) const {
  setColumn<std::int8_t>(name, data, consts::eInt8);
}

void osdf::FrameRows::setColumn(const std::string& name,
                                const std::vector<std::int16_t>& data) const {
  setColumn<std::int16_t>(name, data, consts::eInt16);
}

void osdf::FrameRows::setColumn(const std::string& name,
                                const std::vector<std::int32_t>& data) const {
  setColumn<std::int32_t>(name, data, consts::eInt32);
}

void osdf::FrameRows::setColumn(const std::string& name,
                                const std::vector<std::int64_t>& data) const {
  setColumn<std::int64_t>(name, data, consts::eInt64);
}

void osdf::FrameRows::setColumn(const std::string& name,
                                const std::vector<float>& data) const {
  setColumn<float>(name, data, consts::eFloat);
}

void osdf::FrameRows::setColumn(const std::string& name,
                                const std::vector<double>& data) const {
  setColumn<double>(name, data, consts::eDouble);
}

void osdf::FrameRows::setColumn(const std::string& name,
                                const std::vector<std::string>& data) const {
  setColumn<std::string>(name, data, consts::eString);
}

void osdf::FrameRows::removeColumn(const std::string& name) {
  if (data_.columnExists(name) == true) {
    const std::int32_t index = data_.getIndex(name);
    const std::int8_t permission = data_.getPermission(index);
    if (permission == consts::eReadWrite) {
      data_.removeColumn(index);
    } else {
      oops::Log::error() << "ERROR: Column named \"" << name
                         << "\" is set to read-only." << std::endl;
    }
  } else {
    oops::Log::error() << "ERROR: Column named \"" << name
                       << "\" not found in current data frame." << std::endl;
  }
}

void osdf::FrameRows::removeColumn(const std::int32_t index) {
  const std::string& name = data_.getName(index);
  if (name == consts::kErrorReturnString) {
    const std::int8_t permission = data_.getPermission(index);
    if (permission == consts::eReadWrite) {
      data_.removeColumn(index);
    } else {
      oops::Log::error() << "ERROR: Column at index \"" << index
                         << "\" is set to read-only." << std::endl;
    }
  } else {
    oops::Log::error() << "ERROR: Column at index \"" << index
                       << "\" not found in current data frame." << std::endl;
  }
}

void osdf::FrameRows::removeRow(const std::int64_t index) {
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
    }
  } else {
    oops::Log::error() << "ERROR: Row index \"" << index
                       << "\" is incompatible with current data frame." << std::endl;
  }
}

void osdf::FrameRows::sortRows(const std::string& columnName, const std::int8_t order) {
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
      const std::int32_t index = data_.getIndex(columnName);
      if (order == consts::eAscending) {
        funcs_.sortRows(&data_, index, [&](std::shared_ptr<DatumBase>& datumA,
                                               std::shared_ptr<DatumBase>& datumB) {
          return funcs_.compareDatums(datumA, datumB);
        });
      } else if (order == consts::eDescending) {
        funcs_.sortRows(&data_, index, [&](std::shared_ptr<DatumBase>& datumA,
                                               std::shared_ptr<DatumBase>& datumB) {
          return funcs_.compareDatums(datumB, datumA);
        });
      }
    }
  } else {
    oops::Log::error() << "ERROR: Column named \"" << columnName
                       << "\" not found in current data frame." << std::endl;
  }
}

void osdf::FrameRows::print() const {
  data_.print();
}

void osdf::FrameRows::clear() {
  data_.clear();
}

/// Other public functions

osdf::FrameRows osdf::FrameRows::sliceRows(const std::string& name, const std::int8_t comparison,
                                       const std::int8_t threshold) const {
  return sliceRows<std::int8_t>(name, comparison, threshold);
}

osdf::FrameRows osdf::FrameRows::sliceRows(const std::string& name, const std::int8_t comparison,
                                       const std::int16_t threshold) const {
  return sliceRows<std::int16_t>(name, comparison, threshold);
}

osdf::FrameRows osdf::FrameRows::sliceRows(const std::string& name, const std::int8_t comparison,
                                       const std::int32_t threshold) const {
  return sliceRows<std::int32_t>(name, comparison, threshold);
}

osdf::FrameRows osdf::FrameRows::sliceRows(const std::string& name, const std::int8_t comparison,
                                       const std::int64_t threshold) const {
  return sliceRows<std::int64_t>(name, comparison, threshold);
}

osdf::FrameRows osdf::FrameRows::sliceRows(const std::string& name, const std::int8_t comparison,
                                       const float threshold) const {
  return sliceRows<float>(name, comparison, threshold);
}

osdf::FrameRows osdf::FrameRows::sliceRows(const std::string& name, const std::int8_t comparison,
                                       const double threshold) const {
  return sliceRows<double>(name, comparison, threshold);
}

osdf::FrameRows osdf::FrameRows::sliceRows(const std::string& name, const std::int8_t comparison,
                                       const std::string threshold) const {
  return sliceRows<std::string>(name, comparison, threshold);
}

void osdf::FrameRows::sortRows(const std::string& columnName, const std::function<std::int8_t(
     const std::shared_ptr<DatumBase>, const std::shared_ptr<DatumBase>)> func) {
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
      funcs_.sortRows(&data_, columnName, func);
    }
  } else {
    oops::Log::error() << "ERROR: Column named \"" << columnName
                       << "\" not found in current data frame." << std::endl;
  }
}

osdf::FrameRows osdf::FrameRows::sliceRows(
                                 const std::function<const std::int8_t(const DataRow&)> func) {
  std::vector<DataRow> newDataRows;
  ColumnMetadata newColumnMetadata = data_.getColumnMetadata();
  newDataRows.reserve(static_cast<std::size_t>(data_.getSizeRows()));
  newColumnMetadata.resetMaxId();  // Only relevant for column alignment when printing
  const std::vector<DataRow>& dataRows = data_.getDataRows();
  std::copy_if(dataRows.begin(), dataRows.end(), std::back_inserter(newDataRows),
                                                 [&](const DataRow& dataRow) {
    newColumnMetadata.updateMaxId(dataRow.getId());
    return func(dataRow);
  });
  newDataRows.shrink_to_fit();
  return FrameRows(newColumnMetadata, newDataRows);
}

osdf::ViewRows osdf::FrameRows::makeView() const {
  std::vector<std::shared_ptr<DataRow>> newDataRows;
  newDataRows.reserve(static_cast<std::size_t>(data_.getSizeRows()));
  ColumnMetadata newColumnMetadata = data_.getColumnMetadata();
  for (const DataRow& dataRow : data_.getDataRows()) {
    std::shared_ptr<DataRow> dataRowView = std::make_shared<DataRow>(dataRow);
    newDataRows.push_back(dataRowView);
  }
  return ViewRows(newColumnMetadata, newDataRows);
}

const osdf::FrameRowsData& osdf::FrameRows::getData() const {
  return data_;
}

/// Private functions

template <typename T>
void osdf::FrameRows::appendNewColumn(const std::string& name, const std::vector<T>& values,
                                      const std::int8_t type) {
  if (data_.columnExists(name) == false) {
    if (values.size() != 0) {
      const std::int64_t valuesSize = static_cast<std::int64_t>(values.size());
      if (data_.getSizeRows() == 0) {
        data_.initialise(valuesSize);
      }
      if (valuesSize == data_.getSizeRows()) {
        const std::int32_t columnIndex = data_.getSizeCols();
        data_.appendNewColumn(name, type);
        std::int64_t rowIndex = 0;
        for (const T& value : values) {
          DataRow& dataRow = data_.getDataRow(rowIndex);
          const std::shared_ptr<DatumBase> datum = funcs_.createDatum(value);
          dataRow.insert(datum);
          const std::int16_t datumSize = static_cast<std::int16_t>(datum->getValueStr().size());
          data_.updateColumnWidth(columnIndex, datumSize);
          rowIndex++;
        }
      } else {
        oops::Log::error() << "ERROR: Number of rows in new column incompatible "
                              "with current data frame." << std::endl;
      }
    } else {
      oops::Log::error() << "ERROR: No values present in data vector." << std::endl;
    }
  } else {
    oops::Log::error() << "ERROR: A column named \"" << name << "\" already exists." << std::endl;
  }
}

template<typename T>
void osdf::FrameRows::getColumn(const std::string& name, std::vector<T>& values,
                                const std::int8_t type) const {
  if (data_.columnExists(name) == true)  {
    const std::int32_t columnIndex = data_.getIndex(name);
    const std::int8_t columnType = data_.getType(columnIndex);
    if (type == columnType) {
      funcs_.getColumn<T>(&data_, columnIndex, values);
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
void osdf::FrameRows::setColumn(const std::string& name, const std::vector<T>& data,
                                const std::int8_t type) const {
  if (data_.columnExists(name) == true)  {
    const std::int32_t columnIndex = data_.getIndex(name);
    const std::int8_t permission = data_.getPermission(columnIndex);
    if (permission == consts::eReadWrite) {
      std::int8_t columnType = data_.getType(columnIndex);
      if (type == columnType) {
        if (static_cast<std::int64_t>(data.size()) == data_.getSizeRows()) {
          for (std::int64_t rowIndex = 0; rowIndex < data_.getSizeRows(); ++rowIndex) {
            const std::shared_ptr<DatumBase>& datum =
                                              data_.getDataRow(rowIndex).getColumn(columnIndex);
            funcs_.setDatumValue<T>(datum, data.at(static_cast<std::size_t>(rowIndex)));
          }
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
osdf::FrameRows osdf::FrameRows::sliceRows(const std::string& name, const std::int8_t comparison,
                                           const T threshold) const {
  std::vector<DataRow> newDataRows;
  ColumnMetadata newColumnMetadata;
  if (data_.columnExists(name) == true)  {
    newDataRows.reserve(static_cast<std::size_t>(data_.getSizeRows()));
    newColumnMetadata = data_.getColumnMetadata();
    newColumnMetadata.resetMaxId();  // Only relevant for column alignment when printing
    const std::int32_t index = data_.getIndex(name);
    const std::vector<DataRow>& dataRows = data_.getDataRows();
    std::copy_if(dataRows.begin(), dataRows.end(), std::back_inserter(newDataRows),
                                                   [&](const DataRow& dataRow) {
      const std::shared_ptr<DatumBase>& datum = dataRow.getColumn(index);
      const T value = funcs_.getDatumValue<T>(datum);
      const std::int8_t retVal = funcs_.compareToThreshold(comparison, threshold, value);
      if (retVal == true) {
        newColumnMetadata.updateMaxId(dataRow.getId());
      }
      return retVal;
    });
  } else {
    oops::Log::error() << "ERROR: Column named \"" << name
                       << "\" not found in current data frame." << std::endl;
  }
  newDataRows.shrink_to_fit();
  return FrameRows(newColumnMetadata, newDataRows);
}
