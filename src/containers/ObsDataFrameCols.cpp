
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

void osdf::ObsDataFrameCols::appendNewColumn(const std::string& name,
                                             const std::vector<std::int8_t>& values) {
  appendNewColumn(name, values, consts::eInt8);
}

void osdf::ObsDataFrameCols::appendNewColumn(const std::string& name,
                                             const std::vector<std::int16_t>& values) {
  appendNewColumn(name, values, consts::eInt16);
}

void osdf::ObsDataFrameCols::appendNewColumn(const std::string& name,
                                             const std::vector<std::int32_t>& values) {
  appendNewColumn(name, values, consts::eInt32);
}

void osdf::ObsDataFrameCols::appendNewColumn(const std::string& name,
                                             const std::vector<std::int64_t>& values) {
  appendNewColumn(name, values, consts::eInt64);
}

void osdf::ObsDataFrameCols::appendNewColumn(const std::string& name,
                                             const std::vector<float>& values) {
  appendNewColumn(name, values, consts::eFloat);
}

void osdf::ObsDataFrameCols::appendNewColumn(const std::string& name,
                                             const std::vector<double>& values) {
  appendNewColumn(name, values, consts::eDouble);
}

void osdf::ObsDataFrameCols::appendNewColumn(const std::string& name,
                                             const std::vector<std::string>& values) {
  appendNewColumn(name, values, consts::eString);
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
        std::shared_ptr<Datum<std::int8_t>> datumInt8 =
            std::static_pointer_cast<Datum<std::int8_t>>(datum);
        std::shared_ptr<Data<std::int8_t>> dataInt8 =
            std::static_pointer_cast<Data<std::int8_t>>(data);
        dataInt8->addDatum(datumInt8->getDatum());
        break;
      }
      case consts::eInt16: {
        std::shared_ptr<Datum<std::int16_t>> datumInt16 =
            std::static_pointer_cast<Datum<std::int16_t>>(datum);
        std::shared_ptr<Data<std::int16_t>> dataInt16 =
            std::static_pointer_cast<Data<std::int16_t>>(data);
        dataInt16->addDatum(datumInt16->getDatum());
        break;
      }
      case consts::eInt32: {
        std::shared_ptr<Datum<std::int32_t>> datumInt32 =
            std::static_pointer_cast<Datum<std::int32_t>>(datum);
        std::shared_ptr<Data<std::int32_t>> dataInt32 =
            std::static_pointer_cast<Data<std::int32_t>>(data);
        dataInt32->addDatum(datumInt32->getDatum());
        break;
      }
      case consts::eInt64: {
        std::shared_ptr<Datum<std::int64_t>> datumInt64 =
            std::static_pointer_cast<Datum<std::int64_t>>(datum);
        std::shared_ptr<Data<std::int64_t>> dataInt64 =
            std::static_pointer_cast<Data<std::int64_t>>(data);
        dataInt64->addDatum(datumInt64->getDatum());
        break;
      }
      case consts::eFloat: {
        std::shared_ptr<Datum<float>> datumFloat =
            std::static_pointer_cast<Datum<float>>(datum);
        std::shared_ptr<Data<float>> dataFloat =
            std::static_pointer_cast<Data<float>>(data);
        dataFloat->addDatum(datumFloat->getDatum());
        break;
      }
      case consts::eDouble: {
        std::shared_ptr<Datum<double>> datumDouble =
            std::static_pointer_cast<Datum<double>>(datum);
        std::shared_ptr<Data<double>> dataDouble =
            std::static_pointer_cast<Data<double>>(data);
        dataDouble->addDatum(datumDouble->getDatum());
        break;
      }
      case consts::eString: {
        std::shared_ptr<Datum<std::string>> datumString =
            std::static_pointer_cast<Datum<std::string>>(datum);
        std::shared_ptr<Data<std::string>> dataString =
            std::static_pointer_cast<Data<std::string>>(data);
        dataString->addDatum(datumString->getDatum());
        break;
      }
    }
  }
}

void osdf::ObsDataFrameCols::getColumn(const std::string& name,
                                       std::vector<std::int8_t>& data) const {
  getColumn(name, data, consts::eInt8);
}

void osdf::ObsDataFrameCols::getColumn(const std::string& name,
                                       std::vector<std::int16_t>& data) const {
  getColumn(name, data, consts::eInt16);
}

void osdf::ObsDataFrameCols::getColumn(const std::string& name,
                                       std::vector<std::int32_t>& data) const {
  getColumn(name, data, consts::eInt32);
}

void osdf::ObsDataFrameCols::getColumn(const std::string& name,
                                       std::vector<std::int64_t>& data) const {
  getColumn(name, data, consts::eInt64);
}

void osdf::ObsDataFrameCols::getColumn(const std::string& name,
                                       std::vector<float>& data) const {
  getColumn(name, data, consts::eFloat);
}

void osdf::ObsDataFrameCols::getColumn(const std::string& name,
                                       std::vector<double>& data) const {
  getColumn(name, data, consts::eDouble);
}

void osdf::ObsDataFrameCols::getColumn(const std::string& name,
                                       std::vector<std::string>& data) const {
  getColumn(name, data, consts::eString);
}

void osdf::ObsDataFrameCols::setColumn(const std::string& name,
                                       const std::vector<std::int8_t>& data) const {
  setColumn(name, data, consts::eInt8);
}

void osdf::ObsDataFrameCols::setColumn(const std::string& name,
                                       const std::vector<std::int16_t>& data) const {
  setColumn(name, data, consts::eInt16);
}

void osdf::ObsDataFrameCols::setColumn(const std::string& name,
                                       const std::vector<std::int32_t>& data) const {
  setColumn(name, data, consts::eInt32);
}

void osdf::ObsDataFrameCols::setColumn(const std::string& name,
                                       const std::vector<std::int64_t>& data) const {
  setColumn(name, data, consts::eInt64);
}

void osdf::ObsDataFrameCols::setColumn(const std::string& name,
                                       const std::vector<float>& data) const {
  setColumn(name, data, consts::eFloat);
}

void osdf::ObsDataFrameCols::setColumn(const std::string& name,
                                       const std::vector<double>& data) const {
  setColumn(name, data, consts::eDouble);
}

void osdf::ObsDataFrameCols::setColumn(const std::string& name,
                                       const std::vector<std::string>& data) const {
  setColumn(name, data, consts::eString);
}

template<>
void osdf::ObsDataFrameCols::getDataValue<std::int8_t>(std::shared_ptr<DataBase> data,
                                                       std::vector<std::int8_t>& vars) const {
  std::shared_ptr<Data<std::int8_t>> dataInt8 = std::static_pointer_cast<Data<std::int8_t>>(data);
  vars = dataInt8->getData();
}

template<>
void osdf::ObsDataFrameCols::getDataValue<std::int16_t>(std::shared_ptr<DataBase> data,
                                                        std::vector<std::int16_t>& vars) const {
  std::shared_ptr<Data<std::int16_t>> dataInt16 =
      std::static_pointer_cast<Data<std::int16_t>>(data);
  vars = dataInt16->getData();
}

template<>
void osdf::ObsDataFrameCols::getDataValue<std::int32_t>(std::shared_ptr<DataBase> data,
                                                        std::vector<std::int32_t>& vars) const {
  std::shared_ptr<Data<std::int32_t>> dataInt32 =
      std::static_pointer_cast<Data<std::int32_t>>(data);
  vars = dataInt32->getData();
}

template<>
void osdf::ObsDataFrameCols::getDataValue<std::int64_t>(std::shared_ptr<DataBase> data,
                                                        std::vector<std::int64_t>& vars) const {
  std::shared_ptr<Data<std::int64_t>> dataInt64 =
      std::static_pointer_cast<Data<std::int64_t>>(data);
  vars = dataInt64->getData();
}

template<>
void osdf::ObsDataFrameCols::getDataValue<float>(std::shared_ptr<DataBase> data,
                                                 std::vector<float>& vars) const {
  std::shared_ptr<Data<float>> dataFloat = std::static_pointer_cast<Data<float>>(data);
  vars = dataFloat->getData();
}

template<>
void osdf::ObsDataFrameCols::getDataValue<double>(std::shared_ptr<DataBase> data,
                                                  std::vector<double>& vars) const {
  std::shared_ptr<Data<double>> dataDouble = std::static_pointer_cast<Data<double>>(data);
  vars = dataDouble->getData();
}

template<>
void osdf::ObsDataFrameCols::getDataValue<std::string>(std::shared_ptr<DataBase> data,
                                                       std::vector<std::string>& vars) const {
  std::shared_ptr<Data<std::string>> dataString = std::static_pointer_cast<Data<std::string>>(data);
  vars = dataString->getData();
}

template<>
void osdf::ObsDataFrameCols::setDataValue<std::int8_t>(std::shared_ptr<DataBase> data,
                                                 const std::vector<std::int8_t>& vars) const {
  std::shared_ptr<Data<std::int8_t>> dataInt8 = std::static_pointer_cast<Data<std::int8_t>>(data);
  dataInt8->setData(vars);
}

template<>
void osdf::ObsDataFrameCols::setDataValue<std::int16_t>(std::shared_ptr<DataBase> data,
                                                  const std::vector<std::int16_t>& vars) const {
  std::shared_ptr<Data<std::int16_t>> dataInt16 =
      std::static_pointer_cast<Data<std::int16_t>>(data);
  dataInt16->setData(vars);
}

template<>
void osdf::ObsDataFrameCols::setDataValue<std::int32_t>(std::shared_ptr<DataBase> data,
                                                  const std::vector<std::int32_t>& vars) const {
  std::shared_ptr<Data<std::int32_t>> dataInt32 =
      std::static_pointer_cast<Data<std::int32_t>>(data);
  dataInt32->setData(vars);
}

template<>
void osdf::ObsDataFrameCols::setDataValue<std::int64_t>(std::shared_ptr<DataBase> data,
                                                  const std::vector<std::int64_t>& vars) const {
  std::shared_ptr<Data<std::int64_t>> dataInt64 =
      std::static_pointer_cast<Data<std::int64_t>>(data);
  dataInt64->setData(vars);
}

template<>
void osdf::ObsDataFrameCols::setDataValue<float>(std::shared_ptr<DataBase> data,
                                           const std::vector<float>& vars) const {
  std::shared_ptr<Data<float>> dataFloat = std::static_pointer_cast<Data<float>>(data);
  dataFloat->setData(vars);
}

template<>
void osdf::ObsDataFrameCols::setDataValue<double>(std::shared_ptr<DataBase> data,
                                            const std::vector<double>& vars) const {
  std::shared_ptr<Data<double>> dataDouble = std::static_pointer_cast<Data<double>>(data);
  dataDouble->setData(vars);
}

template<>
void osdf::ObsDataFrameCols::setDataValue<std::string>(std::shared_ptr<DataBase> data,
                                                 const std::vector<std::string>& vars) const {
  std::shared_ptr<Data<std::string>> dataString = std::static_pointer_cast<Data<std::string>>(data);
  dataString->setData(vars);
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
            std::shared_ptr<Data<std::int8_t>> dataInt8 =
                std::static_pointer_cast<Data<std::int8_t>>(data);
            dataInt8->removeDatum(index);
            break;
          }
          case consts::eInt16: {
            std::shared_ptr<Data<std::int16_t>> dataInt16 =
                std::static_pointer_cast<Data<std::int16_t>>(data);
            dataInt16->removeDatum(index);
            break;
          }
          case consts::eInt32: {
            std::shared_ptr<Data<std::int32_t>> dataInt32 =
                std::static_pointer_cast<Data<std::int32_t>>(data);
            dataInt32->removeDatum(index);
            break;
          }
          case consts::eInt64: {
            std::shared_ptr<Data<std::int64_t>> dataInt64 =
                std::static_pointer_cast<Data<std::int64_t>>(data);
            dataInt64->removeDatum(index);
            break;
          }
          case consts::eFloat: {
            std::shared_ptr<Data<float>> dataFloat =
                std::static_pointer_cast<Data<float>>(data);
            dataFloat->removeDatum(index);
            break;
          }
          case consts::eDouble: {
            std::shared_ptr<Data<double>> dataDouble =
                std::static_pointer_cast<Data<double>>(data);
            dataDouble->removeDatum(index);
            break;
          }
          case consts::eString: {
            std::shared_ptr<Data<std::string>> dataString =
                std::static_pointer_cast<Data<std::string>>(data);
            dataString->removeDatum(index);
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
          std::shared_ptr<Data<std::int8_t>> dataInt8 =
              std::static_pointer_cast<Data<std::int8_t>>(data);
          populateIndices(indices, dataInt8->getData(), order);
          break;
        }
        case consts::eInt16: {
          std::shared_ptr<Data<std::int16_t>> dataInt16 =
              std::static_pointer_cast<Data<std::int16_t>>(data);
          populateIndices(indices, dataInt16->getData(), order);
          break;
        }
        case consts::eInt32: {
          std::shared_ptr<Data<std::int32_t>> dataInt32 =
              std::static_pointer_cast<Data<std::int32_t>>(data);
          populateIndices(indices, dataInt32->getData(), order);
          break;
        }
        case consts::eInt64: {
          std::shared_ptr<Data<std::int64_t>> dataInt64 =
              std::static_pointer_cast<Data<std::int64_t>>(data);
          populateIndices(indices, dataInt64->getData(), order);
          break;
        }
        case consts::eFloat: {
          std::shared_ptr<Data<float>> dataFloat =
              std::static_pointer_cast<Data<float>>(data);
          populateIndices(indices, dataFloat->getData(), order);
          break;
        }
        case consts::eDouble: {
          std::shared_ptr<Data<double>> dataDouble =
              std::static_pointer_cast<Data<double>>(data);
          populateIndices(indices, dataDouble->getData(), order);
          break;
        }
        case consts::eString: {
          std::shared_ptr<Data<std::string>> dataString =
              std::static_pointer_cast<Data<std::string>>(data);
          populateIndices(indices, dataString->getData(), order);
          break;
        }
      }
      for (std::shared_ptr<DataBase>& data : dataColumns_) {
        switch (data->getType()) {
          case consts::eInt8: {
            std::shared_ptr<Data<std::int8_t>> dataInt8 =
                std::static_pointer_cast<Data<std::int8_t>>(data);
            swapData(indices, dataInt8->getData());
            break;
          }
          case consts::eInt16: {
            std::shared_ptr<Data<std::int16_t>> dataInt16 =
                std::static_pointer_cast<Data<std::int16_t>>(data);
            swapData(indices, dataInt16->getData());
            break;
          }
          case consts::eInt32: {
            std::shared_ptr<Data<std::int32_t>> dataInt32 =
                std::static_pointer_cast<Data<std::int32_t>>(data);
            swapData(indices, dataInt32->getData());
            break;
          }
          case consts::eInt64: {
            std::shared_ptr<Data<std::int64_t>> dataInt64 =
                std::static_pointer_cast<Data<std::int64_t>>(data);
            swapData(indices, dataInt64->getData());
            break;
          }
          case consts::eFloat: {
            std::shared_ptr<Data<float>> dataFloat =
                std::static_pointer_cast<Data<float>>(data);
            swapData(indices, dataFloat->getData());
            break;
          }
          case consts::eDouble: {
            std::shared_ptr<Data<double>> dataDouble =
                std::static_pointer_cast<Data<double>>(data);
            swapData(indices, dataDouble->getData());
            break;
          }
          case consts::eString: {
            std::shared_ptr<Data<std::string>> dataString =
                std::static_pointer_cast<Data<std::string>>(data);
            swapData(indices, dataString->getData());
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

template void osdf::ObsDataFrameCols::appendNewColumn<std::int8_t>(const std::string&,
                                                             const std::vector<std::int8_t>&,
                                                             const std::int8_t);
template void osdf::ObsDataFrameCols::appendNewColumn<std::int16_t>(const std::string&,
                                                             const std::vector<std::int16_t>&,
                                                             const std::int8_t);
template void osdf::ObsDataFrameCols::appendNewColumn<std::int32_t>(const std::string&,
                                                             const std::vector<std::int32_t>&,
                                                             const std::int8_t);
template void osdf::ObsDataFrameCols::appendNewColumn<std::int64_t>(const std::string&,
                                                             const std::vector<std::int64_t>&,
                                                             const std::int8_t);
template void osdf::ObsDataFrameCols::appendNewColumn<float>(const std::string&,
                                                             const std::vector<float>&,
                                                             const std::int8_t);
template void osdf::ObsDataFrameCols::appendNewColumn<double>(const std::string&,
                                                             const std::vector<double>&,
                                                             const std::int8_t);
template void osdf::ObsDataFrameCols::appendNewColumn<std::string>(const std::string&,
                                                             const std::vector<std::string>&,
                                                             const std::int8_t);

template<typename T>
void osdf::ObsDataFrameCols::addColumnToRow(DataRow& row, std::int8_t& isValid, const T param) {
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

template void osdf::ObsDataFrameCols::getColumn<std::int8_t>(
              const std::string&, std::vector<std::int8_t>&, const std::int8_t) const;
template void osdf::ObsDataFrameCols::getColumn<std::int16_t>(
              const std::string&, std::vector<std::int16_t>&, const std::int8_t) const;
template void osdf::ObsDataFrameCols::getColumn<std::int32_t>(
              const std::string&, std::vector<std::int32_t>&, const std::int8_t) const;
template void osdf::ObsDataFrameCols::getColumn<std::int64_t>(
              const std::string&, std::vector<std::int64_t>&, const std::int8_t) const;
template void osdf::ObsDataFrameCols::getColumn<float>(
              const std::string&, std::vector<float>&, const std::int8_t) const;
template void osdf::ObsDataFrameCols::getColumn<double>(
              const std::string&, std::vector<double>&, const std::int8_t) const;
template void osdf::ObsDataFrameCols::getColumn<std::string>(
              const std::string&, std::vector<std::string>&, const std::int8_t) const;

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

template void osdf::ObsDataFrameCols::setColumn<std::int8_t>(
              const std::string&, const std::vector<std::int8_t>&, const std::int8_t) const;
template void osdf::ObsDataFrameCols::setColumn<std::int16_t>(
              const std::string&, const std::vector<std::int16_t>&, const std::int8_t) const;
template void osdf::ObsDataFrameCols::setColumn<std::int32_t>(
              const std::string&, const std::vector<std::int32_t>&, const std::int8_t) const;
template void osdf::ObsDataFrameCols::setColumn<std::int64_t>(
              const std::string&, const std::vector<std::int64_t>&, const std::int8_t) const;
template void osdf::ObsDataFrameCols::setColumn<float>(
              const std::string&, const std::vector<float>&, const std::int8_t) const;
template void osdf::ObsDataFrameCols::setColumn<double>(
              const std::string&, const std::vector<double>&, const std::int8_t) const;
template void osdf::ObsDataFrameCols::setColumn<std::string>(
              const std::string&, const std::vector<std::string>&, const std::int8_t) const;

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
            std::shared_ptr<Data<std::int8_t>> dataInt8 =
                std::static_pointer_cast<Data<std::int8_t>>(data);
            std::vector<std::int8_t>& values = dataInt8->getData();
            std::vector<std::int8_t> newValues(indices.size());
            std::transform(indices.begin(), indices.end(),
                           newValues.begin(), [values](size_t index) {
              return values[index];
            });
            std::shared_ptr<Data<std::int8_t>> newData =
                std::make_shared<Data<std::int8_t>>(newValues);
            newDataColumns.push_back(newData);
            break;
          }
          case consts::eInt16: {
            std::shared_ptr<Data<std::int16_t>> dataInt16 =
                std::static_pointer_cast<Data<std::int16_t>>(data);
            std::vector<std::int16_t>& values = dataInt16->getData();
            std::vector<std::int16_t> newValues(indices.size());
            std::transform(indices.begin(), indices.end(),
                           newValues.begin(), [values](size_t index) {
              return values[index];
            });
            std::shared_ptr<Data<std::int16_t>> newData =
                std::make_shared<Data<std::int16_t>>(newValues);
            newDataColumns.push_back(newData);
            break;
          }
          case consts::eInt32: {
            std::shared_ptr<Data<std::int32_t>> dataInt32 =
                std::static_pointer_cast<Data<std::int32_t>>(data);
            std::vector<std::int32_t>& values = dataInt32->getData();
            std::vector<std::int32_t> newValues(indices.size());
            std::transform(indices.begin(), indices.end(),
                           newValues.begin(), [values](size_t index) {
              return values[index];
            });
            std::shared_ptr<Data<std::int32_t>> newData =
                std::make_shared<Data<std::int32_t>>(newValues);
            newDataColumns.push_back(newData);
            break;
          }
          case consts::eInt64: {
            std::shared_ptr<Data<std::int64_t>> dataInt64 =
                std::static_pointer_cast<Data<std::int64_t>>(data);
            std::vector<std::int64_t>& values = dataInt64->getData();
            std::vector<std::int64_t> newValues(indices.size());
            std::transform(indices.begin(), indices.end(),
                           newValues.begin(), [values](size_t index) {
              return values[index];
            });
            std::shared_ptr<Data<std::int64_t>> newData =
                std::make_shared<Data<std::int64_t>>(newValues);
            newDataColumns.push_back(newData);
            break;
          }
          case consts::eFloat: {
            std::shared_ptr<Data<float>> dataFloat = std::static_pointer_cast<Data<float>>(data);
            std::vector<float>& values = dataFloat->getData();
            std::vector<float> newValues(indices.size());
            std::transform(indices.begin(), indices.end(),
                           newValues.begin(), [values](size_t index) {
              return values[index];
            });
            std::shared_ptr<Data<float>> newData = std::make_shared<Data<float>>(newValues);
            newDataColumns.push_back(newData);
            break;
          }
          case consts::eDouble: {
            std::shared_ptr<Data<double>> dataDouble = std::static_pointer_cast<Data<double>>(data);
            std::vector<double>& values = dataDouble->getData();
            std::vector<double> newValues(indices.size());
            std::transform(indices.begin(), indices.end(),
                           newValues.begin(), [values](size_t index) {
              return values[index];
            });
            std::shared_ptr<Data<double>> newData = std::make_shared<Data<double>>(newValues);
            newDataColumns.push_back(newData);
            break;
          }
          case consts::eString: {
            std::shared_ptr<Data<std::string>> dataString =
                std::static_pointer_cast<Data<std::string>>(data);
            std::vector<std::string>& values = dataString->getData();
            std::vector<std::string> newValues(indices.size());
            std::transform(indices.begin(), indices.end(),
                           newValues.begin(), [values](size_t index) {
              return values[index];
            });
            std::shared_ptr<Data<std::string>> newData =
                std::make_shared<Data<std::string>>(newValues);
            newDataColumns.push_back(newData);
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

template void osdf::ObsDataFrameCols::populateIndices(
              std::vector<std::int64_t>&, const std::vector<std::int8_t>&, const std::int8_t);
template void osdf::ObsDataFrameCols::populateIndices<std::int16_t>(
              std::vector<std::int64_t>&, const std::vector<std::int16_t>&, const std::int8_t);
template void osdf::ObsDataFrameCols::populateIndices<std::int32_t>(
              std::vector<std::int64_t>&, const std::vector<std::int32_t>&, const std::int8_t);
template void osdf::ObsDataFrameCols::populateIndices<std::int64_t>(
              std::vector<std::int64_t>&, const std::vector<std::int64_t>&, const std::int8_t);
template void osdf::ObsDataFrameCols::populateIndices<float>(
              std::vector<std::int64_t>&, const std::vector<float>&, const std::int8_t);
template void osdf::ObsDataFrameCols::populateIndices<double>(
              std::vector<std::int64_t>&, const std::vector<double>&, const std::int8_t);
template void osdf::ObsDataFrameCols::populateIndices<std::string>(
              std::vector<std::int64_t>&, const std::vector<std::string>&, const std::int8_t);

template<typename T>
void osdf::ObsDataFrameCols::swapData(std::vector<std::int64_t>& indices, std::vector<T>& data) {
  for (std::int64_t i = 0; i < (std::int64_t)ids_.size(); ++i) {
    while (indices.at(i) != indices.at(indices.at(i))) {
      std::swap(data.at(indices.at(i)), data.at(indices.at(indices.at(i))));
      std::swap(indices.at(i), indices.at(indices.at(i)));
    }
  }
}

template void osdf::ObsDataFrameCols::swapData(
              std::vector<std::int64_t>&, std::vector<std::int8_t>&);
template void osdf::ObsDataFrameCols::swapData<std::int16_t>(
              std::vector<std::int64_t>&, std::vector<std::int16_t>&);
template void osdf::ObsDataFrameCols::swapData<std::int32_t>(
              std::vector<std::int64_t>&, std::vector<std::int32_t>&);
template void osdf::ObsDataFrameCols::swapData<std::int64_t>(
              std::vector<std::int64_t>&, std::vector<std::int64_t>&);
template void osdf::ObsDataFrameCols::swapData<float>(
              std::vector<std::int64_t>&, std::vector<float>&);
template void osdf::ObsDataFrameCols::swapData<double>(
              std::vector<std::int64_t>&, std::vector<double>&);
template void osdf::ObsDataFrameCols::swapData<std::string>(
              std::vector<std::int64_t>&, std::vector<std::string>&);

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

void osdf::ObsDataFrameCols::initialise(const std::int64_t& numRows) {
  for (std::int64_t index = 0; index < numRows; ++index) {
    ids_.push_back(index);
  }
  columnMetadata_.updateMaxId(ids_.size() - 1);
}
