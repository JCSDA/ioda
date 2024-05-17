/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "DatumInt64.h"

#include "Constants.h"

DatumInt64::DatumInt64(const std::int64_t& datum) :
  DatumBase(consts::eInt64), datum_(datum) {}

const std::string DatumInt64::getDatumStr() const {
  return std::to_string(datum_);
}

const std::int64_t DatumInt64::getDatum() const {
  return datum_;
}

std::int64_t DatumInt64::getDatum() {
  return datum_;
}

void DatumInt64::setDatum(std::int64_t datum) {
  datum_ = datum;
}
