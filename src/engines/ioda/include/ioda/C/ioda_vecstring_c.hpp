#pragma once
/*
 * (C) Copyright 2022-2023 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include <cstdlib>
#include <vector>
#include <string>
#include <stdexcept>
#include <iostream>
#include <cstring>

#include "ioda_c_utils.hpp"


extern "C" {

// ioda_vecstring_t *
void *  ioda_vecstring_c_alloc();

void ioda_vecstring_c_dealloc(void **p);

void ioda_vecstring_c_clone(void **p,void *rhs);

// ioda_vecstring_t *

void ioda_vecstring_c_copy(void *p,void *pcpy);

void ioda_vecstring_c_set_string_f(void *p,int64_t i,void *pstr);

void ioda_vecstring_c_append_string_f(void *p,int64_t i,void *pstr);

// ioda_string_t *
void *  ioda_vecstring_c_get_string_f(void *p,int64_t i);

void  ioda_vecstring_c_set_f(void *p,int64_t i,void *pstr);

void  ioda_vecstring_c_append_f(void *p,int64_t i,void *pstr);

char * ioda_vecstring_c_get_f(void *p,int64_t i);

void ioda_vecstring_c_clear(void *p);

void ioda_vecstring_c_resize(void *p,int64_t n);

int64_t ioda_vecstring_c_size(void *p);

int64_t ioda_vecstring_c_element_size_f(void *p,int64_t i);

void ioda_vecstring_c_push_back(void *p,void *str);

void ioda_vecstring_c_push_back_string(void *p,void *str);

// ioda_string_t *
void * ioda_string_c_alloc();

void ioda_string_c_dealloc(void *p);

void ioda_string_c_clone(void **p,void *rhs);

void ioda_string_c_set(void *p,void *pstr);

void ioda_string_c_set_string(void *p,void *pstr);

void ioda_string_c_append(void *p,void *pstr);

void ioda_string_c_append_string(void *p,void *pstr);

char * ioda_string_c_get(void *p);

int64_t ioda_string_c_size(void *p);

void ioda_string_c_clear(void *p);

} // end of extern "C"

