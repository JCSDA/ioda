/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/containers/ViewRows.h"

#include "eckit/exception/Exceptions.h"
#include "ioda/containers/DatumBase.h"
#include "ioda/containers/FrameRows.h"

osdf::ViewRows::ViewRows(const ColumnMetadata& columnMetadata,
      const std::vector<DataRow*>& dataRows, FrameRows* parent) :
    data_(funcs_, columnMetadata, dataRows), parent_(parent) {
  parent_->attach(this);
}

osdf::ViewRows::~ViewRows() {
  parent_->detach(this);
  clear();
}

void osdf::ViewRows::getColumn(const std::string& name, std::vector<int>& values) const {
  getColumn<int>(name, values, consts::eInt);
}

void osdf::ViewRows::getColumn(const std::string& name, std::vector<std::int64_t>& values) const {
  getColumn<std::int64_t>(name, values, consts::eInt64);
}

void osdf::ViewRows::getColumn(const std::string& name, std::vector<float>& values) const {
  getColumn<float>(name, values, consts::eFloat);
}

void osdf::ViewRows::getColumn(const std::string& name, std::vector<std::string>& values) const {
  getColumn<std::string>(name, values, consts::eString);
}

osdf::ViewRows osdf::ViewRows::sliceRows(const std::string& name,
                                         const consts::eComparisons comparison,
                                         const int threshold) const {
  return sliceRows<int>(name, comparison, threshold);
}

osdf::ViewRows osdf::ViewRows::sliceRows(const std::string& name,
                                         const consts::eComparisons comparison,
                                         const std::int64_t threshold) const {
  return sliceRows<std::int64_t>(name, comparison, threshold);
}

osdf::ViewRows osdf::ViewRows::sliceRows(const std::string& name,
                                         const consts::eComparisons comparison,
                                         const float threshold) const {
  return sliceRows<float>(name, comparison, threshold);
}

osdf::ViewRows osdf::ViewRows::sliceRows(const std::string& name,
                                         const consts::eComparisons comparison,
                                         const std::string threshold) const {
  return sliceRows<std::string>(name, comparison, threshold);
}

void osdf::ViewRows::sortRows(const std::string& columnName, const consts::eSortOrders order) {
  if (data_.columnExists(columnName) == true) {
    const std::int32_t index = data_.getIndex(columnName);
    if (order == consts::eAscending) {
      reorderDataRows(index, [&](std::shared_ptr<DatumBase>& datumA,
                                 std::shared_ptr<DatumBase>& datumB) {
        return funcs_.compareDatums(datumA, datumB);
      });
    } else if (order == consts::eDescending) {
      reorderDataRows(index, [&](std::shared_ptr<DatumBase>& datumA,
                                 std::shared_ptr<DatumBase>& datumB) {
        return funcs_.compareDatums(datumB, datumA);
      });
    } else {
      const std::string errMsg = std::string("ERROR: Invalid sort order: ") + std::to_string(order)
                                 + std::string(". Must be either eAscending or eDescending.");
      throw eckit::BadParameter(errMsg, Here());
    }
  } else {
    const std::string errMsg = std::string("ERROR: Column named \"") + columnName +
          std::string("\" not found in current data frame.");
    throw eckit::BadParameter(errMsg, Here());
  }
}

void osdf::ViewRows::sortRows(const std::string& columnName, const std::function<std::int8_t(
     const std::shared_ptr<DatumBase>, const std::shared_ptr<DatumBase>)> func) {
  if (data_.columnExists(columnName) == true) {
    // Build list of ordered indices.
    const std::int32_t index = data_.getIndex(columnName);
    const std::int64_t sizeRows = data_.getSizeRows();
    const std::size_t sizeRowsSz = static_cast<std::size_t>(sizeRows);
    std::vector<std::int64_t> indices(sizeRowsSz, 0);
    std::iota(std::begin(indices), std::end(indices), 0);    // Initial sequential list of indices.
    std::sort(std::begin(indices), std::end(indices), [&](const std::int64_t& i,
                                                          const std::int64_t& j) {
      std::shared_ptr<DatumBase>& datumA = data_.getDataRow(i)->getColumn(index);
      std::shared_ptr<DatumBase>& datumB = data_.getDataRow(j)->getColumn(index);
      return func(datumA, datumB);
    });
    // Swap data values for whole rows - casting makes it look more confusing than it is.
    for (std::size_t i = 0; i < sizeRowsSz; ++i) {
      while (indices.at(i) != indices.at(static_cast<std::size_t>(indices.at(i)))) {
        const std::size_t iIdx = static_cast<std::size_t>(indices.at(i));
        DataRow* a = data_.getDataRow(indices.at(i));
        DataRow* b = data_.getDataRow(indices.at(iIdx));
        data_.setDataRow(indices.at(i), b);
        data_.setDataRow(indices.at(iIdx), a);
        std::swap(indices.at(i), indices.at(iIdx));
      }
    }
  } else {
    const std::string errMsg = std::string("ERROR: Column named \"") + columnName +
          std::string("\" not found in current data frame.");
    throw eckit::BadParameter(errMsg, Here());
  }
}

void osdf::ViewRows::print() {
  data_.print();
}

void osdf::ViewRows::setUpdatedObjects(const ColumnMetadata& columnMetadata,
                                       const std::vector<DataRow*>& dataRows) {
  data_.setColumnMetadata(columnMetadata);
  data_.setDataRows(dataRows);
}

/// Private functions

template<typename T>
void osdf::ViewRows::getColumn(const std::string& name, std::vector<T>& values,
                               const consts::eDataTypes type) const {
  if (data_.columnExists(name) == true)  {
    const std::int32_t columnIndex = data_.getIndex(name);
    const consts::eDataTypes columnType = data_.getType(columnIndex);
    if (type == columnType) {
      values.resize(static_cast<std::size_t>(data_.getSizeRows()));
      for (std::int32_t rowIndex = 0; rowIndex < data_.getSizeRows(); ++rowIndex) {
        const std::shared_ptr<DatumBase>& datum =
                                          data_.getDataRow(rowIndex)->getColumn(columnIndex);
        const T value = funcs_.getDatumValue<T>(datum);
        values.at(static_cast<std::size_t>(rowIndex)) = value;
      }
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

template <typename T>
osdf::ViewRows osdf::ViewRows::sliceRows(const std::string& name,
                                         const consts::eComparisons comparison,
                                         const T threshold) const {
  std::vector<DataRow*> newDataRows;
  ColumnMetadata newColumnMetadata;
  if (data_.columnExists(name) == true)  {
    newDataRows.reserve(static_cast<std::size_t>(data_.getSizeRows()));
    newColumnMetadata = data_.getColumnMetadata();
    newColumnMetadata.resetMaxId();  // Only relevant for column alignment when printing
    const std::int32_t index = data_.getIndex(name);
    const std::vector<DataRow*>& dataRows = data_.getDataRows();
    std::copy_if(dataRows.begin(), dataRows.end(), std::back_inserter(newDataRows),
                                                   [&](const DataRow* dataRow) {
      const std::shared_ptr<DatumBase>& datum = dataRow->getColumn(index);
      const T value = funcs_.getDatumValue<T>(datum);
      const bool retVal = funcs_.compareToThreshold(comparison, threshold, value);
      if (retVal == true) {
        newColumnMetadata.updateMaxId(dataRow->getId());
      }
      return retVal;
    });
  } else {
    const std::string errMsg = std::string("ERROR: Column named \"") + name +
          std::string("\" not found in current data frame.");
    throw eckit::BadParameter(errMsg, Here());
  }
  newDataRows.shrink_to_fit();
  return ViewRows(newColumnMetadata, newDataRows, parent_);
}

osdf::ViewRows osdf::ViewRows::sliceRows(
      const std::function<const std::int8_t(const DataRow*)> func) const {
  std::vector<DataRow*> newDataRows;
  ColumnMetadata newColumnMetadata = data_.getColumnMetadata();
  newDataRows.reserve(static_cast<std::size_t>(data_.getSizeRows()));
  newColumnMetadata.resetMaxId();  // Only relevant for column alignment when printing
  const std::vector<DataRow*>& dataRows = data_.getDataRows();
  std::copy_if(dataRows.begin(), dataRows.end(),
               std::back_inserter(newDataRows), [&](const DataRow* dataRow) {
    newColumnMetadata.updateMaxId(dataRow->getId());
    return func(dataRow);
  });
  newDataRows.shrink_to_fit();
  return ViewRows(newColumnMetadata, newDataRows, parent_);
}

void osdf::ViewRows::clear() {
  data_.clear();
}
