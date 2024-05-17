/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "DatumInt8.h"

#include "Constants.h"

DatumInt8::DatumInt8(const std::int8_t& datum) :
  DatumBase(consts::eInt8), datum_(datum) {}

const std::string DatumInt8::getDatumStr() const {
  return std::to_string(datum_);
}

const std::int8_t DatumInt8::getDatum() const {
  return datum_;
}

std::int8_t DatumInt8::getDatum() {
  return datum_;
}

void DatumInt8::setDatum(std::int8_t datum) {
  datum_ = datum;
}
