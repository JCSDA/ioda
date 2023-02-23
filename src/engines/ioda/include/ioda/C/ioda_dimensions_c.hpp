#pragma once
/*
 * (C) Copyright 2022-2023 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include <cstdlib>
#include <vector>
#include <iostream>
#include <stdexcept>
#include <cstdint>

#include "ioda/Misc/Dimensions.h"
#include "ioda/C/ioda_c_utils.hpp"

extern "C" {

void * ioda_dimensions_c_alloc();

void ioda_dimensions_c_set(void **v,
    int64_t ndim,int64_t n_curr_dim,int64_t n_max_dim,
    int64_t * max_dims,
    int64_t * cur_dims);

void ioda_dimensions_c_dtor(void **v);

void ioda_dimensions_c_clone(void **v,void *rhs);

int64_t ioda_dimensions_c_get_dimensionality(void *v);

int64_t ioda_dimensions_c_num_of_elements(void *v);

void ioda_dimensions_c_get_dims_cur(void *v,int64_t * dims,int *ndims_);

void ioda_dimensions_c_get_dims_max(void *v,int64_t * dims,int *ndims_);

int64_t ioda_dimensions_c_get_dims_max_size(void *v);

int64_t ioda_dimensions_c_get_dims_cur_size(void *v);

}
