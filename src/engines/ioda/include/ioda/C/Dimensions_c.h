#pragma once
/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \defgroup ioda_dimensions Dimensions
 * \brief Provides the C-style interface for ioda::Dimensions objects.
 * \ingroup ioda_c_api
 *
 * @{
 * \file Dimensions_c.h
 * \brief @link ioda_dimensions C bindings @endlink for ioda::Dimensions
 */
#include "../defs.h"
#include "./c_binding_macros.h"

#ifdef __cplusplus
#  include <cstddef>
#else
#  include <stddef.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct ioda_dimensions;

/// \brief Deallocates a dimensions container.
/// \param[in] d is the dimensions object to be destructed.
IODA_DL void ioda_dimensions_destruct(struct ioda_dimensions* d);
/// \brief Gets dimensionality (i.e. number of dimensions)
/// \param[in] d is the dimensions object.
/// \param[out] val stores the dimensionality of d.
/// \return true on success, false on failure.
/// \pre val must be valid (exist and be non-null).
/// \pre d must be valid.
/// \post val is filled with the dimensionality on success. On failure, it is unchanged.
IODA_DL bool ioda_dimensions_get_dimensionality(const struct ioda_dimensions* d, size_t* val);
/// \brief Set dimensionality of the dimensions container.
/// \param[in] d is the dimensions object.
/// \param[in] N is the new dimensionality.
/// \return true on success, false on failure.
/// \pre d must be a valid object.
IODA_DL bool ioda_dimensions_set_dimensionality(struct ioda_dimensions* d, size_t N);
/// \brief Get the number of distinct elements in the dimensions object (the product of each
///   dimension size).
/// \param[in] d is the dimensions object.
/// \param[out] val stores the number of elements.
/// \return true on success, false on failure.
/// \pre d must be a valid object.
/// \pre val must be valid.
/// \post val is filled on function success and is unchanged on failure.
IODA_DL bool ioda_dimensions_get_num_elements(const struct ioda_dimensions* d, size_t* val);
/// \brief Get the current size of the n-th dimension.
/// \param[in] d is the dimensions object.
/// \param n is the nth dimension. Count starts at zero.
/// \param[out] val stores the size of the dimension.
/// \return true on success, false on failure.
/// \pre d must be a valid object.
/// \pre n must be less than the dimensionality of d (see ioda_dimensions_get_dimensionality).
/// \pre val must be a valid for output.
/// \post val is filled on function success and is unchanged on failure.
IODA_DL bool ioda_dimensions_get_dim_cur(const struct ioda_dimensions* d, size_t n, ptrdiff_t* val);
/// \brief Set the current size of the n-th dimension.
/// \param[in] d is the dimensions object.
/// \param n is the nth dimension. Count starts at zero.
/// \param sz is the new size of the dimension.
/// \return true on success, false on failure.
/// \pre d must be a valid object.
/// \pre n must be less than the dimensionality of d (see ioda_dimensions_get_dimensionality).
/// \post Dimension size is only set on success. Unchanged on failure.
IODA_DL bool ioda_dimensions_set_dim_cur(struct ioda_dimensions* d, size_t n, ptrdiff_t sz);
/// \brief Get the maximum size of the n-th dimension.
/// \param[in] d is the dimensions object.
/// \param n is the nth dimension. Count starts at zero.
/// \param[out] val stores the maximum size of the dimension.
/// \return true on success, false on failure.
/// \pre d must be a valid object.
/// \pre n must be less than the dimensionality of d (see ioda_dimensions_get_dimensionality).
/// \pre val must be a valid for output.
/// \post val is filled on function success and is unchanged on failure.
IODA_DL bool ioda_dimensions_get_dim_max(const struct ioda_dimensions* d, size_t n, ptrdiff_t* val);
/// \brief Set the maximum size of the n-th dimension.
/// \param[in] d is the dimensions object.
/// \param n is the nth dimension. Count starts at zero.
/// \param sz is the new size of the dimension.
/// \return true on success, false on failure.
/// \pre d must be a valid object.
/// \pre n must be less than the dimensionality of d (see ioda_dimensions_get_dimensionality).
/// \post Dimension max size is only set on success. Unchanged on failure.
IODA_DL bool ioda_dimensions_set_dim_max(struct ioda_dimensions* d, size_t n, ptrdiff_t sz);

/*!@brief Class-like encapsulation of C dimension-manipulating functions.
 * @see c_ioda for an example.
 * @see use_c_ioda for an example.
 */
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

/// @} // End Doxygen group
