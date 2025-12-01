/*
 * (C) Crown Copyright 2021 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include "ioda/Misc/UnitConversions.h"

#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

#include "ioda/Exception.h"

namespace ioda {
std::string getSIUnit(const std::string &unit) {
  try {
    std::string siUnit = detail::equivalentSIUnit.at(unit);
    return siUnit;
  } catch (const std::out_of_range &) {
    throw Exception("unit does not have a defined unit conversion equation", ioda_Here())
      .add("unit", unit);
  }
}
}  // namespace ioda
