/*
 * (C) Copyright 2017 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#ifndef IODA_OBSSPACE_F_H_
#define IODA_OBSSPACE_F_H_

#include "ObsSpace.h"

// -----------------------------------------------------------------------------
// These functions provide a Fortran-callable interface to ObsSpace.
// -----------------------------------------------------------------------------

namespace ioda {

extern "C" {
    int obsspace_get_nobs_f(const ObsSpace & obss);
    void obsspace_get_mdata_f(const ObsSpace & obss, const char [], double [], const int);
}

}  // namespace ioda

#endif  // IODA_OBSSPACE_F_H_
