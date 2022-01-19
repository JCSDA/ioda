#pragma once
/*
 * (C) Copyright 2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! @file Units.h
* @brief UDUNITS-2 bindings and wrappers
*/
#include <memory>
#include <string>

namespace ioda_validate {
namespace units {

struct ConvertResult {
  bool canConvert       = false;  ///< Input and output units are interconvertible.
  bool equivalentUnits  = false;  ///< Input and output units are equivalent. No conversion needed.
  bool validInputUnits  = false;  ///< Are the input units even valid?
  bool validOutputUnits = false;  ///< Are the output units even valid?
};

struct udunits_interface_impl;

class udunits_interface {
 private:
  static void _static_init();
  udunits_interface();
  std::unique_ptr<udunits_interface_impl> impl_;

 public:
  ~udunits_interface();
  static const udunits_interface &instance();

  ConvertResult canConvert(const std::string &inUnits, const std::string &outUnits) const;
};

}  // end namespace units
}  // end namespace ioda_validate
