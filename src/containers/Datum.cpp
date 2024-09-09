/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/containers/Datum.h"

#include <cstdint>

#include "ioda/containers/Constants.h"

template<> osdf::Datum<std::int8_t>::Datum(const std::int8_t& value):
           DatumBase(consts::eInt8), value_(value) {}
template<> osdf::Datum<std::int16_t>::Datum(const std::int16_t& value):
           DatumBase(consts::eInt16), value_(value) {}
template<> osdf::Datum<std::int32_t>::Datum(const std::int32_t& value):
           DatumBase(consts::eInt32), value_(value) {}
template<> osdf::Datum<std::int64_t>::Datum(const std::int64_t& value):
           DatumBase(consts::eInt64), value_(value) {}
template<> osdf::Datum<float>::Datum(const float& value):
           DatumBase(consts::eFloat), value_(value) {}
template<> osdf::Datum<double>::Datum(const double& value):
           DatumBase(consts::eDouble), value_(value) {}
template<> osdf::Datum<std::string>::Datum(const std::string& value):
           DatumBase(consts::eString), value_(value) {}

template<class T>
const std::string osdf::Datum<T>::getValueStr() const {
  return std::to_string(value_);
}

template const std::string osdf::Datum<std::int8_t>::getValueStr() const;
template const std::string osdf::Datum<std::int16_t>::getValueStr() const;
template const std::string osdf::Datum<std::int32_t>::getValueStr() const;
template const std::string osdf::Datum<std::int64_t>::getValueStr() const;
template const std::string osdf::Datum<float>::getValueStr() const;
template const std::string osdf::Datum<double>::getValueStr() const;

template<>
const std::string osdf::Datum<std::string>::getValueStr() const {
  return value_;
}
