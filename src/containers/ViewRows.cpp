/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/containers/ViewRows.h"

#include "oops/util/Logger.h"

#include "ioda/containers/Constants.h"
#include "ioda/containers/Datum.h"
#include "ioda/containers/DatumBase.h"

osdf::ViewRows::ViewRows(const ColumnMetadata columnMetadata,
      const std::vector<std::shared_ptr<DataRow>> dataRows) :
      data_(funcs_, columnMetadata, dataRows) {}

void osdf::ViewRows::getColumn(const std::string& name, std::vector<std::int8_t>& values) const {
  getColumn<std::int8_t>(name, values, consts::eInt8);
}

void osdf::ViewRows::getColumn(const std::string& name, std::vector<std::int16_t>& values) const {
  getColumn<std::int16_t>(name, values, consts::eInt16);
}

void osdf::ViewRows::getColumn(const std::string& name, std::vector<std::int32_t>& values) const {
  getColumn<std::int32_t>(name, values, consts::eInt32);
}

void osdf::ViewRows::getColumn(const std::string& name, std::vector<std::int64_t>& values) const {
  getColumn<std::int64_t>(name, values, consts::eInt64);
}

void osdf::ViewRows::getColumn(const std::string& name, std::vector<float>& values) const {
  getColumn<float>(name, values, consts::eFloat);
}

void osdf::ViewRows::getColumn(const std::string& name, std::vector<double>& values) const {
  getColumn<double>(name, values, consts::eDouble);
}

void osdf::ViewRows::getColumn(const std::string& name, std::vector<std::string>& values) const {
  getColumn<std::string>(name, values, consts::eString);
}

osdf::ViewRows osdf::ViewRows::sliceRows(const std::string& name, const std::int8_t comparison,
                                         const std::int8_t threshold) const {
  return sliceRows<std::int8_t>(name, comparison, threshold);
}

osdf::ViewRows osdf::ViewRows::sliceRows(const std::string& name, const std::int8_t comparison,
                                         const std::int16_t threshold) const {
  return sliceRows<std::int16_t>(name, comparison, threshold);
}

osdf::ViewRows osdf::ViewRows::sliceRows(const std::string& name, const std::int8_t comparison,
                                         const std::int32_t threshold) const {
  return sliceRows<std::int32_t>(name, comparison, threshold);
}

osdf::ViewRows osdf::ViewRows::sliceRows(const std::string& name, const std::int8_t comparison,
                                         const std::int64_t threshold) const {
  return sliceRows<std::int64_t>(name, comparison, threshold);
}

osdf::ViewRows osdf::ViewRows::sliceRows(const std::string& name, const std::int8_t comparison,
                                         const float threshold) const {
  return sliceRows<float>(name, comparison, threshold);
}

osdf::ViewRows osdf::ViewRows::sliceRows(const std::string& name, const std::int8_t comparison,
                                         const double threshold) const {
  return sliceRows<double>(name, comparison, threshold);
}

osdf::ViewRows osdf::ViewRows::sliceRows(const std::string& name, const std::int8_t comparison,
                                         const std::string threshold) const {
  return sliceRows<std::string>(name, comparison, threshold);
}

void osdf::ViewRows::sortRows(const std::string& columnName, const std::int8_t order) {
  if (data_.columnExists(columnName) == true) {
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
  } else {
    oops::Log::error() << "ERROR: Column named \"" << columnName
                       << "\" not found in current data frame." << std::endl;
  }
}

void osdf::ViewRows::sortRows(const std::string& columnName, const std::function<std::int8_t(
     const std::shared_ptr<DatumBase>, const std::shared_ptr<DatumBase>)> func) {
  if (data_.columnExists(columnName) == true) {
    funcs_.sortRows(&data_, columnName, func);
  } else {
    oops::Log::error() << "ERROR: Column named \"" << columnName
                       << "\" not found in current data frame." << std::endl;
  }
}

void osdf::ViewRows::print() const {
  data_.print();
}

/// Private functions

template<typename T>
void osdf::ViewRows::getColumn(const std::string& name, std::vector<T>& values,
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
osdf::ViewRows osdf::ViewRows::sliceRows(const std::string& name, const std::int8_t comparison,
                                         const T threshold) const {
  std::vector<std::shared_ptr<DataRow>> newDataRows;
  ColumnMetadata newColumnMetadata;
  if (data_.columnExists(name) == true)  {
    newDataRows.reserve(static_cast<std::size_t>(data_.getSizeRows()));
    newColumnMetadata = data_.getColumnMetadata();
    newColumnMetadata.resetMaxId();  // Only relevant for column alignment when printing
    const std::int32_t index = data_.getIndex(name);
    const std::vector<std::shared_ptr<DataRow>>& dataRows = data_.getDataRows();
    std::copy_if(dataRows.begin(), dataRows.end(), std::back_inserter(newDataRows),
                                                   [&](const std::shared_ptr<DataRow>& dataRow) {
      const std::shared_ptr<DatumBase>& datum = dataRow->getColumn(index);
      const T value = funcs_.getDatumValue<T>(datum);
      const std::int8_t retVal = funcs_.compareToThreshold(comparison, threshold, value);
      if (retVal == true) {
        newColumnMetadata.updateMaxId(dataRow->getId());
      }
      return retVal;
    });
  } else {
    oops::Log::error() << "ERROR: Column named \"" << name
                       << "\" not found in current data frame." << std::endl;
  }
  newDataRows.shrink_to_fit();
  return ViewRows(newColumnMetadata, newDataRows);
}

osdf::ViewRows osdf::ViewRows::sliceRows(
      const std::function<const std::int8_t(const std::shared_ptr<DataRow>&)> func) const {
  std::vector<std::shared_ptr<DataRow>> newDataRows;
  ColumnMetadata newColumnMetadata = data_.getColumnMetadata();
  newDataRows.reserve(static_cast<std::size_t>(data_.getSizeRows()));
  newColumnMetadata.resetMaxId();  // Only relevant for column alignment when printing
  const std::vector<std::shared_ptr<DataRow>>& dataRows = data_.getDataRows();
  std::copy_if(dataRows.begin(), dataRows.end(),
               std::back_inserter(newDataRows), [&](const std::shared_ptr<DataRow>& dataRow) {
    newColumnMetadata.updateMaxId(dataRow->getId());
    return func(dataRow);
  });
  newDataRows.shrink_to_fit();
  return ViewRows(newColumnMetadata, newDataRows);
}
