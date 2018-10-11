/*
 * (C) Copyright 2017 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#include "obsspace_f.h"

namespace ioda {

// -----------------------------------------------------------------------------

int obsspace_get_nobs_f(const ObsSpace & obss) {
    return obss.nobs();
}

// -----------------------------------------------------------------------------

const double * obsspace_get_mdata_f(const ObsSpace & obss, const char vname[],
               double Vdata[], const int * Vsize) {
std::cout << "DEBUG: obsspace_get_mdata_f: vname, vsize: " << vname << ", " << vsize << std::endl;
    std::string Vname(vname);
std::cout << "DEBUG: obsspace_get_mdata_f: Vname: " << Vname << std::endl;
    return obss.get_mdata(Vname, Vdata, Vsize);
}

// -----------------------------------------------------------------------------

}  // namespace ioda
