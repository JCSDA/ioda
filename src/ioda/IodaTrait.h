/*
 * (C) Copyright 2017 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#ifndef IODA_IODATRAIT_H_
#define IODA_IODATRAIT_H_

#include <string>

#include "ioda/ObsDataVector.h"
#include "ioda/ObsSpace.h"
#include "ioda/ObsVector.h"

namespace ioda {

struct IodaTrait {
  static std::string name() {return "IODA";}

  typedef ioda::ObsSpace       ObsSpace;
  typedef ioda::ObsVector      ObsVector;
  template <typename DATATYPE> using ObsDataVector = ioda::ObsDataVector<DATATYPE>;
};

}  // namespace ioda

#endif  // IODA_IODATRAIT_H_
