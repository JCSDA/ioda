#pragma once
/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// \file Dimensions_c.h
/// \brief C bindings for ioda::Dimensions
#include "../defs.h"
#include "./c_binding_macros.h"

#ifdef __cplusplus
#include <cstddef>
#else
#include <stddef.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct ioda_dimensions;

IODA_DL void ioda_dimensions_destruct(struct ioda_dimensions* d);
IODA_DL bool ioda_dimensions_get_dimensionality(const struct ioda_dimensions* d, size_t* val);
IODA_DL bool ioda_dimensions_set_dimensionality(struct ioda_dimensions* d, size_t N);
IODA_DL bool ioda_dimensions_get_num_elements(const struct ioda_dimensions* d, size_t* val);
IODA_DL bool ioda_dimensions_get_dim_cur(const struct ioda_dimensions* d, size_t n, ptrdiff_t* val);
IODA_DL bool ioda_dimensions_set_dim_cur(struct ioda_dimensions* d, size_t n, ptrdiff_t sz);
IODA_DL bool ioda_dimensions_get_dim_max(const struct ioda_dimensions* d, size_t n, ptrdiff_t* val);
IODA_DL bool ioda_dimensions_set_dim_max(struct ioda_dimensions* d, size_t n, ptrdiff_t sz);

struct c_dimensions {
  void (*destruct)(struct ioda_dimensions*);
  bool (*getDimensionality)(const struct ioda_dimensions*, size_t*);
  bool (*setDimensionality)(struct ioda_dimensions*, size_t);
  bool (*getNumElements)(const struct ioda_dimensions*, size_t*);
  bool (*getDimCur)(const struct ioda_dimensions*, size_t, ptrdiff_t*);
  bool (*setDimCur)(struct ioda_dimensions*, size_t, ptrdiff_t);
  bool (*getDimMax)(const struct ioda_dimensions*, size_t, ptrdiff_t*);
  bool (*setDimMax)(struct ioda_dimensions*, size_t, ptrdiff_t);
};

#ifdef __cplusplus
}
#endif
