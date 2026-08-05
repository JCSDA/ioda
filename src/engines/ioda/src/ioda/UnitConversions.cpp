/*
 * (C) Crown Copyright 2021 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include "ioda/Misc/UnitConversions.h"

#include <stdexcept>
#include <string>

namespace ioda {
std::string getSIUnit(const std::string &unit) {
  try {
    std::string siUnit = detail::equivalentSIUnit.at(unit);
    return siUnit;
  } catch (const std::out_of_range &) {
    std::string msg = "Unit does not have defined unit conversion equation: " + unit;
    throw eckit::Exception(msg, Here());
  }
}
}  // namespace ioda
