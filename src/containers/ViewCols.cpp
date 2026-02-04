/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/containers/ViewCols.h"

#include "eckit/exception/Exceptions.h"

#include "ioda/containers/Constants.h"
#include "ioda/containers/FrameCols.h"

osdf::ViewCols::ViewCols(const ColumnMetadata& columnMetadata, const std::vector<std::int64_t>& ids,
        const std::vector<std::shared_ptr<DataBase>>& dataCols, FrameCols* parent) :
    data_(funcs_, columnMetadata, ids, dataCols), parent_(parent) {
  parent_->attach(this);
}

osdf::ViewCols::~ViewCols() {
  parent_->detach(this);
  clear();
}

void osdf::ViewCols::getColumn(const std::string& name, std::vector<int>& values) const {
  getColumn<int>(name, values, consts::eInt);
}

void osdf::ViewCols::getColumn(const std::string& name, std::vector<std::int64_t>& values) const {
  getColumn<std::int64_t>(name, values, consts::eInt64);
}

void osdf::ViewCols::getColumn(const std::string& name, std::vector<float>& values) const {
  getColumn<float>(name, values, consts::eFloat);
}

void osdf::ViewCols::getColumn(const std::string& name, std::vector<std::string>& values) const {
  getColumn<std::string>(name, values, consts::eString);
}

osdf::ViewCols osdf::ViewCols::sliceRows(const std::string& name, const std::int8_t comparison,
                                         const int threshold) const {
  return sliceRows<int>(name, comparison, threshold);
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
                                         const std::string threshold) const {
  return sliceRows<std::string>(name, comparison, threshold);
}

void osdf::ViewCols::print() {
  data_.print();
}

void osdf::ViewCols::setUpdatedObjects(const ColumnMetadata& columnMetadata,
    const std::vector<std::int64_t>& ids, const std::vector<std::shared_ptr<DataBase>>& dataCols) {
  data_.setColumnMetadata(columnMetadata);
  data_.setIds(ids);
  data_.setDataCols(dataCols);
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
      const std::string errMsg = std::string("ERROR: Input vector for column \"") + name +
            std::string("\" is not the required data type.");
      throw eckit::BadParameter(errMsg, Here());
    }
  } else {
    const std::string errMsg = std::string("ERROR: Column named \"") + name +
          std::string("\" not found in current data frame.");
    throw eckit::BadParameter(errMsg, Here());
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
    const std::string errMsg = std::string("ERROR: Column named \"") + name +
          std::string("\" not found in current data frame.");
    throw eckit::BadParameter(errMsg, Here());
  }
  return ViewCols(newColumnMetadata, newIds, newDataColumns, parent_);
}

void osdf::ViewCols::clear() {
  data_.clear();
}
