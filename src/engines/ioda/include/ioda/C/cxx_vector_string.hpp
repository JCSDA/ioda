/*
 * (C) Copyright 2022-2023 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#pragma once
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <iostream>

#include "ioda/C/ioda_c_utils.hpp"
#include "ioda/C/ioda_decls.hpp"
#include "ioda/C/cxx_string.hpp"

DECL_TYPE(cxx_vector_string_t)
extern "C" {

cxx_vector_string_t  cxx_vector_string_c_alloc();
void cxx_vector_string_c_dealloc(cxx_vector_string_t *p);
void cxx_vector_string_c_copy(cxx_vector_string_t * v,cxx_vector_string_t o);

int64_t cxx_vector_string_c_element_size(cxx_vector_string_t p,int64_t i);
int64_t cxx_vector_string_c_size(cxx_vector_string_t p);

cxx_string_t cxx_vector_string_c_get_str(cxx_vector_string_t p,int64_t i);
char * cxx_vector_string_c_get(cxx_vector_string_t p,int64_t i);
void cxx_vector_string_c_set_str(cxx_vector_string_t p,int64_t i,cxx_string_t v);
void cxx_vector_string_c_set(cxx_vector_string_t p,int64_t i,const char * v);
void cxx_vector_string_c_push_back_str(cxx_vector_string_t vs,cxx_string_t s);
void cxx_vector_string_c_push_back(cxx_vector_string_t vs,const char *v);
void cxx_vector_string_c_clear(cxx_vector_string_t );
void cxx_vector_string_c_resize(cxx_vector_string_t v,int64_t n);
int cxx_vector_string_c_empty(cxx_vector_string_t v);

}  // end extern "C"


