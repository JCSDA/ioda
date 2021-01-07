/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// \file Fill.cpp
/// \brief Fill value getters and setters

#include "ioda/Variables/Fill.h"

#include "ioda/Group.h"

namespace ioda {
namespace detail {
FillValueData_t::FillValueUnion_t FillValueData_t::finalize() const {
  FillValueData_t::FillValueUnion_t ret = fillValue_;
  if (isString_) ret.cp = stringFillValue_.c_str();  // NOLINT
  return ret;
}
}  // namespace detail
}  // namespace ioda
