#pragma once
/*
 * (C) Crown copyright 2021, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// \file UnitConversions.h
/// \brief Basic arithmetic unit conversions to SI

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include "ioda/Engines/ODC/OdbConstants.h"
#include "ioda/Exception.h"
#include "ioda/Variables/Variable.h"
#include "ioda/defs.h"

namespace ioda {
namespace detail {

inline double celsiusToKelvin(double temp) { return temp + 273.15; }

inline double knotsToMetersPerSecond(double knots) { return knots * 0.514444; }

inline double percentageToFraction(double percentage) { return percentage * 0.01; }

inline double hectopascalToPascal(double hPa) { return hPa * 100; }

inline double degreesToRadians(double deg) { return deg * .0174533; }

inline double oktaToFraction(double okta) { return okta * .125; }

/// @todo Move to source file.
const std::unordered_map<std::string, std::function<double(double)>> unitConversionEquations{
  {"celsius", celsiusToKelvin},         {"knot", knotsToMetersPerSecond},
  {"percentage", percentageToFraction}, {"hectopascal", hectopascalToPascal},
  {"degree", degreesToRadians},         {"okta", oktaToFraction}};

/// @todo Move to source file.
const std::unordered_map<std::string, std::string> equivalentSIUnit{
  {"celsius", "kelvin"},     {"knot", "meters per second"}, {"percentage", "-"},
  {"hectopascal", "pascal"}, {"degree", "radian"},          {"okta", "-"}};

}  // namespace detail

template <typename T>
IODA_DL void convertColumn(const std::string &unit, Variable &variableToConvert) {
  std::vector<T> dataToConvert = variableToConvert.readAsVector<T>();
  try {
    std::function<T(T)> conversionFunction = detail::unitConversionEquations.at(unit);
    for (T &value : dataToConvert) {
      if (value != Engines::ODC::odb_missing<T>()) {
        value = conversionFunction(value);
      }
    }
  } catch (const std::out_of_range &) {
    throw Exception("unit does not have a defined unit conversion equation", ioda_Here())
      .add("unit", unit);
  }
  variableToConvert.write(dataToConvert);
}

IODA_DL std::string getSIUnit(const std::string &unit);
}  // namespace ioda
