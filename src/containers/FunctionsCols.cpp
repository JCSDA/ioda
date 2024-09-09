/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/containers/FunctionsCols.h"

#include <algorithm>
#include <utility>

#include "ioda/containers/Constants.h"
#include "ioda/containers/Data.h"
#include "ioda/containers/Datum.h"
#include "ioda/containers/FrameColsData.h"

osdf::FunctionsCols::FunctionsCols() {}

template<typename T> void osdf::FunctionsCols::addDatumValue(
    const std::shared_ptr<DataBase>& data, const std::shared_ptr<DatumBase>& datum) const {
  std::shared_ptr<Data<T>> dataType = std::static_pointer_cast<Data<T>>(data);
  std::shared_ptr<Datum<T>> datumType = std::static_pointer_cast<Datum<T>>(datum);
  dataType->addValue(datumType->getValue());
}

template void osdf::FunctionsCols::addDatumValue<std::int8_t>(const std::shared_ptr<DataBase>&,
                                                        const std::shared_ptr<DatumBase>&) const;
template void osdf::FunctionsCols::addDatumValue<std::int16_t>(const std::shared_ptr<DataBase>&,
                                                        const std::shared_ptr<DatumBase>&) const;
template void osdf::FunctionsCols::addDatumValue<std::int32_t>(const std::shared_ptr<DataBase>&,
                                                        const std::shared_ptr<DatumBase>&) const;
template void osdf::FunctionsCols::addDatumValue<std::int64_t>(const std::shared_ptr<DataBase>&,
                                                        const std::shared_ptr<DatumBase>&) const;
template void osdf::FunctionsCols::addDatumValue<float>(const std::shared_ptr<DataBase>&,
                                                        const std::shared_ptr<DatumBase>&) const;
template void osdf::FunctionsCols::addDatumValue<double>(const std::shared_ptr<DataBase>&,
                                                        const std::shared_ptr<DatumBase>&) const;
template void osdf::FunctionsCols::addDatumValue<std::string>(const std::shared_ptr<DataBase>&,
                                                        const std::shared_ptr<DatumBase>&) const;

template<typename T>
void osdf::FunctionsCols::setDataValues(const std::shared_ptr<DataBase>& data,
                                        const std::vector<T>& values) const {
  const std::shared_ptr<Data<T>>& dataType = std::static_pointer_cast<Data<T>>(data);
  dataType->setValues(values);
}

template void osdf::FunctionsCols::setDataValues<std::int8_t>(
    const std::shared_ptr<DataBase>&, const std::vector<std::int8_t>&) const;
template void osdf::FunctionsCols::setDataValues<std::int16_t>(
    const std::shared_ptr<DataBase>&, const std::vector<std::int16_t>&) const;
template void osdf::FunctionsCols::setDataValues<std::int32_t>(
    const std::shared_ptr<DataBase>&, const std::vector<std::int32_t>&) const;
template void osdf::FunctionsCols::setDataValues<std::int64_t>(
    const std::shared_ptr<DataBase>&, const std::vector<std::int64_t>&) const;
template void osdf::FunctionsCols::setDataValues<float>(
    const std::shared_ptr<DataBase>&, const std::vector<float>&) const;
template void osdf::FunctionsCols::setDataValues<double>(
    const std::shared_ptr<DataBase>&, const std::vector<double>&) const;
template void osdf::FunctionsCols::setDataValues<std::string>(
    const std::shared_ptr<DataBase>&, const std::vector<std::string>&) const;

template<typename T> void osdf::FunctionsCols::removeDatum(std::shared_ptr<DataBase>& data,
                                                                const std::int64_t index) const {
  std::shared_ptr<Data<T>> dataType = std::static_pointer_cast<Data<T>>(data);
  dataType->removeValue(index);
}

template void osdf::FunctionsCols::removeDatum<std::int8_t>(std::shared_ptr<DataBase>&,
                                                                 const std::int64_t) const;
template void osdf::FunctionsCols::removeDatum<std::int16_t>(std::shared_ptr<DataBase>&,
                                                                  const std::int64_t) const;
template void osdf::FunctionsCols::removeDatum<std::int32_t>(std::shared_ptr<DataBase>&,
                                                                  const std::int64_t) const;
template void osdf::FunctionsCols::removeDatum<std::int64_t>(std::shared_ptr<DataBase>&,
                                                                  const std::int64_t) const;
template void osdf::FunctionsCols::removeDatum<float>(std::shared_ptr<DataBase>&,
                                                           const std::int64_t) const;
template void osdf::FunctionsCols::removeDatum<double>(std::shared_ptr<DataBase>&,
                                                             const std::int64_t) const;
template void osdf::FunctionsCols::removeDatum<std::string>(std::shared_ptr<DataBase>&,
                                                                 const std::int64_t) const;

template<typename T>
void osdf::FunctionsCols::sequenceIndices(std::vector<std::int64_t>& indices,
                   const std::vector<T>& values, const std::int8_t order) const {
  if (order == consts::eAscending) {
    std::sort(std::begin(indices), std::end(indices), [&](const std::int64_t& i,
              const std::int64_t& j) {
      return values.at(static_cast<std::size_t>(i)) < values.at(static_cast<std::size_t>(j));
    });
  } else if (order == consts::eDescending) {
    std::sort(std::begin(indices), std::end(indices), [&](const std::int64_t& i,
              const std::int64_t& j) {
      return values.at(static_cast<std::size_t>(j)) < values.at(static_cast<std::size_t>(i));
    });
  }
}

template void osdf::FunctionsCols::sequenceIndices<std::int8_t>(
    std::vector<std::int64_t>&, const std::vector<std::int8_t>&, const std::int8_t) const;
template void osdf::FunctionsCols::sequenceIndices<std::int16_t>(
  std::vector<std::int64_t>&, const std::vector<std::int16_t>&, const std::int8_t) const;
template void osdf::FunctionsCols::sequenceIndices<std::int32_t>(
    std::vector<std::int64_t>&, const std::vector<std::int32_t>&, const std::int8_t) const;
template void osdf::FunctionsCols::sequenceIndices<std::int64_t>(
    std::vector<std::int64_t>&, const std::vector<std::int64_t>&, const std::int8_t) const;
template void osdf::FunctionsCols::sequenceIndices<float>(
    std::vector<std::int64_t>&, const std::vector<float>&, const std::int8_t) const;
template void osdf::FunctionsCols::sequenceIndices<double>(
    std::vector<std::int64_t>&, const std::vector<double>&, const std::int8_t) const;
template void osdf::FunctionsCols::sequenceIndices<std::string>(
    std::vector<std::int64_t>&, const std::vector<std::string>&, const std::int8_t) const;

template<typename T>
void osdf::FunctionsCols::reorderValues(std::vector<std::int64_t> indices,
                                        std::vector<T>& values) const {
  for (std::size_t i = 0; i < indices.size(); ++i) {
    while (indices.at(i) != indices.at(static_cast<std::size_t>(indices.at(i)))) {
      const std::size_t iIdx = static_cast<std::size_t>(indices.at(i));
      const std::size_t jIdx = static_cast<std::size_t>(indices.at(iIdx));
      std::swap(values.at(iIdx), values.at(jIdx));
      std::swap(indices.at(i), indices.at(iIdx));
    }
  }
}

template void osdf::FunctionsCols::reorderValues<std::int8_t>(std::vector<std::int64_t>,
                                                              std::vector<std::int8_t>&) const;
template void osdf::FunctionsCols::reorderValues<std::int16_t>(std::vector<std::int64_t>,
                                                               std::vector<std::int16_t>&) const;
template void osdf::FunctionsCols::reorderValues<std::int32_t>(std::vector<std::int64_t>,
                                                               std::vector<std::int32_t>&) const;
template void osdf::FunctionsCols::reorderValues<std::int64_t>(std::vector<std::int64_t>,
                                                               std::vector<std::int64_t>&) const;
template void osdf::FunctionsCols::reorderValues<float>(std::vector<std::int64_t>,
                                                        std::vector<float>&) const;
template void osdf::FunctionsCols::reorderValues<double>(std::vector<std::int64_t>,
                                                         std::vector<double>&) const;
template void osdf::FunctionsCols::reorderValues<std::string>(std::vector<std::int64_t>,
                                                              std::vector<std::string>&) const;

template<typename T> void osdf::FunctionsCols::sliceRows(const osdf::IColsData* data,
    std::vector<std::shared_ptr<DataBase>>& newDataColumns, ColumnMetadata& newColumnMetadata,
    std::vector<std::int64_t>& newIds, const std::string& name, const std::int8_t comparison,
    const T threshold) const {
  newDataColumns.reserve(static_cast<std::size_t>(data->getSizeCols()));
  newColumnMetadata = data->getColumnMetadata();
  newColumnMetadata.resetMaxId();  // Only relevant for column alignment when printing
  const std::int32_t index = data->getIndex(name);
  const std::int64_t sizeRows = data->getSizeRows();
  std::vector<std::int64_t> indices;
  indices.reserve(static_cast<std::size_t>(sizeRows));
  const std::shared_ptr<DataBase>& dataSelect = data->getDataColumn(index);
  const std::vector<T>& values = getDataValues<T>(dataSelect);
  const std::vector<std::int64_t>& ids = data->getIds();
  for (std::int64_t rowIndex = 0; rowIndex < sizeRows; ++rowIndex) {
    const std::size_t rowIdx = static_cast<std::size_t>(rowIndex);
    const T value = values.at(rowIdx);
    const std::int8_t retVal = compareToThreshold(comparison, threshold, value);
    if (retVal == true) {
      indices.push_back(rowIndex);
      newColumnMetadata.updateMaxId(ids.at(rowIdx));
    }
  }
  indices.shrink_to_fit();
  newIds = getSlicedValues<std::int64_t>(ids, indices);
  for (const std::shared_ptr<DataBase>& dataCol : data->getDataCols()) {
    switch (dataCol->getType()) {
      case consts::eInt8: {
        sliceData<std::int8_t>(newDataColumns, dataCol, indices);
        break;
      }
      case consts::eInt16: {
        sliceData<std::int16_t>(newDataColumns, dataCol, indices);
        break;
      }
      case consts::eInt32: {
        sliceData<std::int32_t>(newDataColumns, dataCol, indices);
        break;
      }
      case consts::eInt64: {
        sliceData<std::int64_t>(newDataColumns, dataCol, indices);
        break;
      }
      case consts::eFloat: {
        sliceData<float>(newDataColumns, dataCol, indices);
        break;
      }
      case consts::eDouble: {
        sliceData<double>(newDataColumns, dataCol, indices);
        break;
      }
      case consts::eString: {
        sliceData<std::string>(newDataColumns, dataCol, indices);
        break;
      }
    }
  }
}

template void osdf::FunctionsCols::sliceRows<std::int8_t>(const osdf::IColsData*,
    std::vector<std::shared_ptr<DataBase>>&, ColumnMetadata&,
    std::vector<std::int64_t>&, const std::string&, const std::int8_t, const std::int8_t) const;
template void osdf::FunctionsCols::sliceRows<std::int16_t>(const osdf::IColsData*,
    std::vector<std::shared_ptr<DataBase>>&, ColumnMetadata&,
    std::vector<std::int64_t>&, const std::string&, const std::int8_t, const std::int16_t) const;
template void osdf::FunctionsCols::sliceRows<std::int32_t>(const osdf::IColsData*,
    std::vector<std::shared_ptr<DataBase>>&, ColumnMetadata&,
    std::vector<std::int64_t>&, const std::string&, const std::int8_t, const std::int32_t) const;
template void osdf::FunctionsCols::sliceRows<std::int64_t>(const osdf::IColsData*,
    std::vector<std::shared_ptr<DataBase>>&, ColumnMetadata&,
    std::vector<std::int64_t>&, const std::string&, const std::int8_t, const std::int64_t) const;
template void osdf::FunctionsCols::sliceRows<float>(const osdf::IColsData*,
    std::vector<std::shared_ptr<DataBase>>&, ColumnMetadata&,
    std::vector<std::int64_t>&, const std::string&, const std::int8_t, const float) const;
template void osdf::FunctionsCols::sliceRows<double>(const osdf::IColsData*,
    std::vector<std::shared_ptr<DataBase>>&, ColumnMetadata&,
    std::vector<std::int64_t>&, const std::string&, const std::int8_t, const double) const;
template void osdf::FunctionsCols::sliceRows<std::string>(const osdf::IColsData*,
    std::vector<std::shared_ptr<DataBase>>&, ColumnMetadata&,
    std::vector<std::int64_t>&, const std::string&, const std::int8_t, const std::string) const;

template<typename T> const std::vector<T> osdf::FunctionsCols::getSlicedValues(
                     const std::vector<T>& values, const std::vector<std::int64_t>& indices) const {
  std::vector<T> newValues(indices.size());
  std::transform(indices.begin(), indices.end(), newValues.begin(), [values](std::int64_t index) {
    return values.at(static_cast<std::size_t>(index));
  });
  return newValues;
}

template const std::vector<std::int8_t> osdf::FunctionsCols::getSlicedValues<std::int8_t>(
         const std::vector<std::int8_t>&, const std::vector<std::int64_t>&) const;
template const std::vector<std::int16_t> osdf::FunctionsCols::getSlicedValues<std::int16_t>(
         const std::vector<std::int16_t>&, const std::vector<std::int64_t>&) const;
template const std::vector<std::int32_t> osdf::FunctionsCols::getSlicedValues<std::int32_t>(
         const std::vector<std::int32_t>&, const std::vector<std::int64_t>&) const;
template const std::vector<std::int64_t> osdf::FunctionsCols::getSlicedValues<std::int64_t>(
         const std::vector<std::int64_t>&, const std::vector<std::int64_t>&) const;
template const std::vector<float> osdf::FunctionsCols::getSlicedValues<float>(
         const std::vector<float>&, const std::vector<std::int64_t>&) const;
template const std::vector<double> osdf::FunctionsCols::getSlicedValues<double>(
         const std::vector<double>&, const std::vector<std::int64_t>&) const;
template const std::vector<std::string> osdf::FunctionsCols::getSlicedValues<std::string>(
         const std::vector<std::string>&, const std::vector<std::int64_t>&) const;

template<typename T> void osdf::FunctionsCols::sliceData(
    std::vector<std::shared_ptr<DataBase>>& newDataColumns, const std::shared_ptr<DataBase>& data,
    const std::vector<std::int64_t>& indices) const {
  const std::shared_ptr<Data<T>>& dataType = std::static_pointer_cast<Data<T>>(data);
  const std::vector<T>& values = dataType->getValues();
  std::vector<T> newValues = getSlicedValues(values, indices);
  std::shared_ptr<Data<T>> newData = std::make_shared<Data<T>>(newValues);
  newDataColumns.push_back(newData);
}

template void osdf::FunctionsCols::sliceData<std::int8_t>(
    std::vector<std::shared_ptr<DataBase>>&, const std::shared_ptr<DataBase>&,
    const std::vector<std::int64_t>&) const;
template void osdf::FunctionsCols::sliceData<std::int16_t>(
    std::vector<std::shared_ptr<DataBase>>&, const std::shared_ptr<DataBase>&,
    const std::vector<std::int64_t>&) const;
template void osdf::FunctionsCols::sliceData<std::int32_t>(
    std::vector<std::shared_ptr<DataBase>>&, const std::shared_ptr<DataBase>&,
    const std::vector<std::int64_t>&) const;
template void osdf::FunctionsCols::sliceData<std::int64_t>(
    std::vector<std::shared_ptr<DataBase>>&, const std::shared_ptr<DataBase>&,
    const std::vector<std::int64_t>&) const;
template void osdf::FunctionsCols::sliceData<float>(
    std::vector<std::shared_ptr<DataBase>>&, const std::shared_ptr<DataBase>&,
    const std::vector<std::int64_t>&) const;
template void osdf::FunctionsCols::sliceData<double>(
    std::vector<std::shared_ptr<DataBase>>&, const std::shared_ptr<DataBase>&,
    const std::vector<std::int64_t>&) const;
template void osdf::FunctionsCols::sliceData<std::string>(
    std::vector<std::shared_ptr<DataBase>>&, const std::shared_ptr<DataBase>&,
    const std::vector<std::int64_t>&) const;

template<typename T> void osdf::FunctionsCols::addValueToData(
    std::vector<std::shared_ptr<osdf::DataBase>>& dataCols, const std::shared_ptr<DatumBase>& datum,
    const std::int8_t initColumn, const std::int64_t sizeRows,
    const std::int32_t columnIndex) const {
  const std::shared_ptr<Datum<T>>& datumType = std::static_pointer_cast<Datum<T>>(datum);
  std::shared_ptr<Data<T>> dataType;
  if (initColumn == false) {
    std::shared_ptr<DataBase>& data = dataCols.at(static_cast<std::size_t>(columnIndex));
    dataType = std::static_pointer_cast<Data<T>>(data);
  } else {
    std::vector<T> values;
    dataType = std::make_shared<Data<T>>(values);
    dataType->reserve(sizeRows);
    dataCols.push_back(dataType);
  }
  dataType->addValue(datumType->getValue());
}

template void osdf::FunctionsCols::addValueToData<std::int8_t>(
    std::vector<std::shared_ptr<osdf::DataBase>>&, const std::shared_ptr<DatumBase>&,
    const std::int8_t, const std::int64_t, const std::int32_t) const;
template void osdf::FunctionsCols::addValueToData<std::int16_t>(
    std::vector<std::shared_ptr<osdf::DataBase>>&, const std::shared_ptr<DatumBase>&,
    const std::int8_t, const std::int64_t, const std::int32_t) const;
template void osdf::FunctionsCols::addValueToData<std::int32_t>(
    std::vector<std::shared_ptr<osdf::DataBase>>&, const std::shared_ptr<DatumBase>&,
    const std::int8_t, const std::int64_t, const std::int32_t) const;
template void osdf::FunctionsCols::addValueToData<std::int64_t>(
    std::vector<std::shared_ptr<osdf::DataBase>>&, const std::shared_ptr<DatumBase>&,
    const std::int8_t, const std::int64_t, const std::int32_t) const;
template void osdf::FunctionsCols::addValueToData<float>(
    std::vector<std::shared_ptr<osdf::DataBase>>&, const std::shared_ptr<DatumBase>&,
    const std::int8_t, const std::int64_t, const std::int32_t) const;
template void osdf::FunctionsCols::addValueToData<double>(
    std::vector<std::shared_ptr<osdf::DataBase>>&, const std::shared_ptr<DatumBase>&,
    const std::int8_t, const std::int64_t, const std::int32_t) const;
template void osdf::FunctionsCols::addValueToData<std::string>(
    std::vector<std::shared_ptr<osdf::DataBase>>&, const std::shared_ptr<DatumBase>&,
    const std::int8_t, const std::int64_t, const std::int32_t) const;

template<typename T> void osdf::FunctionsCols::clearData(
    std::shared_ptr<DataBase>& data) const {
  std::shared_ptr<Data<T>> dataType = std::static_pointer_cast<Data<T>>(data);
  dataType->clear();
}

template void osdf::FunctionsCols::clearData<std::int8_t>(std::shared_ptr<DataBase>&) const;
template void osdf::FunctionsCols::clearData<std::int16_t>(std::shared_ptr<DataBase>&) const;
template void osdf::FunctionsCols::clearData<std::int32_t>(std::shared_ptr<DataBase>&) const;
template void osdf::FunctionsCols::clearData<std::int64_t>(std::shared_ptr<DataBase>&) const;
template void osdf::FunctionsCols::clearData<float>(std::shared_ptr<DataBase>&) const;
template void osdf::FunctionsCols::clearData<double>(std::shared_ptr<DataBase>&) const;
template void osdf::FunctionsCols::clearData<std::string>(std::shared_ptr<DataBase>&) const;

template<typename T> const std::int16_t osdf::FunctionsCols::getSize(
    const std::shared_ptr<DataBase>& data, const std::int64_t index) const {
  std::shared_ptr<Data<T>> dataType = std::static_pointer_cast<Data<T>>(data);
  return static_cast<std::int16_t>(dataType->getValueStr(index).size());
}

template const std::int16_t osdf::FunctionsCols::getSize<std::int8_t>(
         const std::shared_ptr<DataBase>&, const std::int64_t) const;
template const std::int16_t osdf::FunctionsCols::getSize<std::int16_t>(
         const std::shared_ptr<DataBase>&, const std::int64_t) const;
template const std::int16_t osdf::FunctionsCols::getSize<std::int32_t>(
         const std::shared_ptr<DataBase>&, const std::int64_t) const;
template const std::int16_t osdf::FunctionsCols::getSize<std::int64_t>(
         const std::shared_ptr<DataBase>&, const std::int64_t) const;
template const std::int16_t osdf::FunctionsCols::getSize<float>(
         const std::shared_ptr<DataBase>&, const std::int64_t) const;
template const std::int16_t osdf::FunctionsCols::getSize<double>(
         const std::shared_ptr<DataBase>&, const std::int64_t) const;
template const std::int16_t osdf::FunctionsCols::getSize<std::string>(
         const std::shared_ptr<DataBase>&, const std::int64_t) const;
