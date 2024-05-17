/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "DatumInt32.h"

#include "Constants.h"

DatumInt32::DatumInt32(const std::int32_t& datum) :
  DatumBase(consts::eInt32), datum_(datum) {}

const std::string DatumInt32::getDatumStr() const {
  return std::to_string(datum_);
}

const std::int32_t DatumInt32::getDatum() const {
  return datum_;
}

std::int32_t DatumInt32::getDatum() {
  return datum_;
}

void DatumInt32::setDatum(std::int32_t datum) {
  datum_ = datum;
}
