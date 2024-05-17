/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "DatumInt16.h"

#include "Constants.h"

DatumInt16::DatumInt16(const std::int16_t& datum) :
  DatumBase(consts::eInt16), datum_(datum) {}

const std::string DatumInt16::getDatumStr() const {
  return std::to_string(datum_);
}

const std::int16_t DatumInt16::getDatum() const {
  return datum_;
}

std::int16_t DatumInt16::getDatum() {
  return datum_;
}

void DatumInt16::setDatum(std::int16_t datum) {
  datum_ = datum;
}
