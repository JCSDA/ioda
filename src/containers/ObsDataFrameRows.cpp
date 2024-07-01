/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/containers/ObsDataFrameRows.h"

#include <iostream>
#include <memory>
#include <string>

#include "ioda/containers/Constants.h"
#include "ioda/containers/DatumBase.h"

osdf::ObsDataFrameRows::ObsDataFrameRows() : ObsDataFrame(consts::eRowPriority) {}

osdf::ObsDataFrameRows::ObsDataFrameRows(ColumnMetadata columnMetadata,
                                         std::vector<DataRow> dataRows) :
  ObsDataFrame(columnMetadata, consts::eRowPriority), dataRows_(dataRows) {}

void osdf::ObsDataFrameRows::appendNewColumn(const std::string& name,
                                             const std::vector<std::int8_t>& data) {
  appendNewColumn(name, data, consts::eInt8);
}

void osdf::ObsDataFrameRows::appendNewColumn(const std::string& name,
                                             const std::vector<std::int16_t>& data) {
  appendNewColumn(name, data, consts::eInt16);
}

void osdf::ObsDataFrameRows::appendNewColumn(const std::string& name,
                                             const std::vector<std::int32_t>& data) {
  appendNewColumn(name, data, consts::eInt32);
}

void osdf::ObsDataFrameRows::appendNewColumn(const std::string& name,
                                             const std::vector<std::int64_t>& data) {
  appendNewColumn(name, data, consts::eInt64);
}

void osdf::ObsDataFrameRows::appendNewColumn(const std::string& name,
                                             const std::vector<float>& data) {
  appendNewColumn(name, data, consts::eFloat);
}

void osdf::ObsDataFrameRows::appendNewColumn(const std::string& name,
                                             const std::vector<double>& data) {
  appendNewColumn(name, data, consts::eDouble);
}

void osdf::ObsDataFrameRows::appendNewColumn(const std::string& name,
                                             const std::vector<std::string>& data) {
  appendNewColumn(name, data, consts::eString);
}

void osdf::ObsDataFrameRows::appendNewRow(const DataRow& newRow) {
  std::string rowStr = std::to_string(newRow.getId());
  columnMetadata_.updateMaxId(newRow.getId());
  dataRows_.push_back(newRow);
}

void osdf::ObsDataFrameRows::getColumn(const std::string& name,
                                       std::vector<std::int8_t>& data) const {
  getColumn(name, data, consts::eInt8);
}

void osdf::ObsDataFrameRows::getColumn(const std::string& name,
                                       std::vector<std::int16_t>& data) const {
  getColumn(name, data, consts::eInt16);
}

void osdf::ObsDataFrameRows::getColumn(const std::string& name,
                                       std::vector<std::int32_t>& data) const {
  getColumn(name, data, consts::eInt32);
}

void osdf::ObsDataFrameRows::getColumn(const std::string& name,
                                       std::vector<std::int64_t>& data) const {
  getColumn(name, data, consts::eInt64);
}

void osdf::ObsDataFrameRows::getColumn(const std::string& name,
                                       std::vector<float>& data) const {
  getColumn(name, data, consts::eFloat);
}

void osdf::ObsDataFrameRows::getColumn(const std::string& name,
                                       std::vector<double>& data) const {
  getColumn(name, data, consts::eDouble);
}

void osdf::ObsDataFrameRows::getColumn(const std::string& name,
                                       std::vector<std::string>& data) const {
  getColumn(name, data, consts::eString);
}

void osdf::ObsDataFrameRows::setColumn(const std::string& name,
                                       const std::vector<std::int8_t>& data) const {
  setColumn(name, data, consts::eInt8);
}

void osdf::ObsDataFrameRows::setColumn(const std::string& name,
                                       const std::vector<std::int16_t>& data) const {
  setColumn(name, data, consts::eInt16);
}

void osdf::ObsDataFrameRows::setColumn(const std::string& name,
                                       const std::vector<std::int32_t>& data) const {
  setColumn(name, data, consts::eInt32);
}

void osdf::ObsDataFrameRows::setColumn(const std::string& name,
                                       const std::vector<std::int64_t>& data) const {
  setColumn(name, data, consts::eInt64);
}

void osdf::ObsDataFrameRows::setColumn(const std::string& name,
                                       const std::vector<float>& data) const {
  setColumn(name, data, consts::eFloat);
}

void osdf::ObsDataFrameRows::setColumn(const std::string& name,
                                       const std::vector<double>& data) const {
  setColumn(name, data, consts::eDouble);
}

void osdf::ObsDataFrameRows::setColumn(const std::string& name,
                                       const std::vector<std::string>& data) const {
  setColumn(name, data, consts::eString);
}

template<>
void osdf::ObsDataFrameRows::getDatumValue<std::int8_t>(const std::shared_ptr<DatumBase> datum,
                                                        std::int8_t& var) const {
  std::shared_ptr<Datum<std::int8_t>> datumInt8 =
      std::static_pointer_cast<Datum<std::int8_t>>(datum);
  var = datumInt8->getDatum();
}

template<>
void osdf::ObsDataFrameRows::getDatumValue<std::int16_t>(const std::shared_ptr<DatumBase> datum,
                                                         std::int16_t& var) const {
  std::shared_ptr<Datum<std::int16_t>> datumInt16 =
      std::static_pointer_cast<Datum<std::int16_t>>(datum);
  var = datumInt16->getDatum();
}

template<>
void osdf::ObsDataFrameRows::getDatumValue<std::int32_t>(const std::shared_ptr<DatumBase> datum,
                                                         std::int32_t& var) const {
  std::shared_ptr<Datum<std::int32_t>> datumInt32 =
      std::static_pointer_cast<Datum<std::int32_t>>(datum);
  var = datumInt32->getDatum();
}

template<>
void osdf::ObsDataFrameRows::getDatumValue<std::int64_t>(const std::shared_ptr<DatumBase> datum,
                                                         std::int64_t& var) const {
  std::shared_ptr<Datum<std::int64_t>> datumInt64 =
      std::static_pointer_cast<Datum<std::int64_t>>(datum);
  var = datumInt64->getDatum();
}

template<>
void osdf::ObsDataFrameRows::getDatumValue<float>(const std::shared_ptr<DatumBase> datum,
                                                  float& var) const {
  std::shared_ptr<Datum<float>> datumFloat = std::static_pointer_cast<Datum<float>>(datum);
  var = datumFloat->getDatum();
}

template<>
void osdf::ObsDataFrameRows::getDatumValue<double>(const std::shared_ptr<DatumBase> datum,
                                                   double& var) const {
  std::shared_ptr<Datum<double>> datumDouble = std::static_pointer_cast<Datum<double>>(datum);
  var = datumDouble->getDatum();
}

template<>
void osdf::ObsDataFrameRows::getDatumValue<std::string>(const std::shared_ptr<DatumBase> datum,
                                                        std::string& var) const {
  std::shared_ptr<Datum<std::string>> datumString =
      std::static_pointer_cast<Datum<std::string>>(datum);
  var = datumString->getDatum();
}


template<>
void osdf::ObsDataFrameRows::setDatumValue<std::int8_t>(const std::shared_ptr<DatumBase> datum,
                                                        const std::int8_t& var) const {
  std::shared_ptr<Datum<std::int8_t>> datumInt8 =
      std::static_pointer_cast<Datum<std::int8_t>>(datum);
  datumInt8->setDatum(var);
}

template<>
void osdf::ObsDataFrameRows::setDatumValue<std::int16_t>(const std::shared_ptr<DatumBase> datum,
                                                         const std::int16_t& var) const {
  std::shared_ptr<Datum<std::int16_t>> datumInt16 =
      std::static_pointer_cast<Datum<std::int16_t>>(datum);
  datumInt16->setDatum(var);
}

template<>
void osdf::ObsDataFrameRows::setDatumValue<std::int32_t>(const std::shared_ptr<DatumBase> datum,
                                                         const std::int32_t& var) const {
  std::shared_ptr<Datum<std::int32_t>> datumInt32 =
      std::static_pointer_cast<Datum<std::int32_t>>(datum);
  datumInt32->setDatum(var);
}

template<>
void osdf::ObsDataFrameRows::setDatumValue<std::int64_t>(const std::shared_ptr<DatumBase> datum,
                                                         const std::int64_t& var) const {
  std::shared_ptr<Datum<std::int64_t>> datumInt64 =
      std::static_pointer_cast<Datum<std::int64_t>>(datum);
  datumInt64->setDatum(var);
}

template<>
void osdf::ObsDataFrameRows::setDatumValue<float>(const std::shared_ptr<DatumBase> datum,
                                                  const float& var) const {
  std::shared_ptr<Datum<float>> datumFloat =
      std::static_pointer_cast<Datum<float>>(datum);
  datumFloat->setDatum(var);
}

template<>
void osdf::ObsDataFrameRows::setDatumValue<double>(const std::shared_ptr<DatumBase> datum,
                                                   const double& var) const {
  std::shared_ptr<Datum<double>> datumDouble =
      std::static_pointer_cast<Datum<double>>(datum);
  datumDouble->setDatum(var);
}

template<>
void osdf::ObsDataFrameRows::setDatumValue<std::string>(const std::shared_ptr<DatumBase> datum,
                                                        const std::string& var) const {
  std::shared_ptr<Datum<std::string>> datumString =
      std::static_pointer_cast<Datum<std::string>>(datum);
  datumString->setDatum(var);
}

void osdf::ObsDataFrameRows::removeColumn(const std::string& name) {
  std::int32_t columnIndex = columnMetadata_.getIndex(name);
  if (columnIndex != consts::kErrorValue)  {
    std::int8_t permission = columnMetadata_.getPermission(columnIndex);
    if (permission == consts::eReadWrite) {
      columnMetadata_.remove(columnIndex);
      for (DataRow& dataRow : dataRows_) {
        dataRow.remove(columnIndex);
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

void osdf::ObsDataFrameRows::removeRow(const std::int64_t& index) {
  std::int64_t numberOfRows = dataRows_.size();
  if (index >= 0 && index < numberOfRows) {
    std::int8_t canRemove = true;
    for (const ColumnMetadatum& columnMetadatum : columnMetadata_.get()) {
      std::string columnName = columnMetadatum.getName();
      std::int8_t permission = columnMetadatum.getPermission();
      if (permission == consts::eReadOnly) {
        oops::Log::error() << "ERROR: The column \"" << columnName
                           << "\" is set to read-only." << std::endl;
        canRemove = false;
        break;
      }
    }
    if (canRemove == true) {
      dataRows_.erase(std::next(dataRows_.begin(), index));
    }
  } else {
    oops::Log::error() << "ERROR: Row index is incompatible with current data frame." << std::endl;
  }
}

void osdf::ObsDataFrameRows::sort(const std::string& columnName, const std::int8_t order) {
  std::int8_t columnsAreWriteable = true;
  for (const ColumnMetadatum& columnMetadatum : columnMetadata_.get()) {
    columnsAreWriteable = columnMetadatum.getPermission();
    if (columnsAreWriteable == false) {
      break;
    }
  }
  if (columnsAreWriteable == true) {
    std::int32_t columnIndex = columnMetadata_.getIndex(columnName);
    if (columnIndex != consts::kErrorValue) {
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
      oops::Log::error() << "ERROR: Column named \"" << columnName
                         << "\" not found in current data frame." << std::endl;
    }
  } else {
    oops::Log::error() << "ERROR: One or more columns in the current "
                          "data table are set to read-only." << std::endl;
  }
}


void osdf::ObsDataFrameRows::sort(const std::string& columnName,
                                  const std::function<std::int8_t(
                                  const std::shared_ptr<DatumBase>,
                                  const std::shared_ptr<DatumBase>)> func) {
  std::int8_t columnsAreWriteable = true;
  for (const ColumnMetadatum& columnMetadatum : columnMetadata_.get()) {
    columnsAreWriteable = columnMetadatum.getPermission();
    if (columnsAreWriteable == false) {
      break;
    }
  }
  if (columnsAreWriteable == true) {
    std::int32_t columnIndex = columnMetadata_.getIndex(columnName);
    if (columnIndex != consts::kErrorValue) {
      std::int64_t numberOfRows = dataRows_.size();
      std::vector<std::int64_t> indices(numberOfRows, 0);
      std::iota(std::begin(indices), std::end(indices), 0);  // Build initial list of indices.
      std::sort(std::begin(indices), std::end(indices), [&](const std::int64_t& i,
                                                            const std::int64_t& j) {
        return func(dataRows_.at(i).getColumn(columnIndex), dataRows_.at(j).getColumn(columnIndex));
      });
      for (std::int64_t i = 0; i < numberOfRows; ++i) {
        while (indices.at(i) != indices.at(indices.at(i))) {
          std::swap(dataRows_.at(indices.at(i)), dataRows_.at(indices.at(indices.at(i))));
          std::swap(indices.at(i), indices.at(indices.at(i)));
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

std::shared_ptr<osdf::ObsDataFrame> osdf::ObsDataFrameRows::slice(const std::string& columnName,
                                                                  const std::int8_t& comparison,
                                                                  const std::int8_t& threshold) {
  return slice(columnName, comparison, threshold, consts::eInt8);
}

std::shared_ptr<osdf::ObsDataFrame> osdf::ObsDataFrameRows::slice(const std::string& columnName,
                                                                  const std::int8_t& comparison,
                                                                  const std::int16_t& threshold) {
  return slice(columnName, comparison, threshold, consts::eInt16);
}

std::shared_ptr<osdf::ObsDataFrame> osdf::ObsDataFrameRows::slice(const std::string& columnName,
                                                                  const std::int8_t& comparison,
                                                                  const std::int32_t& threshold) {
  return slice(columnName, comparison, threshold, consts::eInt32);
}

std::shared_ptr<osdf::ObsDataFrame> osdf::ObsDataFrameRows::slice(const std::string& columnName,
                                                                  const std::int8_t& comparison,
                                                                  const std::int64_t& threshold) {
  return slice(columnName, comparison, threshold, consts::eInt64);
}

std::shared_ptr<osdf::ObsDataFrame> osdf::ObsDataFrameRows::slice(const std::string& columnName,
                                                                  const std::int8_t& comparison,
                                                                  const float& threshold) {
  return slice(columnName, comparison, threshold, consts::eFloat);
}

std::shared_ptr<osdf::ObsDataFrame> osdf::ObsDataFrameRows::slice(const std::string& columnName,
                                                                  const std::int8_t& comparison,
                                                                  const double& threshold) {
  return slice(columnName, comparison, threshold, consts::eDouble);
}

std::shared_ptr<osdf::ObsDataFrame> osdf::ObsDataFrameRows::slice(const std::string& columnName,
                                                                  const std::int8_t& comparison,
                                                                  const std::string& threshold) {
  return slice(columnName, comparison, threshold, consts::eString);
}

std::shared_ptr<osdf::ObsDataFrame> osdf::ObsDataFrameRows::slice(const std::function<
                                                      const std::int8_t(const DataRow&)> func) {
  std::vector<DataRow> newDataRows;
  newDataRows.reserve(dataRows_.size());
  ColumnMetadata newColumnMetadata = columnMetadata_;
  newColumnMetadata.resetMaxId();  // Only relevant for column alignment when printing
  std::copy_if(dataRows_.begin(), dataRows_.end(),
               std::back_inserter(newDataRows), [&](DataRow& dataRow) {
    newColumnMetadata.updateMaxId(dataRow.getId());
    return func(dataRow);
  });
  newDataRows.shrink_to_fit();
  return std::make_shared<ObsDataFrameRows>(newColumnMetadata, newDataRows);
}

void osdf::ObsDataFrameRows::print() {
  if (dataRows_.size() > 0) {
    std::string maxRowIdString = std::to_string(columnMetadata_.getMaxId());
    std::int32_t maxRowIdStringSize = maxRowIdString.size();
    columnMetadata_.print(maxRowIdStringSize);
    for (const DataRow& dataRow : dataRows_) {
      dataRow.print(columnMetadata_, maxRowIdStringSize);
    }
  }
}

std::vector<osdf::DataRow>& osdf::ObsDataFrameRows::getDataRows() {
  return dataRows_;
}

const std::int8_t osdf::ObsDataFrameRows::compareDatums(const std::shared_ptr<DatumBase> datumA,
                                                  const std::shared_ptr<DatumBase> datumB) const {
  std::int8_t type = datumA->getType();
  switch (type) {
    case consts::eInt8: {
      std::shared_ptr<Datum<std::int8_t>> datumAInt8 =
          std::static_pointer_cast<Datum<std::int8_t>>(datumA);
      std::shared_ptr<Datum<std::int8_t>> datumBInt8 =
          std::static_pointer_cast<Datum<std::int8_t>>(datumB);
      return datumAInt8->getDatum() < datumBInt8->getDatum();
    }
    case consts::eInt16: {
      std::shared_ptr<Datum<std::int16_t>> datumAInt16 =
          std::static_pointer_cast<Datum<std::int16_t>>(datumA);
      std::shared_ptr<Datum<std::int16_t>> datumBInt16 =
          std::static_pointer_cast<Datum<std::int16_t>>(datumB);
      return datumAInt16->getDatum() < datumBInt16->getDatum();
    }
    case consts::eInt32: {
      std::shared_ptr<Datum<std::int32_t>> datumAInt32 =
          std::static_pointer_cast<Datum<std::int32_t>>(datumA);
      std::shared_ptr<Datum<std::int32_t>> datumBInt32 =
          std::static_pointer_cast<Datum<std::int32_t>>(datumB);
      return datumAInt32->getDatum() < datumBInt32->getDatum();
    }
    case consts::eInt64: {
      std::shared_ptr<Datum<std::int64_t>> datumAInt64 =
          std::static_pointer_cast<Datum<std::int64_t>>(datumA);
      std::shared_ptr<Datum<std::int64_t>> datumBInt64 =
          std::static_pointer_cast<Datum<std::int64_t>>(datumB);
      return datumAInt64->getDatum() < datumBInt64->getDatum();
    }
    case consts::eFloat: {
      std::shared_ptr<Datum<float>> datumAFloat =
          std::static_pointer_cast<Datum<float>>(datumA);
      std::shared_ptr<Datum<float>> datumBFloat =
          std::static_pointer_cast<Datum<float>>(datumB);
      return datumAFloat->getDatum() < datumBFloat->getDatum();
    }
    case consts::eDouble: {
      std::shared_ptr<Datum<double>> datumADouble =
          std::static_pointer_cast<Datum<double>>(datumA);
      std::shared_ptr<Datum<double>> datumBDouble =
          std::static_pointer_cast<Datum<double>>(datumB);
      return datumADouble->getDatum() < datumBDouble->getDatum();
    }
    case consts::eString: {
      std::shared_ptr<Datum<std::string>> datumAString =
          std::static_pointer_cast<Datum<std::string>>(datumA);
      std::shared_ptr<Datum<std::string>> datumBString =
          std::static_pointer_cast<Datum<std::string>>(datumB);
      return datumAString->getDatum() < datumBString->getDatum();
    }
    default:
      throw std::runtime_error("ERROR: Missing type specification.");
  }
}

template <typename T>
void osdf::ObsDataFrameRows::appendNewColumn(const std::string& name,
                                             const std::vector<T>& values,
                                             const std::int8_t type) {
  std::int8_t columnExists = columnMetadata_.exists(name);
  if (columnExists == false) {
    if (values.size() != 0) {
      if (dataRows_.size() == 0) {
        initialise(values.size());
      }
      if (dataRows_.size() == values.size()) {
        std::int32_t rowIndex = 0;
        std::int32_t columnIndex = columnMetadata_.add(ColumnMetadatum(name, type));
        for (const T& value : values) {
          DataRow& dataRow = dataRows_.at(rowIndex);
          std::shared_ptr<DatumBase> datum = osdf::funcs::createDatum(columnIndex, value);
          dataRow.insert(datum);
          rowIndex++;
        }
      } else {
        oops::Log::error() << "ERROR: Number of rows in new column incompatible "
                              "with current ObsDataFrameRows." << std::endl;
      }
    } else {
      oops::Log::error() << "ERROR: No values present in data vector." << std::endl;
    }
  } else {
    oops::Log::error() << "ERROR: A column named \"" + name + "\" already exists." << std::endl;
  }
}

template void osdf::ObsDataFrameRows::appendNewColumn<std::int8_t>(
              const std::string&, const std::vector<std::int8_t>&, const std::int8_t);
template void osdf::ObsDataFrameRows::appendNewColumn<std::int16_t>(
              const std::string&, const std::vector<std::int16_t>&, const std::int8_t);
template void osdf::ObsDataFrameRows::appendNewColumn<std::int32_t>(
              const std::string&, const std::vector<std::int32_t>&, const std::int8_t);
template void osdf::ObsDataFrameRows::appendNewColumn<std::int64_t>(
              const std::string&, const std::vector<std::int64_t>&, const std::int8_t);
template void osdf::ObsDataFrameRows::appendNewColumn<float>(
              const std::string&, const std::vector<float>&, const std::int8_t);
template void osdf::ObsDataFrameRows::appendNewColumn<double>(
              const std::string&, const std::vector<double>&, const std::int8_t);
template void osdf::ObsDataFrameRows::appendNewColumn<std::string>(
              const std::string&, const std::vector<std::string>&, const std::int8_t);

template<typename T>
void osdf::ObsDataFrameRows::addColumnToRow(DataRow& row, std::int8_t& isValid, const T param) {
  if (isValid == true) {
    std::int32_t columnIndex = row.getSize();
    std::string name = columnMetadata_.getName(columnIndex);
    std::int8_t permission = columnMetadata_.getPermission(columnIndex);
    if (permission == consts::eReadWrite) {
      std::int8_t type = columnMetadata_.getType(columnIndex);
      std::shared_ptr<DatumBase> newDatum = osdf::funcs::createDatum(columnIndex, param);
      columnMetadata_.updateColumnWidth(columnIndex, newDatum->getDatumStr().size());
      if (newDatum->getType() == type) {
        row.insert(newDatum);
      } else {
        oops::Log::error() << "ERROR: Data type for \"" << param
                           << "\" is incompatible with the column \"" << name
                           << "\" of current ObsDataFrameRows" << std::endl;
        isValid = false;
      }
    } else {
      oops::Log::error() << "ERROR: The column \"" << name
                         << "\" is set to read-only." << std::endl;
      isValid = false;
    }
  }
}

template void osdf::ObsDataFrameRows::addColumnToRow<std::int8_t>(
              DataRow&, std::int8_t&, const std::int8_t);
template void osdf::ObsDataFrameRows::addColumnToRow<std::int16_t>(
              DataRow&, std::int8_t&, const std::int16_t);
template void osdf::ObsDataFrameRows::addColumnToRow<std::int32_t>(
              DataRow&, std::int8_t&, const std::int32_t);
template void osdf::ObsDataFrameRows::addColumnToRow<std::int64_t>(
              DataRow&, std::int8_t&, const std::int64_t);
template void osdf::ObsDataFrameRows::addColumnToRow<float>(
              DataRow&, std::int8_t&, const float);
template void osdf::ObsDataFrameRows::addColumnToRow<double>(
              DataRow&, std::int8_t&, const double);
template void osdf::ObsDataFrameRows::addColumnToRow<std::string>(
              DataRow&, std::int8_t&, const std::string);
template void osdf::ObsDataFrameRows::addColumnToRow<const char*>(
              DataRow&, std::int8_t&, const char*);

template<typename T>
void osdf::ObsDataFrameRows::getColumn(const std::string& name, std::vector<T>& data,
                                 const std::int8_t type) const {
  std::int64_t columnIndex = columnMetadata_.getIndex(name);
  if (columnIndex != consts::kErrorValue)  {
    std::int8_t columnType = columnMetadata_.getType(columnIndex);
    if (type == columnType) {
      std::int64_t numberOfRows = dataRows_.size();
      data.resize(numberOfRows);
      for (std::int32_t rowIndex = 0; rowIndex < numberOfRows; ++rowIndex) {
        std::shared_ptr<DatumBase> datum = dataRows_.at(rowIndex).getColumn(columnIndex);
        getDatumValue(datum, data.at(rowIndex));
      }
    } else {
      oops::Log::error() << "ERROR: Input vector for column \"" << name
                         << "\" is not the required data type." << std::endl;
    }
  } else {
    oops::Log::error() << "ERROR: Column named \"" << name
                       << "\" not found in current data frame." << std::endl;
  }
}

template void osdf::ObsDataFrameRows::getColumn<std::int8_t>(
              const std::string&, std::vector<std::int8_t>&, const std::int8_t) const;
template void osdf::ObsDataFrameRows::getColumn<std::int16_t>(
              const std::string&, std::vector<std::int16_t>&, const std::int8_t) const;
template void osdf::ObsDataFrameRows::getColumn<std::int32_t>(
              const std::string&,  std::vector<std::int32_t>&, const std::int8_t) const;
template void osdf::ObsDataFrameRows::getColumn<std::int64_t>(
              const std::string&, std::vector<std::int64_t>&, const std::int8_t) const;
template void osdf::ObsDataFrameRows::getColumn<float>(
              const std::string&, std::vector<float>&, const std::int8_t) const;
template void osdf::ObsDataFrameRows::getColumn<double>(
              const std::string&, std::vector<double>&, const std::int8_t) const;
template void osdf::ObsDataFrameRows::getColumn<std::string>(
              const std::string&, std::vector<std::string>&, const std::int8_t) const;

template<typename T>
void osdf::ObsDataFrameRows::setColumn(const std::string& name, const std::vector<T>& data,
                                 const std::int8_t type) const {
  std::int64_t columnIndex = columnMetadata_.getIndex(name);
  if (columnIndex != consts::kErrorValue)  {
    std::int8_t permission = columnMetadata_.getPermission(columnIndex);
    if (permission == consts::eReadWrite) {
      std::int8_t columnType = columnMetadata_.getType(columnIndex);
      if (type == columnType) {
        std::int64_t numberOfRows = dataRows_.size();
        if ((std::int64_t)data.size() == numberOfRows) {
          for (std::int32_t rowIndex = 0; rowIndex < numberOfRows; ++rowIndex) {
            std::shared_ptr<DatumBase> datum = dataRows_.at(rowIndex).getColumn(columnIndex);
            setDatumValue(datum, data.at(rowIndex));
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

template void osdf::ObsDataFrameRows::setColumn<std::int8_t>(
              const std::string&, const std::vector<std::int8_t>&, const std::int8_t) const;
template void osdf::ObsDataFrameRows::setColumn<std::int16_t>(
              const std::string&, const std::vector<std::int16_t>&, const std::int8_t) const;
template void osdf::ObsDataFrameRows::setColumn<std::int32_t>(
              const std::string&, const std::vector<std::int32_t>&, const std::int8_t) const;
template void osdf::ObsDataFrameRows::setColumn<std::int64_t>(
              const std::string&, const std::vector<std::int64_t>&, const std::int8_t) const;
template void osdf::ObsDataFrameRows::setColumn<float>(
              const std::string&, const std::vector<float>&, const std::int8_t) const;
template void osdf::ObsDataFrameRows::setColumn<double>(
              const std::string&, const std::vector<double>&, const std::int8_t) const;
template void osdf::ObsDataFrameRows::setColumn<std::string>(
              const std::string&, const std::vector<std::string>&, const std::int8_t) const;

template<typename T>
std::shared_ptr<osdf::ObsDataFrame> osdf::ObsDataFrameRows::slice(const std::string& columnName,
                                                                  const std::int8_t& comparison,
                                                                  const T& threshold,
                                                                  const std::int8_t& type) {
  std::vector<DataRow> newDataRows;
  newDataRows.reserve(dataRows_.size());
  std::int32_t columnIndex = columnMetadata_.getIndex(columnName);
  ColumnMetadata newColumnMetadata = columnMetadata_;
  newColumnMetadata.resetMaxId();  // Only relevant for column alignment when printing
  if (columnIndex != consts::kErrorValue)  {
    if (type == columnMetadata_.getType(columnIndex)) {
      std::copy_if(dataRows_.begin(), dataRows_.end(),
                   std::back_inserter(newDataRows), [&](DataRow& dataRow) {
        T datumValue;
        getDatumValue(dataRow.getColumn(columnIndex), datumValue);
        std::int8_t retVal = compareDatumToThreshold(comparison, threshold, datumValue);
        if (retVal == true) {
          newColumnMetadata.updateMaxId(dataRow.getId());
        }
        return retVal;
      });
    } else {
      oops::Log::error() << "ERROR: Column and threshold data type misconfiguration." << std::endl;
    }
  } else {
    oops::Log::error() << "ERROR: Column named \"" << columnName
                       << "\" not found in current data frame." << std::endl;
  }
  newDataRows.shrink_to_fit();
  return std::make_shared<ObsDataFrameRows>(newColumnMetadata, newDataRows);
}

template std::shared_ptr<osdf::ObsDataFrame> osdf::ObsDataFrameRows::slice<std::int8_t>(
         const std::string&, const std::int8_t&, const std::int8_t&, const std::int8_t&);
template std::shared_ptr<osdf::ObsDataFrame> osdf::ObsDataFrameRows::slice<std::int16_t>(
         const std::string&, const std::int8_t&, const std::int16_t&, const std::int8_t&);
template std::shared_ptr<osdf::ObsDataFrame> osdf::ObsDataFrameRows::slice<std::int32_t>(
         const std::string&, const std::int8_t&, const std::int32_t&, const std::int8_t&);
template std::shared_ptr<osdf::ObsDataFrame> osdf::ObsDataFrameRows::slice<std::int64_t>(
         const std::string&, const std::int8_t&, const std::int64_t&, const std::int8_t&);
template std::shared_ptr<osdf::ObsDataFrame> osdf::ObsDataFrameRows::slice<float>(
         const std::string&, const std::int8_t&, const float&, const std::int8_t&);
template std::shared_ptr<osdf::ObsDataFrame> osdf::ObsDataFrameRows::slice<double>(
         const std::string&, const std::int8_t&, const double&, const std::int8_t&);
template std::shared_ptr<osdf::ObsDataFrame> osdf::ObsDataFrameRows::slice<std::string>(
         const std::string&, const std::int8_t&, const std::string&, const std::int8_t&);

template<typename T>
const std::int8_t osdf::ObsDataFrameRows::compareDatumToThreshold(const std::int8_t comparison,
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

template const std::int8_t osdf::ObsDataFrameRows::compareDatumToThreshold<std::int8_t>(
                           const std::int8_t, const std::int8_t, const std::int8_t) const;
template const std::int8_t osdf::ObsDataFrameRows::compareDatumToThreshold<std::int16_t>(
                           const std::int8_t, const std::int16_t, const std::int16_t) const;
template const std::int8_t osdf::ObsDataFrameRows::compareDatumToThreshold<std::int32_t>(
                           const std::int8_t, const std::int32_t, const std::int32_t) const;
template const std::int8_t osdf::ObsDataFrameRows::compareDatumToThreshold<std::int64_t>(
                           const std::int8_t, const std::int64_t, const std::int64_t) const;
template const std::int8_t osdf::ObsDataFrameRows::compareDatumToThreshold<float>(
                           const std::int8_t, const float, const float) const;
template const std::int8_t osdf::ObsDataFrameRows::compareDatumToThreshold<double>(
                           const std::int8_t, const double, const double) const;
template const std::int8_t osdf::ObsDataFrameRows::compareDatumToThreshold<std::string>(
                           const std::int8_t, const std::string, const std::string) const;

void osdf::ObsDataFrameRows::initialise(const std::int64_t& numRows) {
  for (auto _ = numRows; _--;) {
    DataRow dataRow(dataRows_.size());
    dataRows_.push_back(std::move(dataRow));
  }
  columnMetadata_.updateMaxId(dataRows_.size() - 1);
}
