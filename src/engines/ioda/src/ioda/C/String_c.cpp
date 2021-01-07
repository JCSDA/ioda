/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// \file String_c.cpp
/// \brief C bindings for variable-length strings. Needed for reads.

#include "ioda/C/String_c.h"

#include <gsl/gsl-lite.hpp>

extern "C" {

IODA_DL void ioda_string_ret_t_destruct(ioda_string_ret_t* obj) {
  C_TRY;
  Expects(obj != nullptr);
  Expects(obj->strings != nullptr);
  for (size_t i = 0; i < obj->n; ++i) {
    if (obj->strings[i] != nullptr) {
      delete[] obj->strings[i];
    }
  }
  delete[] obj->strings;
  delete obj;
  C_CATCH_AND_TERMINATE;
}
}
