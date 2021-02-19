#pragma once
/*
 * (C) Copyright 2017-2020 Ryan Honeyager (ryan@honeyager.info)
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// \file HH-attributes.h
/// \brief HDF5 engine implementation of Attribute.

#include <string>
#include <vector>

#include "./Handles.h"
#include "ioda/Group.h"
#include "ioda/defs.h"

namespace ioda {
namespace detail {
namespace Engines {
namespace HH {
/// \brief This is the implementation of Attributes using HDF5.
class IODA_HIDDEN HH_Attribute : public ioda::detail::Attribute_Backend,
                                 public std::enable_shared_from_this<HH_Attribute> {
private:
  HH_hid_t attr_;
  detail::Type_Provider* getTypeProvider() const final;

public:
  HH_Attribute();
  HH_Attribute(HH_hid_t hnd_attr);
  virtual ~HH_Attribute();

  HH_hid_t get() const;
  bool isAttribute() const;

  std::string getName() const;

  Attribute write(gsl::span<char> data, const Type& in_memory_dataType) final;
  void write(gsl::span<const char> data, HH_hid_t in_memory_dataType);

  void read(gsl::span<char> data, HH_hid_t in_memory_dataType) const;
  Attribute read(gsl::span<char> data, const Type& in_memory_dataType) const final;

  HH_hid_t type() const;
  bool isA(HH_hid_t ttype) const;
  bool isA(Type lhs) const final;
  HH_hid_t space() const;
  Dimensions getDimensions() const final;
};
}  // namespace HH
}  // namespace Engines
}  // namespace detail
}  // namespace ioda
