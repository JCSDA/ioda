#pragma once
/*
 * (C) Crown copyright 2021, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// \file UnitConversions.h
/// \brief Basic arithmetic unit conversions to SI

#include "ioda/defs.h"

#include <functional>
#include <unordered_map>
#include <string>
#include <vector>

namespace ioda {
namespace detail {

IODA_DL inline double celsiusToKelvin(double temp) {
  return temp + 273.15;
}

IODA_DL inline double knotsToMetersPerSecond(double knots) {
  return knots * 0.514444;
}

IODA_DL inline double percentageToFraction(double percentage) {
  return percentage * 0.01;
}

IODA_DL inline double hectopascalToPascal(double hPa) {
  return hPa * 100;
}

IODA_DL inline double degreesToRadians(double deg) {
  return deg * .0174533;
}

IODA_DL inline double oktaToFraction(double okta) {
  return okta * .125;
}

const std::unordered_map<std::string, std::function<double(double)>> unitConversionEquations {
  {"celsius", celsiusToKelvin},
  {"knot", knotsToMetersPerSecond},
  {"percentage", percentageToFraction},
  {"hectopascal", hectopascalToPascal},
  {"degree", degreesToRadians},
  {"okta", oktaToFraction}
};

const std::unordered_map<std::string, std::string> equivalentSIUnit {
  {"celsius", "kelvin"},
  {"knot", "meters per second"},
  {"percentage", "-"},
  {"hectopascal", "pascal"},
  {"degree", "radian"},
  {"okta", "-"}
};

} // namespace detail

IODA_DL void convertColumn(const std::string &unit, std::vector<double> &dataToConvert);
IODA_DL std::string getSIUnit(const std::string &unit);
} // namespace ioda
