/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "DatumString.h"

#include "Constants.h"

DatumString::DatumString(const std::string& datum) :
  DatumBase(consts::eString), datum_(datum) {}

const std::string DatumString::getDatumStr() const {
  return datum_;
}

const std::string DatumString::getDatum() const {
  return datum_;
}

std::string DatumString::getDatum() {
  return datum_;
}

void DatumString::setDatum(std::string datum) {
  datum_ = datum;
}
