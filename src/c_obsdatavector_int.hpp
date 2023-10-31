/*
 * (C) Copyright 2023 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#pragma once
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

#include "ioda/C/ioda_c_utils.hpp"
#include "ioda/C/ioda_decls.hpp"
#include "ioda/C/cxx_string.hpp"
#include "ioda/ObsDataVector.h"
DECL_TYPE(obsdatavector_int_t)
extern "C" {
const int * obsdatavector_int_c_get_row_i(obsdatavector_int_t p, size_t i);
const int * obsdatavector_int_c_get_row_cxx_str(obsdatavector_int_t p, cxx_string_t s);
const int * obsdatavector_int_c_get_row_str(obsdatavector_int_t p, const char *str);
int obsdatavector_int_get(obsdatavector_int_t p, size_t i, size_t j);
int64_t obsdatavector_int_c_nvars(obsdatavector_int_t p);
int64_t obsdatavector_int_c_nlocs(obsdatavector_int_t p);
}

