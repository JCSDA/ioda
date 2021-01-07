#pragma once
/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// \file String_c.h
/// \brief C bindings for variable-length strings. Needed for reads.
#include "../defs.h"
#include "./c_binding_macros.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ioda_string_ret_t {
  size_t n;
  char** strings;
};

IODA_DL void ioda_string_ret_t_destruct(struct ioda_string_ret_t*);

struct c_strings {
  void (*destruct)(struct ioda_string_ret_t*);
};

#ifdef __cplusplus
}
#endif
