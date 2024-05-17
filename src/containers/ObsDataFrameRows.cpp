/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ObsDataFrameRows.h"

#include <iostream>
#include <memory>
#include <string>

#include "Constants.h"
#include "DatumBase.h"
#include "DatumDouble.h"
#include "DatumFloat.h"
#include "DatumInt8.h"
#include "DatumInt16.h"
#include "DatumInt32.h"
#include "DatumInt64.h"
#include "DatumString.h"

ObsDataFrameRows::ObsDataFrameRows() : ObsDataFrame(consts::eRowPriority) {}

ObsDataFrameRows::ObsDataFrameRows(ColumnMetadata columnMetadata, std::vector<DataRow> dataRows) :
  ObsDataFrame(columnMetadata, consts::eRowPriority), dataRows_(dataRows) {}

void ObsDataFrameRows::appendNewColumn(const std::string& name,
                                       const std::vector<std::int8_t>& data) {
  appendNewColumn(name, data, consts::eInt8);
}

void ObsDataFrameRows::appendNewColumn(const std::string& name,
                                       const std::vector<std::int16_t>& data) {
  appendNewColumn(name, data, consts::eInt16);
}

void ObsDataFrameRows::appendNewColumn(const std::string& name,
                                       const std::vector<std::int32_t>& data) {
  appendNewColumn(name, data, consts::eInt32);
}

void ObsDataFrameRows::appendNewColumn(const std::string& name,
                                       const std::vector<std::int64_t>& data) {
  appendNewColumn(name, data, consts::eInt64);
}

void ObsDataFrameRows::appendNewColumn(const std::string& name,
                                       const std::vector<float>& data) {
  appendNewColumn(name, data, consts::eFloat);
}

void ObsDataFrameRows::appendNewColumn(const std::string& name,
                                       const std::vector<double>& data) {
  appendNewColumn(name, data, consts::eDouble);
}

void ObsDataFrameRows::appendNewColumn(const std::string& name,
                                       const std::vector<std::string>& data) {
  appendNewColumn(name, data, consts::eString);
}

void ObsDataFrameRows::appendNewRow(DataRow& newRow) {
  std::string rowStr = std::to_string(newRow.getId());
  columnMetadata_.updateColumnWidth(newRow.getId(), rowStr.size());
  dataRows_.push_back(std::move(newRow));
}

void ObsDataFrameRows::getColumn(const std::string& name,
                                 std::vector<std::int8_t>& data) const {
  getColumn(name, data, consts::eInt8);
}

void ObsDataFrameRows::getColumn(const std::string& name,
                                 std::vector<std::int16_t>& data) const {
  getColumn(name, data, consts::eInt16);
}

void ObsDataFrameRows::getColumn(const std::string& name,
                                 std::vector<std::int32_t>& data) const {
  getColumn(name, data, consts::eInt32);
}

void ObsDataFrameRows::getColumn(const std::string& name,
                                 std::vector<std::int64_t>& data) const {
  getColumn(name, data, consts::eInt64);
}

void ObsDataFrameRows::getColumn(const std::string& name,
                                 std::vector<float>& data) const {
  getColumn(name, data, consts::eFloat);
}

void ObsDataFrameRows::getColumn(const std::string& name,
                                 std::vector<double>& data) const {
  getColumn(name, data, consts::eDouble);
}

void ObsDataFrameRows::getColumn(const std::string& name,
                                 std::vector<std::string>& data) const {
  getColumn(name, data, consts::eString);
}

void ObsDataFrameRows::setColumn(const std::string& name,
                                 std::vector<std::int8_t>& data) const {
  setColumn(name, data, consts::eInt8);
}

void ObsDataFrameRows::setColumn(const std::string& name,
                                 std::vector<std::int16_t>& data) const {
  setColumn(name, data, consts::eInt16);
}

void ObsDataFrameRows::setColumn(const std::string& name,
                                 std::vector<std::int32_t>& data) const {
  setColumn(name, data, consts::eInt32);
}

void ObsDataFrameRows::setColumn(const std::string& name,
                                 std::vector<std::int64_t>& data) const {
  setColumn(name, data, consts::eInt64);
}

void ObsDataFrameRows::setColumn(const std::string& name,
                                 std::vector<float>& data) const {
  setColumn(name, data, consts::eFloat);
}

void ObsDataFrameRows::setColumn(const std::string& name,
                                 std::vector<double>& data) const {
  setColumn(name, data, consts::eDouble);
}

void ObsDataFrameRows::setColumn(const std::string& name,
                                 std::vector<std::string>& data) const {
  setColumn(name, data, consts::eString);
}

/// createDataum functions are up here due to the need to define them before they are used.
template<>
std::shared_ptr<DatumBase> ObsDataFrameRows::createDatum<std::int8_t>(
                                      const std::int32_t& columnIndex, std::int8_t value) {
  std::shared_ptr<DatumInt8> datum = std::make_shared<DatumInt8>(value);
  columnMetadata_.updateColumnWidth(columnIndex, std::to_string(value).size());
  return datum;
}

template<>
std::shared_ptr<DatumBase> ObsDataFrameRows::createDatum<std::int16_t>(
                                      const std::int32_t& columnIndex, std::int16_t value) {
  std::shared_ptr<DatumInt16> datum = std::make_shared<DatumInt16>(value);
  columnMetadata_.updateColumnWidth(columnIndex, std::to_string(value).size());
  return datum;
}

template<>
std::shared_ptr<DatumBase> ObsDataFrameRows::createDatum<std::int32_t>(
                                      const std::int32_t& columnIndex, std::int32_t value) {
  std::shared_ptr<DatumInt32> datum = std::make_shared<DatumInt32>(value);
  columnMetadata_.updateColumnWidth(columnIndex, std::to_string(value).size());
  return datum;
}

template<>
std::shared_ptr<DatumBase> ObsDataFrameRows::createDatum<std::int64_t>(
                                      const std::int32_t& columnIndex, std::int64_t value) {
  std::shared_ptr<DatumInt64> datum = std::make_shared<DatumInt64>(value);
  columnMetadata_.updateColumnWidth(columnIndex, std::to_string(value).size());
  return datum;
}

template<>
std::shared_ptr<DatumBase> ObsDataFrameRows::createDatum<float>(
                                      const std::int32_t& columnIndex, float value) {
  std::shared_ptr<DatumFloat> datum = std::make_shared<DatumFloat>(value);
  columnMetadata_.updateColumnWidth(columnIndex, std::to_string(value).size());
  return datum;
}

template<>
std::shared_ptr<DatumBase> ObsDataFrameRows::createDatum<double>(
                                      const std::int32_t& columnIndex, double value) {
  std::shared_ptr<DatumDouble> datum = std::make_shared<DatumDouble>(value);
  columnMetadata_.updateColumnWidth(columnIndex, std::to_string(value).size());
  return datum;
}

template<>
std::shared_ptr<DatumBase> ObsDataFrameRows::createDatum<std::string>(
                                      const std::int32_t& columnIndex, std::string value) {
  std::shared_ptr<DatumString> datum = std::make_shared<DatumString>(value);
  columnMetadata_.updateColumnWidth(columnIndex, value.size());
  return datum;
}

template<>
std::shared_ptr<DatumBase> ObsDataFrameRows::createDatum<char const*>(
                                      const std::int32_t& columnIndex, char const* value) {
  std::string valStr = std::string(value);
  std::shared_ptr<DatumString> datum = std::make_shared<DatumString>(valStr);
  columnMetadata_.updateColumnWidth(columnIndex, valStr.size());
  return datum;
}

template<>
void ObsDataFrameRows::getDatumValue<std::int8_t>(
                               std::shared_ptr<DatumBase> datum, std::int8_t& var) const {
  std::shared_ptr<DatumInt8> datumInt8 = std::static_pointer_cast<DatumInt8>(datum);
  var = datumInt8->getDatum();
}

template<>
void ObsDataFrameRows::getDatumValue<std::int16_t>(
                               std::shared_ptr<DatumBase> datum, std::int16_t& var) const {
  std::shared_ptr<DatumInt16> datumInt16 = std::static_pointer_cast<DatumInt16>(datum);
  var = datumInt16->getDatum();
}

template<>
void ObsDataFrameRows::getDatumValue<std::int32_t>(
                               std::shared_ptr<DatumBase> datum, std::int32_t& var) const {
  std::shared_ptr<DatumInt32> datumInt32 = std::static_pointer_cast<DatumInt32>(datum);
  var = datumInt32->getDatum();
}

template<>
void ObsDataFrameRows::getDatumValue<std::int64_t>(
                               std::shared_ptr<DatumBase> datum, std::int64_t& var) const {
  std::shared_ptr<DatumInt64> datumInt64 = std::static_pointer_cast<DatumInt64>(datum);
  var = datumInt64->getDatum();
}

template<>
void ObsDataFrameRows::getDatumValue<float>(
                               std::shared_ptr<DatumBase> datum, float& var) const {
  std::shared_ptr<DatumFloat> datumFloat = std::static_pointer_cast<DatumFloat>(datum);
  var = datumFloat->getDatum();
}

template<>
void ObsDataFrameRows::getDatumValue<double>(
                               std::shared_ptr<DatumBase> datum, double& var) const {
  std::shared_ptr<DatumDouble> datumDouble = std::static_pointer_cast<DatumDouble>(datum);
  var = datumDouble->getDatum();
}

template<>
void ObsDataFrameRows::getDatumValue<std::string>(
                               std::shared_ptr<DatumBase> datum, std::string& var) const {
  std::shared_ptr<DatumString> datumString = std::static_pointer_cast<DatumString>(datum);
  var = datumString->getDatum();
}


template<>
void ObsDataFrameRows::setDatumValue<std::int8_t>(
                               std::shared_ptr<DatumBase> datum, const std::int8_t& var) const {
  std::shared_ptr<DatumInt8> datumInt8 = std::static_pointer_cast<DatumInt8>(datum);
  datumInt8->setDatum(var);
}

template<>
void ObsDataFrameRows::setDatumValue<std::int16_t>(
                               std::shared_ptr<DatumBase> datum, const std::int16_t& var) const {
  std::shared_ptr<DatumInt16> datumInt16 = std::static_pointer_cast<DatumInt16>(datum);
  datumInt16->setDatum(var);
}

template<>
void ObsDataFrameRows::setDatumValue<std::int32_t>(
                               std::shared_ptr<DatumBase> datum, const std::int32_t& var) const {
  std::shared_ptr<DatumInt32> datumInt32 = std::static_pointer_cast<DatumInt32>(datum);
  datumInt32->setDatum(var);
}

template<>
void ObsDataFrameRows::setDatumValue<std::int64_t>(
                               std::shared_ptr<DatumBase> datum, const std::int64_t& var) const {
  std::shared_ptr<DatumInt64> datumInt64 = std::static_pointer_cast<DatumInt64>(datum);
  datumInt64->setDatum(var);
}

template<>
void ObsDataFrameRows::setDatumValue<float>(
                               std::shared_ptr<DatumBase> datum, const float& var) const {
  std::shared_ptr<DatumFloat> datumFloat = std::static_pointer_cast<DatumFloat>(datum);
  datumFloat->setDatum(var);
}

template<>
void ObsDataFrameRows::setDatumValue<double>(
                               std::shared_ptr<DatumBase> datum, const double& var) const {
  std::shared_ptr<DatumDouble> datumDouble = std::static_pointer_cast<DatumDouble>(datum);
  datumDouble->setDatum(var);
}

template<>
void ObsDataFrameRows::setDatumValue<std::string>(
                               std::shared_ptr<DatumBase> datum, const std::string& var) const {
  std::shared_ptr<DatumString> datumString = std::static_pointer_cast<DatumString>(datum);
  datumString->setDatum(var);
}

void ObsDataFrameRows::removeColumn(const std::string& name) {
  std::int32_t columnIndex = columnMetadata_.getIndex(name);
  if (columnIndex != consts::kErrorValue)  {
    std::int8_t permission = columnMetadata_.getPermission(columnIndex);
    if (permission == consts::eReadWrite) {
      columnMetadata_.remove(columnIndex);
      std::int64_t numberOfRows = dataRows_.size();
      for (std::int32_t rowIndex = 0; rowIndex < numberOfRows; ++rowIndex) {
        dataRows_[rowIndex].remove(columnIndex);
      }
    } else {
      std::cout << "ERROR: The column \"" << name
                << "\" is set to read-only." << std::endl;
    }
  } else {
    std::cout << "ERROR: Column named \"" << name
              << "\" not found in current data frame." << std::endl;
  }
}

void ObsDataFrameRows::removeRow(const std::int64_t& index) {
  std::int64_t numberOfRows = dataRows_.size();
  if (index >= 0 && index < numberOfRows) {
    std::int8_t canRemove = true;
    std::int64_t numberOfColumns = columnMetadata_.getNumCols();
    for (std::int32_t columnIndex = 0; columnIndex < numberOfColumns; ++columnIndex) {
      std::string columnName = columnMetadata_.getName(columnIndex);
      std::int8_t permission = columnMetadata_.getPermission(columnIndex);
      if (permission == consts::eReadOnly) {
        std::cout << "ERROR: The column \"" << columnName
                  << "\" is set to read-only." << std::endl;
        canRemove = false;
        break;
      }
    }
    if (canRemove == true) {
      dataRows_.erase(std::next(dataRows_.begin(), index));
    }
  } else {
    std::cout << "ERROR: Row index is incompatible with current data frame." << std::endl;
  }
}

void ObsDataFrameRows::sort(const std::string& columnName, const std::int8_t order) {
  std::int32_t columnIndex = columnMetadata_.getIndex(columnName);
  if (columnIndex != consts::kErrorValue)  {
    std::int8_t permission = columnMetadata_.getPermission(columnIndex);
    if (permission == consts::eReadWrite) {
      if (order == consts::eAscending) {
        sortRows(columnIndex, [&](std::shared_ptr<DatumBase> datumA,
                                  std::shared_ptr<DatumBase> datumB) {
          return compareDatums(datumA, datumB);
        });
      } else if (order == consts::eDescending) {
        sortRows(columnIndex, [&](std::shared_ptr<DatumBase> datumA,
                                  std::shared_ptr<DatumBase> datumB) {
          return compareDatums(datumB, datumA);
        });
      }
    } else {
      std::cout << "ERROR: The column \"" << columnName
                << "\" is set to read-only." << std::endl;
    }
  } else {
    std::cout << "ERROR: Column named \"" << columnName
              << "\" not found in current data frame." << std::endl;
  }
}


void ObsDataFrameRows::sort(std::function<std::int8_t(DataRow&, DataRow&)> func) {
  std::int32_t numberOfColumns = columnMetadata_.getNumCols();
  std::int8_t columnsAreWriteable = true;
  for (std::int32_t columnIndex = 0; columnIndex < numberOfColumns; ++columnIndex) {
    columnsAreWriteable = columnMetadata_.getPermission(columnIndex);
    if (columnsAreWriteable == false) {
      break;
    }
  }
  if (columnsAreWriteable == true) {
    std::int64_t numberOfRows = dataRows_.size();
    std::vector<std::int64_t> indices(numberOfRows, 0);
    std::iota(std::begin(indices), std::end(indices), 0);  // Build initial list of indices.
    std::sort(std::begin(indices), std::end(indices),
              [&](const std::int64_t& a, const std::int64_t& b) {
      return func(dataRows_[a], dataRows_[b]);
    });
    for (std::int64_t i = 0; i < numberOfRows; ++i) {
      while (indices[i] != indices[indices[i]]) {
        std::swap(dataRows_[indices[i]], dataRows_[indices[indices[i]]]);
        std::swap(indices[i], indices[indices[i]]);
      }
    }
  } else {
    std::cout << "ERROR: One or more columns in the current data table are set to read-only."
              << std::endl;
  }
}

std::shared_ptr<ObsDataFrame> ObsDataFrameRows::slice(const std::string& columnName,
                                                      const std::int8_t comparison,
                                                      std::int8_t threshold) {
  std::vector<DataRow> newDataRows;
  std::int32_t columnIndex = columnMetadata_.getIndex(columnName);
  if (columnIndex != consts::kErrorValue)  {
    if (columnMetadata_.getType(columnIndex) == consts::eInt8) {
      std::copy_if(dataRows_.begin(), dataRows_.end(), std::back_inserter(newDataRows),
                   [&](DataRow& row) {
        std::shared_ptr<DatumBase> datum = row.getColumn(columnIndex);
        std::shared_ptr<DatumInt8> datumInt8 = std::static_pointer_cast<DatumInt8>(datum);
        std::int8_t datumValue = datumInt8->getDatum();
        return compareDatumToThreshold(comparison, threshold, datumValue);
      });
    } else {
      std::cout << "ERROR: Column and threshold data type misconfiguration."
                << std::endl;
    }
  } else {
    std::cout << "ERROR: Column named \"" << columnName
              << "\" not found in current data frame." << std::endl;
  }
  return std::make_shared<ObsDataFrameRows>(columnMetadata_, newDataRows);
}

std::shared_ptr<ObsDataFrame> ObsDataFrameRows::slice(const std::string& columnName,
                                                      const std::int8_t comparison,
                                                      std::int16_t threshold) {
  std::vector<DataRow> newDataRows;
  std::int32_t columnIndex = columnMetadata_.getIndex(columnName);
  if (columnIndex != consts::kErrorValue)  {
    if (columnMetadata_.getType(columnIndex) == consts::eInt16) {
      std::copy_if(dataRows_.begin(), dataRows_.end(),
                   std::back_inserter(newDataRows), [&](DataRow& row) {
        std::shared_ptr<DatumBase> datum = row.getColumn(columnIndex);
        std::shared_ptr<DatumInt16> datumInt16 = std::static_pointer_cast<DatumInt16>(datum);
        std::int16_t datumValue = datumInt16->getDatum();
        return compareDatumToThreshold(comparison, threshold, datumValue);
      });
    } else {
      std::cout << "ERROR: Column and threshold data type misconfiguration." << std::endl;
    }
  } else {
    std::cout << "ERROR: Column named \"" << columnName
              << "\" not found in current data frame." << std::endl;
  }
  return std::make_shared<ObsDataFrameRows>(columnMetadata_, newDataRows);
}

std::shared_ptr<ObsDataFrame> ObsDataFrameRows::slice(const std::string& columnName,
                                                      const std::int8_t comparison,
                                                      std::int32_t threshold) {
  std::vector<DataRow> newDataRows;
  std::int32_t columnIndex = columnMetadata_.getIndex(columnName);
  if (columnIndex != consts::kErrorValue)  {
    if (columnMetadata_.getType(columnIndex) == consts::eInt32) {
      std::copy_if(dataRows_.begin(), dataRows_.end(),
                   std::back_inserter(newDataRows), [&](DataRow& row) {
        std::shared_ptr<DatumBase> datum = row.getColumn(columnIndex);
        std::shared_ptr<DatumInt32> datumInt32 = std::static_pointer_cast<DatumInt32>(datum);
        std::int32_t datumValue = datumInt32->getDatum();
        return compareDatumToThreshold(comparison, threshold, datumValue);
      });
    } else {
      std::cout << "ERROR: Column and threshold data type misconfiguration." << std::endl;
    }
  } else {
    std::cout << "ERROR: Column named \"" << columnName
              << "\" not found in current data frame." << std::endl;
  }
  return std::make_shared<ObsDataFrameRows>(columnMetadata_, newDataRows);
}

std::shared_ptr<ObsDataFrame> ObsDataFrameRows::slice(const std::string& columnName,
                                                      const std::int8_t comparison,
                                                      std::int64_t threshold) {
  std::vector<DataRow> newDataRows;
  std::int32_t columnIndex = columnMetadata_.getIndex(columnName);
  if (columnIndex != consts::kErrorValue)  {
    if (columnMetadata_.getType(columnIndex) == consts::eInt64) {
      std::copy_if(dataRows_.begin(), dataRows_.end(),
                   std::back_inserter(newDataRows), [&](DataRow& row) {
        std::shared_ptr<DatumBase> datum = row.getColumn(columnIndex);
        std::shared_ptr<DatumInt64> datumInt64 = std::static_pointer_cast<DatumInt64>(datum);
        std::int64_t datumValue = datumInt64->getDatum();
        return compareDatumToThreshold(comparison, threshold, datumValue);
      });
    } else {
      std::cout << "ERROR: Column and threshold data type misconfiguration." << std::endl;
    }
  } else {
    std::cout << "ERROR: Column named \"" << columnName
              << "\" not found in current data frame." << std::endl;
  }
  return std::make_shared<ObsDataFrameRows>(columnMetadata_, newDataRows);
}

std::shared_ptr<ObsDataFrame> ObsDataFrameRows::slice(const std::string& columnName,
                                                      const std::int8_t comparison,
                                                      float threshold) {
  std::vector<DataRow> newDataRows;
  std::int32_t columnIndex = columnMetadata_.getIndex(columnName);
  if (columnIndex != consts::kErrorValue)  {
    if (columnMetadata_.getType(columnIndex) == consts::eFloat) {
      std::copy_if(dataRows_.begin(), dataRows_.end(),
                   std::back_inserter(newDataRows), [&](DataRow& row) {
        std::shared_ptr<DatumBase> datum = row.getColumn(columnIndex);
        std::shared_ptr<DatumFloat> datumFloat = std::static_pointer_cast<DatumFloat>(datum);
        float datumValue = datumFloat->getDatum();
        return compareDatumToThreshold(comparison, threshold, datumValue);
      });
    } else {
      std::cout << "ERROR: Column and threshold data type misconfiguration." << std::endl;
    }
  } else {
    std::cout << "ERROR: Column named \"" << columnName
              << "\" not found in current data frame." << std::endl;
  }
  return std::make_shared<ObsDataFrameRows>(columnMetadata_, newDataRows);
}

std::shared_ptr<ObsDataFrame> ObsDataFrameRows::slice(const std::string& columnName,
                                                      const std::int8_t comparison,
                                                      double threshold) {
  std::vector<DataRow> newDataRows;
  std::int32_t columnIndex = columnMetadata_.getIndex(columnName);
  if (columnIndex != consts::kErrorValue)  {
    if (columnMetadata_.getType(columnIndex) == consts::eDouble) {
      std::copy_if(dataRows_.begin(), dataRows_.end(),
                   std::back_inserter(newDataRows), [&](DataRow& row) {
        std::shared_ptr<DatumBase> datum = row.getColumn(columnIndex);
        std::shared_ptr<DatumDouble> datumDouble = std::static_pointer_cast<DatumDouble>(datum);
        double datumValue = datumDouble->getDatum();
        return compareDatumToThreshold(comparison, threshold, datumValue);
      });
    } else {
      std::cout << "ERROR: Column and threshold data type misconfiguration." << std::endl;
    }
  } else {
    std::cout << "ERROR: Column named \"" << columnName
              << "\" not found in current data frame." << std::endl;
  }
  return std::make_shared<ObsDataFrameRows>(columnMetadata_, newDataRows);
}

std::shared_ptr<ObsDataFrame> ObsDataFrameRows::slice(const std::string& columnName,
                                                      const std::int8_t comparison,
                                                      std::string threshold) {
  std::vector<DataRow> newDataRows;
  std::int32_t columnIndex = columnMetadata_.getIndex(columnName);
  if (columnIndex != consts::kErrorValue)  {
    if (columnMetadata_.getType(columnIndex) == consts::eString) {
      std::copy_if(dataRows_.begin(), dataRows_.end(),
                   std::back_inserter(newDataRows), [&](DataRow& row) {
        std::shared_ptr<DatumBase> datum = row.getColumn(columnIndex);
        std::shared_ptr<DatumString> datumString = std::static_pointer_cast<DatumString>(datum);
        std::string datumValue = datumString->getDatum();
        return compareDatumToThreshold(comparison, threshold, datumValue);
      });
    } else {
      std::cout << "ERROR: Column and threshold data type misconfiguration." << std::endl;
    }
  } else {
    std::cout << "ERROR: Column named \"" << columnName
              << "\" not found in current data frame." << std::endl;
  }
  return std::make_shared<ObsDataFrameRows>(columnMetadata_, newDataRows);
}

std::shared_ptr<ObsDataFrame> ObsDataFrameRows::slice(std::function<std::int8_t(DataRow&)> func) {
  std::vector<DataRow> newDataRows;
  std::copy_if(dataRows_.begin(), dataRows_.end(),
               std::back_inserter(newDataRows), [&](DataRow& dataRow) {
    return func(dataRow);
  });
  return std::make_shared<ObsDataFrameRows>(columnMetadata_, newDataRows);
}

const std::int64_t ObsDataFrameRows::getNumRows() const {
  return dataRows_.size();
}

void ObsDataFrameRows::print() {
  if (dataRows_.size() > 0) {
    std::string rowStr = std::to_string(dataRows_.size());
    std::int32_t rowStrSz = rowStr.size();
    columnMetadata_.print(rowStrSz);
    for (auto& dataRow : dataRows_) {
      dataRow.print(columnMetadata_.get(), rowStrSz);
    }
  }
}

std::vector<DataRow>& ObsDataFrameRows::getDataRows() {
  return dataRows_;
}

std::int8_t ObsDataFrameRows::compareDatums(std::shared_ptr<DatumBase> datumA,
                                            std::shared_ptr<DatumBase> datumB) {
  std::int8_t type = datumA->getType();
  switch (type) {
    case consts::eInt8: {
      std::shared_ptr<DatumInt8> datumAInt8 = std::static_pointer_cast<DatumInt8>(datumA);
      std::shared_ptr<DatumInt8> datumBInt8 = std::static_pointer_cast<DatumInt8>(datumB);
      return datumAInt8->getDatum() < datumBInt8->getDatum();
    }
    case consts::eInt16: {
      std::shared_ptr<DatumInt16> datumAInt16 = std::static_pointer_cast<DatumInt16>(datumA);
      std::shared_ptr<DatumInt16> datumBInt16 = std::static_pointer_cast<DatumInt16>(datumB);
      return datumAInt16->getDatum() < datumBInt16->getDatum();
    }
    case consts::eInt32: {
      std::shared_ptr<DatumInt32> datumAInt32 = std::static_pointer_cast<DatumInt32>(datumA);
      std::shared_ptr<DatumInt32> datumBInt32 = std::static_pointer_cast<DatumInt32>(datumB);
      return datumAInt32->getDatum() < datumBInt32->getDatum();
    }
    case consts::eInt64: {
      std::shared_ptr<DatumInt64> datumAInt64 = std::static_pointer_cast<DatumInt64>(datumA);
      std::shared_ptr<DatumInt64> datumBInt64 = std::static_pointer_cast<DatumInt64>(datumB);
      return datumAInt64->getDatum() < datumBInt64->getDatum();
    }
    case consts::eFloat: {
      std::shared_ptr<DatumFloat> datumAFloat = std::static_pointer_cast<DatumFloat>(datumA);
      std::shared_ptr<DatumFloat> datumBFloat = std::static_pointer_cast<DatumFloat>(datumB);
      return datumAFloat->getDatum() < datumBFloat->getDatum();
    }
    case consts::eDouble: {
      std::shared_ptr<DatumDouble> datumADouble = std::static_pointer_cast<DatumDouble>(datumA);
      std::shared_ptr<DatumDouble> datumBDouble = std::static_pointer_cast<DatumDouble>(datumB);
      return datumADouble->getDatum() < datumBDouble->getDatum();
    }
    case consts::eString: {
      std::shared_ptr<DatumString> datumAString = std::static_pointer_cast<DatumString>(datumA);
      std::shared_ptr<DatumString> datumBString = std::static_pointer_cast<DatumString>(datumB);
      return datumAString->getDatum() < datumBString->getDatum();
    }
    default:
      throw std::runtime_error("ERROR: Missing type specification.");
  }
}

template <typename T>
void ObsDataFrameRows::appendNewColumn(const std::string& name,
                                       const std::vector<T>& values, const std::int8_t type) {
  std::int8_t columnExists = columnMetadata_.exists(name);
  if (columnExists == false) {
    if (dataRows_.size() == 0) {
      initialise(values.size());
    }
    if (dataRows_.size() == values.size()) {
      std::int32_t rowIndex = 0;
      std::int32_t columnIndex = columnMetadata_.add(ColumnMetadatum(name, type));
      for (const auto& value : values) {
        DataRow& dataRow = dataRows_[rowIndex];
        std::shared_ptr<DatumBase> datum = createDatum(columnIndex, value);
        dataRow.insert(datum);
        rowIndex++;
      }
    } else {
      std::cout
        << "ERROR: Number of rows in new column incompatible with current ObsDataFrameRows."
        << std::endl;
    }
  } else {
    std::cout << "ERROR: A column named \"" + name + "\" already exists." << std::endl;
  }
}

template void ObsDataFrameRows::appendNewColumn<std::int8_t>(const std::string&,
                                                             const std::vector<std::int8_t>&,
                                                             const std::int8_t);
template void ObsDataFrameRows::appendNewColumn<std::int16_t>(const std::string&,
                                                              const std::vector<std::int16_t>&,
                                                              const std::int8_t);
template void ObsDataFrameRows::appendNewColumn<std::int32_t>(const std::string&,
                                                              const std::vector<std::int32_t>&,
                                                              const std::int8_t);
template void ObsDataFrameRows::appendNewColumn<std::int64_t>(const std::string&,
                                                              const std::vector<std::int64_t>&,
                                                              const std::int8_t);
template void ObsDataFrameRows::appendNewColumn<float>(const std::string&,
                                                       const std::vector<float>&,
                                                       const std::int8_t);
template void ObsDataFrameRows::appendNewColumn<double>(const std::string&,
                                                        const std::vector<double>&,
                                                        const std::int8_t);
template void ObsDataFrameRows::appendNewColumn<std::string>(const std::string&,
                                                             const std::vector<std::string>&,
                                                             const std::int8_t);

template<typename T>
void ObsDataFrameRows::addColumnToRow(DataRow& row, std::int8_t& isValid, const T param) {
  if (isValid == true) {
    std::int32_t columnIndex = row.getSize();
    std::string name = columnMetadata_.getName(columnIndex);
    std::int8_t permission = columnMetadata_.getPermission(columnIndex);
    if (permission == consts::eReadWrite) {
      std::int8_t type = columnMetadata_.getType(columnIndex);
      std::shared_ptr<DatumBase> newDatum = createDatum(columnIndex, param);
      if (newDatum->getType() == type) {
        row.insert(newDatum);
      } else {
        std::cout << "ERROR: Data type for \"" << param
                  << "\" is incompatible with the column \""
                  << name << "\" of current ObsDataFrameRows" << std::endl;
        isValid = false;
      }
    } else {
      std::cout << "ERROR: The column \"" << name << "\" is set to read-only." << std::endl;
      isValid = false;
    }
  }
}

template void ObsDataFrameRows::addColumnToRow<std::int8_t>(DataRow&, std::int8_t&,
                                                            const std::int8_t);
template void ObsDataFrameRows::addColumnToRow<std::int16_t>(DataRow&, std::int8_t&,
                                                             const std::int16_t);
template void ObsDataFrameRows::addColumnToRow<std::int32_t>(DataRow&, std::int8_t&,
                                                             const std::int32_t);
template void ObsDataFrameRows::addColumnToRow<std::int64_t>(DataRow&, std::int8_t&,
                                                             const std::int64_t);
template void ObsDataFrameRows::addColumnToRow<float>(DataRow&, std::int8_t&,
                                                      const float);
template void ObsDataFrameRows::addColumnToRow<double>(DataRow&, std::int8_t&,
                                                       const double);
template void ObsDataFrameRows::addColumnToRow<std::string>(DataRow&, std::int8_t&,
                                                            const std::string);
template void ObsDataFrameRows::addColumnToRow<const char*>(DataRow&, std::int8_t&,
                                                            const char*);

template<typename T>
void ObsDataFrameRows::getColumn(const std::string& name, std::vector<T>& data,
                                 const std::int8_t type) const {
  std::int64_t columnIndex = columnMetadata_.getIndex(name);
  if (columnIndex != consts::kErrorValue)  {
    std::int8_t columnType = columnMetadata_.getType(columnIndex);
    if (type == columnType) {
      std::int64_t numberOfRows = dataRows_.size();
      data.resize(numberOfRows);
      for (std::int32_t rowIndex = 0; rowIndex < numberOfRows; ++rowIndex) {
        std::shared_ptr<DatumBase> datum = dataRows_[rowIndex].getColumn(columnIndex);
        getDatumValue(datum, data[rowIndex]);
      }
    } else {
      std::cout << "ERROR: Input vector for column \"" << name
                << "\" is not the required data type." << std::endl;
    }
  } else {
    std::cout << "ERROR: Column named \"" << name
              << "\" not found in current data frame." << std::endl;
  }
}

template void ObsDataFrameRows::getColumn<std::int8_t>(const std::string&,
                                                       std::vector<std::int8_t>&,
                                                       const std::int8_t) const;
template void ObsDataFrameRows::getColumn<std::int16_t>(const std::string&,
                                                        std::vector<std::int16_t>&,
                                                        const std::int8_t) const;
template void ObsDataFrameRows::getColumn<std::int32_t>(const std::string&,
                                                        std::vector<std::int32_t>&,
                                                        const std::int8_t) const;
template void ObsDataFrameRows::getColumn<std::int64_t>(const std::string&,
                                                        std::vector<std::int64_t>&,
                                                        const std::int8_t) const;
template void ObsDataFrameRows::getColumn<float>(const std::string&,
                                                 std::vector<float>&,
                                                 const std::int8_t) const;
template void ObsDataFrameRows::getColumn<double>(const std::string&,
                                                  std::vector<double>&,
                                                  const std::int8_t) const;
template void ObsDataFrameRows::getColumn<std::string>(const std::string&,
                                                       std::vector<std::string>&,
                                                       const std::int8_t) const;

template<typename T>
void ObsDataFrameRows::setColumn(const std::string& name, const std::vector<T>& data,
                                 const std::int8_t type) const {
  std::int64_t columnIndex = columnMetadata_.getIndex(name);
  if (columnIndex != consts::kErrorValue)  {
    std::int8_t permission = columnMetadata_.getPermission(columnIndex);
    if (permission == consts::eReadWrite) {
      std::int8_t columnType = columnMetadata_.getType(columnIndex);
      if (type == columnType) {
        std::int64_t numberOfRows = dataRows_.size();
        if (data.size() == numberOfRows) {
          for (std::int32_t rowIndex = 0; rowIndex < numberOfRows; ++rowIndex) {
            std::shared_ptr<DatumBase> datum = dataRows_[rowIndex].getColumn(columnIndex);
            setDatumValue(datum, data[rowIndex]);
          }
        } else {
          std::cout << "ERROR: Input vector for column \"" << name
                    << "\" is not the required size." << std::endl;
        }
      } else {
        std::cout << "ERROR: Input vector for column \"" << name
                  << "\" is not the required data type." << std::endl;
      }
    } else {
      std::cout << "ERROR: The column \"" << name
                << "\" is set to read-only." << std::endl;
    }
  } else {
    std::cout << "ERROR: Column named \"" << name
              << "\" not found in current data frame." << std::endl;
  }
}

template void ObsDataFrameRows::setColumn<std::int8_t>(const std::string&,
                                                       const std::vector<std::int8_t>&,
                                                       const std::int8_t) const;
template void ObsDataFrameRows::setColumn<std::int16_t>(const std::string&,
                                                        const std::vector<std::int16_t>&,
                                                        const std::int8_t) const;
template void ObsDataFrameRows::setColumn<std::int32_t>(const std::string&,
                                                        const std::vector<std::int32_t>&,
                                                        const std::int8_t) const;
template void ObsDataFrameRows::setColumn<std::int64_t>(const std::string&,
                                                        const  std::vector<std::int64_t>&,
                                                        const std::int8_t) const;
template void ObsDataFrameRows::setColumn<float>(const std::string&,
                                                 const std::vector<float>&,
                                                 const std::int8_t) const;
template void ObsDataFrameRows::setColumn<double>(const std::string&,
                                                  const std::vector<double>&,
                                                  const std::int8_t) const;
template void ObsDataFrameRows::setColumn<std::string>(const std::string&,
                                                       const std::vector<std::string>&,
                                                       const std::int8_t) const;

template<typename T> std::int8_t ObsDataFrameRows::compareDatumToThreshold(
                                        std::int8_t comparison, T threshold, T datumValue) {
  switch (comparison) {
    case consts::eLessThan: return datumValue < threshold;
    case consts::eLessThanOrEqualTo: return datumValue <= threshold;
    case consts::eEqualTo: return datumValue == threshold;
    case consts::eGreaterThan: return datumValue > threshold;
    case consts::eGreaterThanOrEqualTo: return datumValue >= threshold;
    default: throw std::runtime_error("ERROR: Invalid comparison operator specification.");
  }
}

template std::int8_t ObsDataFrameRows::compareDatumToThreshold<std::int8_t>(std::int8_t,
                                                                            std::int8_t,
                                                                            std::int8_t);
template std::int8_t ObsDataFrameRows::compareDatumToThreshold<std::int16_t>(std::int8_t,
                                                                             std::int16_t,
                                                                             std::int16_t);
template std::int8_t ObsDataFrameRows::compareDatumToThreshold<std::int32_t>(std::int8_t,
                                                                             std::int32_t,
                                                                             std::int32_t);
template std::int8_t ObsDataFrameRows::compareDatumToThreshold<std::int64_t>(std::int8_t,
                                                                             std::int64_t,
                                                                             std::int64_t);
template std::int8_t ObsDataFrameRows::compareDatumToThreshold<float>(std::int8_t,
                                                                      float, float);
template std::int8_t ObsDataFrameRows::compareDatumToThreshold<double>(std::int8_t,
                                                                       double, double);
template std::int8_t ObsDataFrameRows::compareDatumToThreshold<std::string>(std::int8_t,
                                                                            std::string,
                                                                            std::string);

void ObsDataFrameRows::createNewRow() {
  DataRow dataRow(dataRows_.size());
  dataRows_.push_back(std::move(dataRow));
}

void ObsDataFrameRows::initialise(const std::int64_t& numRows) {
  for (auto _ = numRows; _--;) {
    createNewRow();
  }
}

void ObsDataFrameRows::configColumns(std::vector<ColumnMetadatum> cols) {
  columnMetadata_.add(std::move(cols));
}

void ObsDataFrameRows::configColumns(std::initializer_list<ColumnMetadatum> initList) {
  std::vector<ColumnMetadatum> cols;
  std::copy(std::begin(initList), std::end(initList), std::back_inserter(cols));
  columnMetadata_.add(std::move(cols));
}
