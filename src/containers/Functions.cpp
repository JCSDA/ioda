/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/containers/Functions.h"
#include "ioda/Exception.h"

#include "ioda/containers/Data.h"
#include "ioda/containers/Datum.h"
#include "ioda/containers/FrameUtils.h"
#include "ioda/containers/DataRow.h"

osdf::Functions::Functions() {}

template<> void osdf::Functions::addColumnToRow<const char*>(IFrameData* data, DataRow& row,
    bool& isValid, std::int32_t& columnIndex, const char* param) const {
  std::string paramStr = std::string(param);
  addColumnToRow<std::string>(data, row, isValid, columnIndex, paramStr);
}

template<typename T>
void osdf::Functions::addColumnToRow(IFrameData* data, DataRow& row,
    bool& isValid, std::int32_t& columnIndex, const T param) const {
  if (isValid == true) {
    columnIndex = row.getSize();

    const consts::eDataTypes type = data->getType(columnIndex);
    std::shared_ptr<DatumBase> newDatum = createDatum<T>(param);
    if (newDatum->getType() == type) {
      row.insert(newDatum);
    } else {
      isValid = false;
    }
  }
}

template void osdf::Functions::addColumnToRow<>(IFrameData* data, DataRow& row,
    bool& isValid, std::int32_t&, const int param) const;
template void osdf::Functions::addColumnToRow<std::int64_t>(IFrameData* data, DataRow& row,
    bool& isValid, std::int32_t&, const std::int64_t param) const;
template void osdf::Functions::addColumnToRow<float>(IFrameData* data, DataRow& row,
    bool& isValid, std::int32_t&, const float param) const;
template void osdf::Functions::addColumnToRow<char>(IFrameData* data, DataRow& row,
    bool& isValid, std::int32_t&, const char param) const;
template void osdf::Functions::addColumnToRow<std::string>(IFrameData* data, DataRow& row,
    bool& isValid, std::int32_t&, const std::string param) const;

template<typename T> const std::shared_ptr<osdf::DataBase> osdf::Functions::createData(
                                                     const std::vector<T>& values) const {
  std::shared_ptr<Data<T>> data = std::make_shared<Data<T>>(values);
  return data;
}

template const std::shared_ptr<osdf::DataBase>
    osdf::Functions::createData<int>(const std::vector<int>&) const;
template const std::shared_ptr<osdf::DataBase>
    osdf::Functions::createData<std::int64_t>(const std::vector<std::int64_t>&) const;
template const std::shared_ptr<osdf::DataBase>
    osdf::Functions::createData<float>(const std::vector<float>&) const;
template const std::shared_ptr<osdf::DataBase>
    osdf::Functions::createData<char>(const std::vector<char>&) const;
  template const std::shared_ptr<osdf::DataBase>
    osdf::Functions::createData<std::string>(const std::vector<std::string>&) const;

template<>
const std::shared_ptr<osdf::DataBase> osdf::Functions::createData<const char*>(
                                      const std::vector<const char*>& values) const {
  std::vector<std::string> valueStrings;
  for (const char* value : values) {
    valueStrings.push_back(std::string(value));
  }
  std::shared_ptr<Data<std::string>> data = std::make_shared<Data<std::string>>(valueStrings);
  return data;
}

template<typename T>
const std::shared_ptr<osdf::DatumBase> osdf::Functions::createDatum(const T value) const {
  std::shared_ptr<Datum<T>> datum = std::make_shared<Datum<T>>(value);
  return datum;
}

template const std::shared_ptr<osdf::DatumBase>
    osdf::Functions::createDatum<int>(const int) const;
template const std::shared_ptr<osdf::DatumBase>
    osdf::Functions::createDatum<std::int64_t>(const std::int64_t) const;
template const std::shared_ptr<osdf::DatumBase>
    osdf::Functions::createDatum<float>(const float) const;
template const std::shared_ptr<osdf::DatumBase>
    osdf::Functions::createDatum<char>(const char) const;
template const std::shared_ptr<osdf::DatumBase>
    osdf::Functions::createDatum<std::string>(const std::string) const;

template<>
const std::shared_ptr<osdf::DatumBase> osdf::Functions::createDatum<char const*>(
                                                                   char const* value) const {
  std::string valStr = std::string(value);
  std::shared_ptr<Datum<std::string>> datum = std::make_shared<Datum<std::string>>(valStr);
  return datum;
}

const bool osdf::Functions::compareDatums(const std::shared_ptr<osdf::DatumBase>& datumA,
    const std::shared_ptr<DatumBase>& datumB) const {
  return osdf::FrameUtils::callWithSupportedType(
    datumA->getType(),
    [&](auto typeDiscriminator) {
      using T = decltype(typeDiscriminator);
      const std::shared_ptr<Datum<T>>& datumAType = std::static_pointer_cast<Datum<T>>(datumA);
      const std::shared_ptr<Datum<T>>& datumBType = std::static_pointer_cast<Datum<T>>(datumB);
      return datumAType->getValue() < datumBType->getValue();
    });
}


template<typename T>
const bool osdf::Functions::compareToThreshold(const consts::eComparisons comparison,
                                                      const T threshold,
                                                      const T value) const {
  switch (comparison) {
    case consts::eLessThan: return value < threshold;
    case consts::eLessThanOrEqualTo: return value <= threshold;
    case consts::eEqualTo: return value == threshold;
    case consts::eGreaterThan: return value > threshold;
    case consts::eGreaterThanOrEqualTo: return value >= threshold;
    default:
      throw ioda::Exception("ERROR: Invalid comparison operator specification.", ioda_Here());
  }
}

template const bool osdf::Functions::compareToThreshold<int>(
                           const consts::eComparisons, const int, const int) const;
template const bool osdf::Functions::compareToThreshold<std::int64_t>(const consts::eComparisons,
                                                                      const std::int64_t,
                                                                      const std::int64_t) const;
template const bool osdf::Functions::compareToThreshold<float>(
                           const consts::eComparisons, const float, const float) const;
template const bool osdf::Functions::compareToThreshold<char>(
                           const consts::eComparisons, const char, const char) const;
template const bool osdf::Functions::compareToThreshold<std::string>(
                           const consts::eComparisons, const std::string, const std::string) const;

template<typename T> const std::vector<T>& osdf::Functions::getDataValues(
                                           const std::shared_ptr<DataBase>& data) const {
  std::shared_ptr<Data<T>> dataType = std::static_pointer_cast<Data<T>>(data);
  return dataType->getValues();
}

template const std::vector<int>& osdf::Functions::getDataValues<int>(
                                         const std::shared_ptr<DataBase>&) const;
template const std::vector<std::int64_t>& osdf::Functions::getDataValues<std::int64_t>(
                                         const std::shared_ptr<DataBase>&) const;
template const std::vector<float>& osdf::Functions::getDataValues<float>(
                                         const std::shared_ptr<DataBase>&) const;
template const std::vector<char>& osdf::Functions::getDataValues<char>(
                                         const std::shared_ptr<DataBase>&) const;
template const std::vector<std::string>& osdf::Functions::getDataValues<std::string>(
                                         const std::shared_ptr<DataBase>&) const;

template<typename T> std::vector<T>& osdf::Functions::getDataValues(
                                     std::shared_ptr<DataBase>& data) {
  std::shared_ptr<Data<T>> dataType = std::static_pointer_cast<Data<T>>(data);
  return dataType->getValues();
}

template std::vector<int>& osdf::Functions::getDataValues<int>(
                                   std::shared_ptr<DataBase>&);
template std::vector<std::int64_t>& osdf::Functions::getDataValues<std::int64_t>(
                                   std::shared_ptr<DataBase>&);
template std::vector<float>& osdf::Functions::getDataValues<float>(
                                   std::shared_ptr<DataBase>&);
template std::vector<char>& osdf::Functions::getDataValues<char>(
                                   std::shared_ptr<DataBase>&);
template std::vector<std::string>& osdf::Functions::getDataValues<std::string>(
                                   std::shared_ptr<DataBase>&);

const std::string osdf::Functions::padString(std::string str,
                                             const std::int32_t columnWidth) const {
  const std::int32_t diff = columnWidth - static_cast<std::int32_t>(str.size());
  if (diff > 0) {
    str.insert(str.end(), static_cast<std::size_t>(diff), osdf::consts::kSpace[0]);
  }
  return str;
}
