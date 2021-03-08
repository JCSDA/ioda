/*
 * (C) Copyright 2017-2020 Ryan Honeyager (ryan@honeyager.info)
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \addtogroup ioda_internals_engines_hh
 *
 * @{
 * \file HH-attributes.cpp
 * \brief HDF5 engine implementation of Attribute.
 */

#include "./HH/HH-attributes.h"

#include "./HH/HH-types.h"
#include "ioda/Misc/Dimensions.h"

namespace ioda {
namespace detail {
namespace Engines {
namespace HH {
HH_Attribute::HH_Attribute() : attr_(HH_hid_t::dummy()) {}
HH_Attribute::HH_Attribute(HH_hid_t h) : attr_(h) {}
HH_Attribute::~HH_Attribute() = default;

detail::Type_Provider* HH_Attribute::getTypeProvider() const {
  return HH_Type_Provider::instance();
}

HH_hid_t HH_Attribute::get() const { return attr_; }

bool HH_Attribute::isAttribute() const {
  H5I_type_t typ = H5Iget_type(attr_());
  if (typ == H5I_BADID) throw;  // HH_throw;
  return (typ == H5I_ATTR);
}

std::string HH_Attribute::getName() const {
  ssize_t sz = H5Aget_name(attr_(), 0, nullptr);
  if (sz < 0) throw;
  auto s = gsl::narrow<size_t>(sz);
  std::vector<char> v(s + 1, '\0');
  sz = H5Aget_name(attr_(), v.size(), v.data());
  if (sz < 0) throw;
  return std::string(v.data());
}

void HH_Attribute::write(gsl::span<const char> data, HH_hid_t in_memory_dataType) {
  if (H5Awrite(attr_(), in_memory_dataType(), data.data()) < 0)
    throw;  // HH_throw.add("Reason", "H5Awrite failed.");
}

Attribute HH_Attribute::write(gsl::span<char> data, const Type& in_memory_dataType) {
  auto typeBackend = std::dynamic_pointer_cast<HH_Type>(in_memory_dataType.getBackend());
  write(data, typeBackend->handle);
  return Attribute{shared_from_this()};
}

void HH_Attribute::read(gsl::span<char> data, HH_hid_t in_memory_dataType) const {
  herr_t ret = H5Aread(attr_(), in_memory_dataType(), static_cast<void*>(data.data()));
  if (ret < 0) throw;  // HH_throw.add("Reason", "H5Aread failed.");
}

Attribute HH_Attribute::read(gsl::span<char> data, const Type& in_memory_dataType) const {
  auto typeBackend = std::dynamic_pointer_cast<HH_Type>(in_memory_dataType.getBackend());
  read(data, typeBackend->handle);

  return Attribute{
    std::make_shared<HH_Attribute>(*this)};  // For const-ness instead of shared_from_this.
}

bool HH_Attribute::isA(HH_hid_t ttype) const {
  HH_hid_t otype = type();
  auto ret       = H5Tequal(ttype(), otype());
  if (ret < 0) throw;
  return (ret > 0) ? true : false;
}

bool HH_Attribute::isA(Type lhs) const {
  auto typeBackend = std::dynamic_pointer_cast<HH_Type>(lhs.getBackend());

  // Override for old-format ioda files:
  // Unfortunately, v0 ioda files have an odd mixture
  // of ascii vs unicode strings, as well as
  // fixed and variable-length strings.
  // We try and fix a few of these issues here.

  // Do we expect a string of any type?
  // Is the object a string of any type?
  // If both are true, then we just return true.
  H5T_class_t cls_lhs = H5Tget_class(typeBackend->handle.get());
  H5T_class_t cls_my  = H5Tget_class(type()());
  if (cls_lhs == H5T_STRING && cls_my == H5T_STRING) return true;

  return isA(typeBackend->handle);
}

HH_hid_t HH_Attribute::type() const {
  return HH_hid_t(H5Aget_type(attr_()), Handles::Closers::CloseHDF5Datatype::CloseP);
}

HH_hid_t HH_Attribute::space() const {
  return HH_hid_t(H5Aget_space(attr_()), Handles::Closers::CloseHDF5Dataspace::CloseP);
}

Dimensions HH_Attribute::getDimensions() const {
  Dimensions ret;

  std::vector<hsize_t> dims;
  if (H5Sis_simple(space()()) < 0) throw;
  hssize_t numPoints = H5Sget_simple_extent_npoints(space()());
  int dimensionality = H5Sget_simple_extent_ndims(space()());
  if (dimensionality < 0) throw;
  dims.resize(dimensionality);
  if (H5Sget_simple_extent_dims(space()(), dims.data(), nullptr) < 0) throw;

  ret.numElements    = gsl::narrow<decltype(Dimensions::numElements)>(numPoints);
  ret.dimensionality = gsl::narrow<decltype(Dimensions::dimensionality)>(dimensionality);
  for (const auto& d : dims) ret.dimsCur.push_back(gsl::narrow<Dimensions_t>(d));
  for (const auto& d : dims) ret.dimsMax.push_back(gsl::narrow<Dimensions_t>(d));

  return ret;
}
}  // namespace HH
}  // namespace Engines
}  // namespace detail
}  // namespace ioda

/// @}
