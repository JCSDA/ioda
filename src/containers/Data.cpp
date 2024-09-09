/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/containers/Data.h"

#include <cstdint>

#include "ioda/containers/Constants.h"

template<> osdf::Data<std::int8_t>::Data(const std::vector<std::int8_t>& values) :
           DataBase(consts::eInt8), values_(values) {}
template<> osdf::Data<std::int16_t>::Data(const std::vector<std::int16_t>& values) :
           DataBase(consts::eInt16), values_(values) {}
template<> osdf::Data<std::int32_t>::Data(const std::vector<std::int32_t>& values) :
           DataBase(consts::eInt32), values_(values) {}
template<> osdf::Data<std::int64_t>::Data(const std::vector<std::int64_t>& values) :
           DataBase(consts::eInt64), values_(values) {}
template<> osdf::Data<float>::Data(const std::vector<float>& values) :
           DataBase(consts::eFloat), values_(values) {}
template<> osdf::Data<double>::Data(const std::vector<double>& values) :
           DataBase(consts::eDouble), values_(values) {}
template<> osdf::Data<std::string>::Data(const std::vector<std::string>& values) :
           DataBase(consts::eString), values_(values) {}

template<class T>
const std::string osdf::Data<T>::getValueStr(const std::int64_t rowIndex) const {
  return std::to_string(values_.at(static_cast<std::size_t>(rowIndex)));
}

template const std::string osdf::Data<std::int8_t>::getValueStr(const std::int64_t) const;
template const std::string osdf::Data<std::int16_t>::getValueStr(const std::int64_t) const;
template const std::string osdf::Data<std::int32_t>::getValueStr(const std::int64_t) const;
template const std::string osdf::Data<std::int64_t>::getValueStr(const std::int64_t) const;
template const std::string osdf::Data<float>::getValueStr(const std::int64_t) const;
template const std::string osdf::Data<double>::getValueStr(const std::int64_t) const;

template<>
const std::string osdf::Data<std::string>::getValueStr(const std::int64_t rowIndex) const {
  return values_.at(static_cast<std::size_t>(rowIndex));
}
