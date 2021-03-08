#pragma once
/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \addtogroup ioda_cxx_variable
 * \addtogroup ioda_cxx_attribute
 * @{
 * \file Dimensions.h
 * \brief Describe the dimensions of a ioda::Attribute or ioda::Variable.
 */
#include <vector>

#include "ioda/defs.h"

namespace ioda {
/// \brief Describes the dimensions of an Attribute or Variable.
/// \ingroup ioda_cxx_variable
/// \ingroup ioda_cxx_attribute
struct Dimensions {
  std::vector<Dimensions_t> dimsCur;  ///< The dimensions of the data.
  std::vector<Dimensions_t> dimsMax;  ///< This must always equal dimsCur for Attribute.
  Dimensions_t dimensionality;        ///< The dimensionality (rank) of the data.
  Dimensions_t numElements;           ///< The number of elements of data (PROD(dimsCur)).
  /// Convenient constructor function.
  Dimensions(const std::vector<Dimensions_t>& dimscur, const std::vector<Dimensions_t>& dimsmax,
             Dimensions_t dality, Dimensions_t np)
      : dimsCur(dimscur), dimsMax(dimsmax), dimensionality(dality), numElements(np) {}
  Dimensions() : dimensionality(0), numElements(0) {}
};
}  // namespace ioda

/// @}
