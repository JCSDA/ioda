/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/containers/ObsDataFrame.h"

#include "ioda/containers/Data.h"
#include "ioda/containers/Datum.h"

osdf::ObsDataFrame::ObsDataFrame(const std::int8_t type) : type_(type) {}

osdf::ObsDataFrame::ObsDataFrame(ColumnMetadata columnMetadata, const std::int8_t type) :
  columnMetadata_(columnMetadata), type_(type) {}

void osdf::ObsDataFrame::configColumns(std::vector<ColumnMetadatum> cols) {
  columnMetadata_.add(std::move(cols));
}

void osdf::ObsDataFrame::configColumns(std::initializer_list<ColumnMetadatum> initList) {
  std::vector<ColumnMetadatum> cols;
  std::copy(std::begin(initList), std::end(initList), std::back_inserter(cols));
  columnMetadata_.add(std::move(cols));
}

const std::int8_t osdf::ObsDataFrame::getType() {
  return type_;
}

template<typename T>
std::shared_ptr<osdf::DataBase> osdf::funcs::createData(const std::int32_t& columnIndex,
                                                        const std::vector<T>& values) {
  std::shared_ptr<Data<T>> data = std::make_shared<Data<T>>(values);
  return data;
}

template std::shared_ptr<osdf::DataBase> osdf::funcs::createData<std::int8_t>(
                                         const std::int32_t&, const std::vector<std::int8_t>&);
template std::shared_ptr<osdf::DataBase> osdf::funcs::createData<std::int16_t>(
                                         const std::int32_t&, const std::vector<std::int16_t>&);
template std::shared_ptr<osdf::DataBase> osdf::funcs::createData<std::int32_t>(
                                         const std::int32_t&, const std::vector<std::int32_t>&);
template std::shared_ptr<osdf::DataBase> osdf::funcs::createData<std::int64_t>(
                                         const std::int32_t&, const std::vector<std::int64_t>&);
template std::shared_ptr<osdf::DataBase> osdf::funcs::createData<float>(
                                         const std::int32_t&, const std::vector<float>&);
template std::shared_ptr<osdf::DataBase> osdf::funcs::createData<double>(
                                         const std::int32_t&, const std::vector<double>&);
template std::shared_ptr<osdf::DataBase> osdf::funcs::createData<std::string>(
                                         const std::int32_t&, const std::vector<std::string>&);

template<>
std::shared_ptr<osdf::DataBase> osdf::funcs::createData<const char*>(
                                                        const std::int32_t& columnIndex,
                                                        const std::vector<const char*>& values) {
  std::vector<std::string> valueStrings;
  for (const char* value : values) {
    valueStrings.push_back(std::string(value));
  }
  std::shared_ptr<Data<std::string>> data = std::make_shared<Data<std::string>>(valueStrings);
  return data;
}

template<typename T>
std::shared_ptr<osdf::DatumBase> osdf::funcs::createDatum(const std::int32_t& columnIndex,
                                                          const T value) {
  std::shared_ptr<Datum<T>> datum = std::make_shared<Datum<T>>(value);
  return datum;
}

template std::shared_ptr<osdf::DatumBase> osdf::funcs::createDatum<std::int8_t>(
                                          const std::int32_t&, const std::int8_t);
template std::shared_ptr<osdf::DatumBase> osdf::funcs::createDatum<std::int16_t>(
                                          const std::int32_t&, const std::int16_t);
template std::shared_ptr<osdf::DatumBase> osdf::funcs::createDatum<std::int32_t>(
                                          const std::int32_t&, const std::int32_t);
template std::shared_ptr<osdf::DatumBase> osdf::funcs::createDatum<std::int64_t>(
                                          const std::int32_t&, const std::int64_t);
template std::shared_ptr<osdf::DatumBase> osdf::funcs::createDatum<float>(
                                          const std::int32_t&, const float);
template std::shared_ptr<osdf::DatumBase> osdf::funcs::createDatum<double>(
                                          const std::int32_t&, const double);
template std::shared_ptr<osdf::DatumBase> osdf::funcs::createDatum<std::string>(
                                          const std::int32_t&, const std::string);

template<>
std::shared_ptr<osdf::DatumBase> osdf::funcs::createDatum<char const*>(
                                 const std::int32_t& columnIndex, char const* value) {
  std::string valStr = std::string(value);
  std::shared_ptr<Datum<std::string>> datum = std::make_shared<Datum<std::string>>(valStr);
  return datum;
}
