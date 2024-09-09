/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/containers/Functions.h"
#include "ioda/Exception.h"

#include "ioda/containers/Constants.h"
#include "ioda/containers/Data.h"
#include "ioda/containers/Datum.h"

osdf::Functions::Functions() {}

template<> void osdf::Functions::addColumnToRow<const char*>(IFrameData* data, DataRow& row,
    std::int8_t& isValid, std::int32_t& columnIndex, const char* param) const {
  std::string paramStr = std::string(param);
  addColumnToRow<std::string>(data, row, isValid, columnIndex, paramStr);
}

template<typename T>
void osdf::Functions::addColumnToRow(IFrameData* data, DataRow& row,
    std::int8_t& isValid, std::int32_t& columnIndex, const T param) const {
  if (isValid == true) {
    columnIndex = row.getSize();
    const std::string name = data->getName(columnIndex);
    const std::int8_t type = data->getType(columnIndex);
    std::shared_ptr<DatumBase> newDatum = createDatum<T>(param);
    if (newDatum->getType() == type) {
      row.insert(newDatum);
    } else {
      isValid = false;
    }
  }
}

template void osdf::Functions::addColumnToRow<std::int8_t>(IFrameData* data, DataRow& row,
    std::int8_t& isValid, std::int32_t&, const std::int8_t param) const;
template void osdf::Functions::addColumnToRow<std::int16_t>(IFrameData* data, DataRow& row,
    std::int8_t& isValid, std::int32_t&, const std::int16_t param) const;
template void osdf::Functions::addColumnToRow<std::int32_t>(IFrameData* data, DataRow& row,
    std::int8_t& isValid, std::int32_t&, const std::int32_t param) const;
template void osdf::Functions::addColumnToRow<std::int64_t>(IFrameData* data, DataRow& row,
    std::int8_t& isValid, std::int32_t&, const std::int64_t param) const;
template void osdf::Functions::addColumnToRow<float>(IFrameData* data, DataRow& row,
    std::int8_t& isValid, std::int32_t&, const float param) const;
template void osdf::Functions::addColumnToRow<double>(IFrameData* data, DataRow& row,
    std::int8_t& isValid, std::int32_t&, const double param) const;
template void osdf::Functions::addColumnToRow<std::string>(IFrameData* data, DataRow& row,
    std::int8_t& isValid, std::int32_t&, const std::string param) const;

template<typename T> const std::shared_ptr<osdf::DataBase> osdf::Functions::createData(
                                                     const std::vector<T>& values) const {
  std::shared_ptr<Data<T>> data = std::make_shared<Data<T>>(values);
  return data;
}

template const std::shared_ptr<osdf::DataBase>
    osdf::Functions::createData<std::int8_t>(const std::vector<std::int8_t>&) const;
template const std::shared_ptr<osdf::DataBase>
    osdf::Functions::createData<std::int16_t>(const std::vector<std::int16_t>&) const;
template const std::shared_ptr<osdf::DataBase>
    osdf::Functions::createData<std::int32_t>(const std::vector<std::int32_t>&) const;
template const std::shared_ptr<osdf::DataBase>
    osdf::Functions::createData<std::int64_t>(const std::vector<std::int64_t>&) const;
template const std::shared_ptr<osdf::DataBase>
    osdf::Functions::createData<float>(const std::vector<float>&) const;
template const std::shared_ptr<osdf::DataBase>
    osdf::Functions::createData<double>(const std::vector<double>&) const;
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
    osdf::Functions::createDatum<std::int8_t>(const std::int8_t) const;
template const std::shared_ptr<osdf::DatumBase>
    osdf::Functions::createDatum<std::int16_t>(const std::int16_t) const;
template const std::shared_ptr<osdf::DatumBase>
    osdf::Functions::createDatum<std::int32_t>(const std::int32_t) const;
template const std::shared_ptr<osdf::DatumBase>
    osdf::Functions::createDatum<std::int64_t>(const std::int64_t) const;
template const std::shared_ptr<osdf::DatumBase>
    osdf::Functions::createDatum<float>(const float) const;
template const std::shared_ptr<osdf::DatumBase>
    osdf::Functions::createDatum<double>(const double) const;
template const std::shared_ptr<osdf::DatumBase>
    osdf::Functions::createDatum<std::string>(const std::string) const;

template<>
const std::shared_ptr<osdf::DatumBase> osdf::Functions::createDatum<char const*>(
                                                                   char const* value) const {
  std::string valStr = std::string(value);
  std::shared_ptr<Datum<std::string>> datum = std::make_shared<Datum<std::string>>(valStr);
  return datum;
}

const std::int8_t osdf::Functions::compareDatums(const std::shared_ptr<osdf::DatumBase>& datumA,
    const std::shared_ptr<DatumBase>& datumB) const {
  switch (datumA->getType()) {
    case consts::eInt8: {
      const std::shared_ptr<Datum<std::int8_t>>& datumAType =
                                  std::static_pointer_cast<Datum<std::int8_t>>(datumA);
      const std::shared_ptr<Datum<std::int8_t>>& datumBType =
                                  std::static_pointer_cast<Datum<std::int8_t>>(datumB);
      return datumAType->getValue() < datumBType->getValue();
    }
    case consts::eInt16: {
      const std::shared_ptr<Datum<std::int16_t>>& datumAType =
                                  std::static_pointer_cast<Datum<std::int16_t>>(datumA);
      const std::shared_ptr<Datum<std::int16_t>>& datumBType =
                                  std::static_pointer_cast<Datum<std::int16_t>>(datumB);
      return datumAType->getValue() < datumBType->getValue();
    }
    case consts::eInt32: {
      const std::shared_ptr<Datum<std::int32_t>>& datumAType =
                                  std::static_pointer_cast<Datum<std::int32_t>>(datumA);
      const std::shared_ptr<Datum<std::int32_t>>& datumBType =
                                  std::static_pointer_cast<Datum<std::int32_t>>(datumB);
      return datumAType->getValue() < datumBType->getValue();
    }
    case consts::eInt64: {
      const std::shared_ptr<Datum<std::int64_t>>& datumAType =
                                  std::static_pointer_cast<Datum<std::int64_t>>(datumA);
      const std::shared_ptr<Datum<std::int64_t>>& datumBType =
                                  std::static_pointer_cast<Datum<std::int64_t>>(datumB);
      return datumAType->getValue() < datumBType->getValue();
    }
    case consts::eFloat: {
      const std::shared_ptr<Datum<float>>& datumAType =
                                  std::static_pointer_cast<Datum<float>>(datumA);
      const std::shared_ptr<Datum<float>>& datumBType =
                                  std::static_pointer_cast<Datum<float>>(datumB);
      return datumAType->getValue() < datumBType->getValue();
    }
    case consts::eDouble: {
      const std::shared_ptr<Datum<double>>& datumAType =
                                  std::static_pointer_cast<Datum<double>>(datumA);
      const std::shared_ptr<Datum<double>>& datumBType =
                                  std::static_pointer_cast<Datum<double>>(datumB);
      return datumAType->getValue() < datumBType->getValue();
    }
    case consts::eString: {
      const std::shared_ptr<Datum<std::string>>& datumAType =
                                  std::static_pointer_cast<Datum<std::string>>(datumA);
      const std::shared_ptr<Datum<std::string>>& datumBType =
                                  std::static_pointer_cast<Datum<std::string>>(datumB);
      return datumAType->getValue() < datumBType->getValue();
    }
    default:
      throw ioda::Exception("ERROR: Missing type specification.", ioda_Here());
  }
}


template<typename T>
const std::int8_t osdf::Functions::compareToThreshold(const std::int8_t comparison,
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

template const std::int8_t osdf::Functions::compareToThreshold<std::int8_t>(
                           const std::int8_t, const std::int8_t, const std::int8_t) const;
template const std::int8_t osdf::Functions::compareToThreshold<std::int16_t>(
                           const std::int8_t, const std::int16_t, const std::int16_t) const;
template const std::int8_t osdf::Functions::compareToThreshold<std::int32_t>(
                           const std::int8_t, const std::int32_t, const std::int32_t) const;
template const std::int8_t osdf::Functions::compareToThreshold<std::int64_t>(
                           const std::int8_t, const std::int64_t, const std::int64_t) const;
template const std::int8_t osdf::Functions::compareToThreshold<float>(
                           const std::int8_t, const float, const float) const;
template const std::int8_t osdf::Functions::compareToThreshold<double>(
                           const std::int8_t, const double, const double) const;
template const std::int8_t osdf::Functions::compareToThreshold<std::string>(
                           const std::int8_t, const std::string, const std::string) const;

template<typename T> const std::vector<T>& osdf::Functions::getDataValues(
                                           const std::shared_ptr<DataBase>& data) const {
  std::shared_ptr<Data<T>> dataType = std::static_pointer_cast<Data<T>>(data);
  return dataType->getValues();
}

template const std::vector<std::int8_t>& osdf::Functions::getDataValues<std::int8_t>(
                                         const std::shared_ptr<DataBase>&) const;
template const std::vector<std::int16_t>& osdf::Functions::getDataValues<std::int16_t>(
                                         const std::shared_ptr<DataBase>&) const;
template const std::vector<std::int32_t>& osdf::Functions::getDataValues<std::int32_t>(
                                         const std::shared_ptr<DataBase>&) const;
template const std::vector<std::int64_t>& osdf::Functions::getDataValues<std::int64_t>(
                                         const std::shared_ptr<DataBase>&) const;
template const std::vector<float>& osdf::Functions::getDataValues<float>(
                                         const std::shared_ptr<DataBase>&) const;
template const std::vector<double>& osdf::Functions::getDataValues<double>(
                                         const std::shared_ptr<DataBase>&) const;
template const std::vector<std::string>& osdf::Functions::getDataValues<std::string>(
                                         const std::shared_ptr<DataBase>&) const;

template<typename T> std::vector<T>& osdf::Functions::getDataValues(
                                     std::shared_ptr<DataBase>& data) {
  std::shared_ptr<Data<T>> dataType = std::static_pointer_cast<Data<T>>(data);
  return dataType->getValues();
}

template std::vector<std::int8_t>& osdf::Functions::getDataValues<std::int8_t>(
                                   std::shared_ptr<DataBase>&);
template std::vector<std::int16_t>& osdf::Functions::getDataValues<std::int16_t>(
                                   std::shared_ptr<DataBase>&);
template std::vector<std::int32_t>& osdf::Functions::getDataValues<std::int32_t>(
                                   std::shared_ptr<DataBase>&);
template std::vector<std::int64_t>& osdf::Functions::getDataValues<std::int64_t>(
                                   std::shared_ptr<DataBase>&);
template std::vector<float>& osdf::Functions::getDataValues<float>(
                                   std::shared_ptr<DataBase>&);
template std::vector<double>& osdf::Functions::getDataValues<double>(
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
