/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "DatumDouble.h"

#include "Constants.h"

DatumDouble::DatumDouble(const double& datum) :
  DatumBase(consts::eDouble), datum_(datum) {}

const std::string DatumDouble::getDatumStr() const {
  return std::to_string(datum_);
}

const double DatumDouble::getDatum() const {
  return datum_;
}

double DatumDouble::getDatum() {
  return datum_;
}

void DatumDouble::setDatum(double datum) {
  datum_ = datum;
}
