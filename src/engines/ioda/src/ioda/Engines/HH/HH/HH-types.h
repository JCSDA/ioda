#pragma once
/*
 * (C) Copyright 2017-2020 Ryan Honeyager (ryan@honeyager.info)
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include "./Handles.h"
#include "ioda/Types/Type.h"
#include "ioda/Types/Type_Provider.h"
#include "ioda/defs.h"

namespace ioda {
namespace detail {
namespace Engines {
namespace HH {

/// \brief This is the implementation of Type_Provider using HDF5. Do not use outside of IODA.
class IODA_DL HH_Type_Provider : public detail::Type_Provider {
public:
  virtual ~HH_Type_Provider();
  static HH_hid_t getFundamentalHHType(std::type_index type);
  Type makeFundamentalType(std::type_index type) const final;
  Type makeArrayType(std::initializer_list<Dimensions_t> dimensions, std::type_index typeOuter,
                     std::type_index typeInner) const final;
  Type makeStringType(size_t string_length, std::type_index typeOuter) const final;
  static HH_Type_Provider* instance();
};

/// \brief This is the implementation of ioda::Type using HDF5. Do not use outside of IODA.
class IODA_DL HH_Type : public detail::Type_Backend {
public:
  virtual ~HH_Type();
  HH_hid_t handle;
  HH_Type(HH_hid_t h);
};
}  // namespace HH
}  // namespace Engines
}  // namespace detail
}  // namespace ioda
