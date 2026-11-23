/*
 * (C) Copyright 2022-2023 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#pragma once
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>

#include "ioda/core/ioda_c_utils.hpp"

extern "C" {
cxx_string_t cxx_string_alloc();
void cxx_string_c_dealloc(cxx_string_t *s);
void cxx_string_c_set(cxx_string_t *s, const char *val);
char *cxx_string_c_get(cxx_string_t s);
void cxx_string_c_copy(cxx_string_t *s, cxx_string_t o);
void cxx_string_c_append_str(cxx_string_t l, cxx_string_t r);
void cxx_string_c_append(cxx_string_t l, const char *r);
int64_t cxx_string_c_size(cxx_string_t s);
void cxx_string_c_clear(cxx_string_t s);
}  // end extern "C"
