/*
 * (C) Copyright 2017 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#include "ioda/obsspace_f.h"

#include <string>

namespace ioda {

// -----------------------------------------------------------------------------

int obsspace_get_nobs_f(const ObsSpace & obss) {
  return obss.nobs();
}

// -----------------------------------------------------------------------------

int obsspace_get_nlocs_f(const ObsSpace & obss) {
  return obss.nlocs();
}

// -----------------------------------------------------------------------------

void obsspace_get_var_f(const ObsSpace & obss, const char vname[],
                        const int Vsize, double Vdata[]) {
  std::string Vname(vname);
  return obss.getvar(Vname, Vsize, Vdata);
}

// -----------------------------------------------------------------------------

}  // namespace ioda
