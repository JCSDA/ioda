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
#include "ioda/C/ioda_c_utils.hpp"
#include "ioda/C/ioda_decls.hpp"

DECL_TYPE(cxx_vector_int_t)

extern "C" {

cxx_vector_int_t cxx_vector_int_c_alloc();
void cxx_vector_int_c_dealloc(cxx_vector_int_t * p);
void cxx_vector_int_c_copy(cxx_vector_int_t * p, cxx_vector_int_t q);
void cxx_vector_int_c_push_back(cxx_vector_int_t vp,int x);
void cxx_vector_int_c_set(cxx_vector_int_t vp,int64_t i,int x);
int cxx_vector_int_c_get(cxx_vector_int_t vp,int64_t i);
int64_t cxx_vector_int_c_size(cxx_vector_int_t vp);
void cxx_vector_int_c_resize(cxx_vector_int_t vp,int64_t n);
void cxx_vector_int_c_clear(cxx_vector_int_t vp);
int cxx_vector_int_c_empty(cxx_vector_int_t vp);

}
