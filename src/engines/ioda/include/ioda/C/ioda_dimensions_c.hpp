/*
 * (C) Copyright 2022-2023 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#pragma once
#include <cstdlib>
#include <vector>
#include <iostream>
#include <stdexcept>
#include <cstdint>

#include "ioda/C/ioda_decls.hpp"
#include "ioda/Misc/Dimensions.h"
#include "ioda/C/ioda_c_utils.hpp"

extern "C" {

ioda_dimensions_t  ioda_dimensions_c_alloc();
void ioda_dimensions_c_set(ioda_dimensions_t *v,
    int64_t ndim,int64_t n_curr_dim,int64_t n_max_dim,
    int64_t * max_dims,
    int64_t * cur_dims);

void ioda_dimensions_c_dtor(ioda_dimensions_t *v);
void ioda_dimensions_c_clone(ioda_dimensions_t *t_p,ioda_dimensions_t rhs_p);
int64_t ioda_dimensions_c_get_dimensionality(ioda_dimensions_t v);
int64_t ioda_dimensions_c_num_of_elements(ioda_dimensions_t v);
void ioda_dimensions_c_get_dims_cur(ioda_dimensions_t v,int64_t * dims,int *ndims_);
void ioda_dimensions_c_get_dims_max(ioda_dimensions_t v,int64_t * dims,int *ndims_);
int64_t ioda_dimensions_c_get_dims_cur_size(ioda_dimensions_t v);
int64_t ioda_dimensions_c_get_dims_max_size(ioda_dimensions_t v);

} // end extern "C"