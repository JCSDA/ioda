#pragma once
/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \addtogroup ioda_cxx_api
 *
 * @{
 * \file Eigen_Compat.h
 * \brief Convenience functions to work with Eigen objects.
 */
#include <type_traits>
#include <vector>

#include "ioda/defs.h"

// Eigen causes static analysis tools to emit lots of warnings.
#ifdef _MSC_FULL_VER
#  pragma warning(push)
#  pragma warning(disable : 26450)
#  pragma warning(disable : 26495)
#  pragma warning(disable : 26812)
#endif
#include "Eigen/Dense"
#include "unsupported/Eigen/CXX11/Tensor"
#ifdef _MSC_FULL_VER
#  pragma warning(pop)
#endif
#include "ioda/Misc/Dimensions.h"

/// See the [Eigen3 project](http://eigen.tuxfamily.org/) for stuff in this namespace.
namespace Eigen {
// Forward declaration in case Eigen/Dense can not be included.
template <class T>
class PlainObjectBase;
}  // namespace Eigen
namespace ioda {
/// \brief Do we want to auto-resize the Eigen object on read to fit the data being read?
enum class ioda_Eigen_Resize {
  Resize,    ///< Yes
  No_Resize  ///< No
};

namespace detail {
/// Functions to work with Eigen.
namespace EigenCompat {
// This can resize
// template<typename Derived>
// class Eigen::PlainObjectBase< Derived >;
// Other objects can not resize
template <class EigenClass, class ResizeableBase = ::Eigen::PlainObjectBase<EigenClass>>
using CanResize = ::std::is_base_of<ResizeableBase, EigenClass>;

template <class EigenClass>
typename ::std::enable_if<CanResize<EigenClass>::value>::type DoEigenResize(EigenClass& e,
                                                                            ::Eigen::Index rows,
                                                                            ::Eigen::Index cols) {
  e.resize(rows, cols);
}

/// \todo Make a static_assert!
template <class EigenClass>
typename ::std::enable_if<!CanResize<EigenClass>::value>::type DoEigenResize(EigenClass&,
                                                                             ::Eigen::Index,
                                                                             ::Eigen::Index) {
  throw;
}

template <class EigenClass>
Dimensions getTensorDimensions(EigenClass& e) {
  const int numDims = e.NumDimensions;
  const auto& dims  = e.dimensions();

  Dimensions res;
  std::vector<Dimensions_t> hdims;
  Dimensions_t sz = (numDims > 0) ? 1 : 0;
  for (int i = 0; i < numDims; ++i) {
    auto val = gsl::narrow<Dimensions_t>(dims[i]);
    hdims.push_back(val);
    sz *= val;
  }
  res.dimsCur        = hdims;
  res.dimsMax        = hdims;
  res.numElements    = sz;
  res.dimensionality = gsl::narrow<Dimensions_t>(numDims);
  return res;
}
}  // namespace EigenCompat
}  // namespace detail
}  // namespace ioda

/// @}
