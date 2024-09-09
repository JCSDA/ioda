/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/containers/ViewCols.h"

#include "oops/util/Logger.h"

#include "ioda/containers/Constants.h"


osdf::ViewCols::ViewCols(const ColumnMetadata& columnMetadata,
                         const std::vector<std::int64_t>& ids,
                         const std::vector<std::shared_ptr<DataBase>>& dataCols) :
    data_(funcs_, columnMetadata, ids, dataCols) {}


void osdf::ViewCols::getColumn(const std::string& name, std::vector<std::int8_t>& values) const {
  getColumn<std::int8_t>(name, values, consts::eInt8);
}

void osdf::ViewCols::getColumn(const std::string& name, std::vector<std::int16_t>& values) const {
  getColumn<std::int16_t>(name, values, consts::eInt16);
}

void osdf::ViewCols::getColumn(const std::string& name, std::vector<std::int32_t>& values) const {
  getColumn<std::int32_t>(name, values, consts::eInt32);
}

void osdf::ViewCols::getColumn(const std::string& name, std::vector<std::int64_t>& values) const {
  getColumn<std::int64_t>(name, values, consts::eInt64);
}

void osdf::ViewCols::getColumn(const std::string& name, std::vector<float>& values) const {
  getColumn<float>(name, values, consts::eFloat);
}

void osdf::ViewCols::getColumn(const std::string& name, std::vector<double>& values) const {
  getColumn<double>(name, values, consts::eDouble);
}

void osdf::ViewCols::getColumn(const std::string& name, std::vector<std::string>& values) const {
  getColumn<std::string>(name, values, consts::eString);
}

osdf::ViewCols osdf::ViewCols::sliceRows(const std::string& name, const std::int8_t comparison,
                                         const std::int8_t threshold) const {
  return sliceRows<std::int8_t>(name, comparison, threshold);
}

osdf::ViewCols osdf::ViewCols::sliceRows(const std::string& name, const std::int8_t comparison,
                                         const std::int16_t threshold) const {
  return sliceRows<std::int16_t>(name, comparison, threshold);
}

osdf::ViewCols osdf::ViewCols::sliceRows(const std::string& name, const std::int8_t comparison,
                                         const std::int32_t threshold) const {
  return sliceRows<std::int32_t>(name, comparison, threshold);
}

osdf::ViewCols osdf::ViewCols::sliceRows(const std::string& name, const std::int8_t comparison,
                                         const std::int64_t threshold) const {
  return sliceRows<std::int64_t>(name, comparison, threshold);
}

osdf::ViewCols osdf::ViewCols::sliceRows(const std::string& name, const std::int8_t comparison,
                                         const float threshold) const {
  return sliceRows<float>(name, comparison, threshold);
}

osdf::ViewCols osdf::ViewCols::sliceRows(const std::string& name, const std::int8_t comparison,
                                         const double threshold) const {
  return sliceRows<double>(name, comparison, threshold);
}

osdf::ViewCols osdf::ViewCols::sliceRows(const std::string& name, const std::int8_t comparison,
                                         const std::string threshold) const {
  return sliceRows<std::string>(name, comparison, threshold);
}

void osdf::ViewCols::print() const {
  data_.print();
}

template<typename T>
void osdf::ViewCols::getColumn(const std::string& name, std::vector<T>& values,
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
osdf::ViewCols osdf::ViewCols::sliceRows(const std::string& name, const std::int8_t comparison,
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
  return ViewCols(newColumnMetadata, newIds, newDataColumns);
}
