/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "Datum.h"

#include <cstdint>

template<>
  Datum<std::int8_t>::Datum(std::int8_t datum): DatumBase(consts::eInt8), datum_(datum) {}
template<>
  Datum<std::int16_t>::Datum(std::int16_t datum): DatumBase(consts::eInt16), datum_(datum) {}
template<>
  Datum<std::int32_t>::Datum(std::int32_t datum): DatumBase(consts::eInt32), datum_(datum) {}
template<>
  Datum<std::int64_t>::Datum(std::int64_t datum): DatumBase(consts::eInt64), datum_(datum) {}
template<>
  Datum<float>::Datum(float datum): DatumBase(consts::eFloat), datum_(datum) {}
template<>
  Datum<double>::Datum(double datum): DatumBase(consts::eDouble), datum_(datum) {}
template<>
  Datum<std::string>::Datum(std::string datum): DatumBase(consts::eString), datum_(datum) {}

template<class T>
const std::string Datum<T>::getDatumStr() const {
  return std::to_string(datum_);
}

template const std::string Datum<std::int8_t>::getDatumStr() const;
template const std::string Datum<std::int16_t>::getDatumStr() const;
template const std::string Datum<std::int32_t>::getDatumStr() const;
template const std::string Datum<std::int64_t>::getDatumStr() const;
template const std::string Datum<float>::getDatumStr() const;
template const std::string Datum<double>::getDatumStr() const;

template<>
const std::string Datum<std::string>::getDatumStr() const {
  return datum_;
}
