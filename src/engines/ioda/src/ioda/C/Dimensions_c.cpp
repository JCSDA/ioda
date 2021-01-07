/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// \file Dimensions_c.cpp
/// \brief C bindings for ioda::Dimensions
#include "ioda/C/Dimensions_c.h"

#include <numeric>

#include "ioda/C/structs_c.h"

extern "C" {

void ioda_dimensions_destruct(ioda_dimensions* d) {
  C_TRY;
  Expects(d != nullptr);
  delete d;
  C_CATCH_AND_TERMINATE;
}

bool ioda_dimensions_get_dimensionality(const ioda_dimensions* d, size_t* res) {
  C_TRY;
  Expects(d != nullptr);
  Expects(res != nullptr);
  *res = gsl::narrow<size_t>(d->d.dimensionality);
  C_CATCH_AND_RETURN(true, false);
}

bool ioda_dimensions_set_dimensionality(ioda_dimensions* d, size_t N) {
  C_TRY;
  Expects(d != nullptr);
  d->d.dimensionality = gsl::narrow<ioda::Dimensions_t>(N);
  d->d.dimsCur.resize(N, 0);
  d->d.dimsMax.resize(N, 0);
  d->d.numElements = gsl::narrow<size_t>(std::accumulate(
    d->d.dimsCur.begin(), d->d.dimsCur.end(), (ioda::Dimensions_t)1, std::multiplies<ioda::Dimensions_t>()));
  C_CATCH_AND_RETURN(true, false);
}

bool ioda_dimensions_get_num_elements(const ioda_dimensions* d, size_t* res) {
  C_TRY;
  Expects(d != nullptr);
  Expects(res != nullptr);
  *res = gsl::narrow<size_t>(d->d.numElements);
  C_CATCH_AND_RETURN(true, false);
}

bool ioda_dimensions_get_dim_cur(const ioda_dimensions* d, size_t n, ptrdiff_t* res) {
  C_TRY;
  Expects(d != nullptr);
  Expects(d->d.dimsCur.size() > n);
  Expects(res != nullptr);
  *res = gsl::narrow<ptrdiff_t>(d->d.dimsCur.at(n));
  C_CATCH_AND_RETURN(true, false);
}

bool ioda_dimensions_set_dim_cur(ioda_dimensions* d, size_t n, ptrdiff_t sz) {
  C_TRY;
  Expects(d != nullptr);
  Expects(d->d.dimsCur.size() > n);
  d->d.dimsCur[n] = gsl::narrow<ioda::Dimensions_t>(sz);
  C_CATCH_AND_RETURN(true, false);
}

bool ioda_dimensions_get_dim_max(const ioda_dimensions* d, size_t n, ptrdiff_t* res) {
  C_TRY;
  Expects(d != nullptr);
  Expects(d->d.dimsMax.size() > n);
  Expects(res != nullptr);
  *res = gsl::narrow<ptrdiff_t>(d->d.dimsMax.at(n));
  C_CATCH_AND_RETURN(true, false);
}

bool ioda_dimensions_set_dim_max(ioda_dimensions* d, size_t n, ptrdiff_t sz) {
  C_TRY;
  Expects(d != nullptr);
  Expects(d->d.dimsMax.size() > n);
  d->d.dimsMax[n] = gsl::narrow<ioda::Dimensions_t>(sz);
  C_CATCH_AND_RETURN(true, false);
}
}
