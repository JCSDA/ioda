/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "DatumFloat.h"

#include "Constants.h"

DatumFloat::DatumFloat(const float& datum) :
  DatumBase(consts::eFloat), datum_(datum) {}

const std::string DatumFloat::getDatumStr() const {
  return std::to_string(datum_);
}

const float DatumFloat::getDatum() const {
  return datum_;
}

float DatumFloat::getDatum() {
  return datum_;
}

void DatumFloat::setDatum(float datum) {
  datum_ = datum;
}
