/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "DatumBase.h"

DatumBase::DatumBase(const std::int8_t type) : type_(type) {}

const std::int8_t DatumBase::getType() const {
  return type_;
}
