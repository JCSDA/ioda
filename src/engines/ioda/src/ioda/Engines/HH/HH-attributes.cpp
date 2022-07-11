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
#include "./HH/HH-util.h"
#include "ioda/Exception.h"
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
  if (typ == H5I_BADID) throw Exception("H5Iget_type failed.", ioda_Here());
  return (typ == H5I_ATTR);
}

std::string HH_Attribute::getName() const {
  ssize_t sz = H5Aget_name(attr_(), 0, nullptr);
  if (sz < 0) throw Exception("H5Aget_name failed.", ioda_Here());
  auto s = gsl::narrow<size_t>(sz);
  std::vector<char> v(s + 1, '\0');
  sz = H5Aget_name(attr_(), v.size(), v.data());
  if (sz < 0) throw Exception("H5Aget_name failed.", ioda_Here());
  return std::string(v.data());
}

/**
 * @details This function is somewhat complicated because we perform special
 * handling / reprocessing when writing fixed-length string types. This is
 * done as a convenience to end users, since they should not need to
 * use different in-memory data representations for variable-length and
 * fixed-length strings.
 */
void HH_Attribute::write(gsl::span<const char> data, HH_hid_t in_memory_dataType) {
  H5T_class_t memTypeClass = H5Tget_class(in_memory_dataType());
  HH_hid_t attrType(H5Aget_type(attr_()), Handles::Closers::CloseHDF5Datatype::CloseP);
  H5T_class_t attrTypeClass = H5Tget_class(attrType());

  if ((memTypeClass == H5T_STRING) && (attrTypeClass == H5T_STRING)) {
    // Both memory and attribute types are strings. Need to check fixed vs variable length.
    // Variable-length strings (default) are packed as an array of pointers.
    // Fixed-length strings are just a sequence of characters.
    htri_t isMemStrVar  = H5Tis_variable_str(in_memory_dataType());
    htri_t isAttrStrVar = H5Tis_variable_str(attrType());
    if (isMemStrVar < 0)
      throw Exception("H5Tis_variable_str failed on memory data type.", ioda_Here());
    if (isAttrStrVar < 0)
      throw Exception("H5Tis_variable_str failed on backend attribute data type.", ioda_Here());

    if ((isMemStrVar && isAttrStrVar) || (!isMemStrVar && !isAttrStrVar)) {
      // No need to change anything. Pass through.
      // NOTE: Using attrType instead of in_memory_dataType! This is because strings can have
      //   different character sets (ASCII vs UTF-8), which is entirely unhandled in IODA.
      if (H5Awrite(attr_(), attrType(), data.data()) < 0)
        throw Exception("H5Awrite failed.", ioda_Here());
    } else if (isMemStrVar) {
      // Variable-length in memory. Fixed-length in attribute.
      size_t strLen  = H5Tget_size(attrType());
      size_t numStrs = getDimensions().numElements;

      std::vector<char> out_buf = convertVariableLengthToFixedLength(data, strLen, false);

      if (H5Awrite(attr_(), attrType(), out_buf.data()) < 0)
        throw Exception("H5Awrite failed.", ioda_Here());
    } else if (isAttrStrVar) {
      // Fixed-length in memory. Variable-length in attribute.

      size_t strLen      = H5Tget_size(in_memory_dataType());
      size_t numStrs     = getDimensions().numElements;
      size_t totalStrLen = strLen * numStrs;

      auto converted_data_holder = convertFixedLengthToVariableLength(data, strLen);
      char* converted_data = reinterpret_cast<char*>(converted_data_holder.DataPointers.data());

      if (H5Awrite(attr_(), attrType(), converted_data) < 0)
        throw Exception("H5Awrite failed.", ioda_Here());
    }
  } else {
    // Pass-through case.
    if (H5Awrite(attr_(), in_memory_dataType(), data.data()) < 0)
      throw Exception("H5Awrite failed.", ioda_Here());
  }
}

Attribute HH_Attribute::write(gsl::span<const char> data, const Type& in_memory_dataType) {
  auto typeBackend = std::dynamic_pointer_cast<HH_Type>(in_memory_dataType.getBackend());
  write(data, typeBackend->handle);
  return Attribute{shared_from_this()};
}

void HH_Attribute::read(gsl::span<char> data, HH_hid_t in_memory_dataType) const {
  H5T_class_t memTypeClass = H5Tget_class(in_memory_dataType());
  HH_hid_t attrType(H5Aget_type(attr_()), Handles::Closers::CloseHDF5Datatype::CloseP);
  H5T_class_t attrTypeClass = H5Tget_class(attrType());

  if ((memTypeClass == H5T_STRING) && (attrTypeClass == H5T_STRING)) {
    // Both memory and attribute types are strings. Need to check fixed vs variable length.
    // Variable-length strings (default) are packed as an array of pointers.
    // Fixed-length strings are just a sequence of characters.
    htri_t isMemStrVar  = H5Tis_variable_str(in_memory_dataType());
    htri_t isAttrStrVar = H5Tis_variable_str(attrType());
    if (isMemStrVar < 0)
      throw Exception("H5Tis_variable_str failed on memory data type.", ioda_Here());
    if (isAttrStrVar < 0)
      throw Exception("H5Tis_variable_str failed on backend attribute data type.", ioda_Here());

    if ((isMemStrVar && isAttrStrVar) || (!isMemStrVar && !isAttrStrVar)) {
      // No need to change anything. Pass through.
      // NOTE: Using attrType instead of in_memory_dataType! This is because strings can have
      //   different character sets (ASCII vs UTF-8), which is entirely unhandled in IODA.
      herr_t ret = H5Aread(attr_(), attrType(), static_cast<void*>(data.data()));
      if (ret < 0) throw Exception("H5Aread failed.", ioda_Here());
    } else if (isMemStrVar) {
      // Variable-length in memory. Fixed-length in attribute.
      size_t strLen  = H5Tget_size(attrType());
      size_t numStrs = getDimensions().numElements;
      std::vector<char> in_buf(numStrs * strLen);

      if (H5Aread(attr_(), attrType(), in_buf.data()) < 0)
        throw Exception("H5Aread failed.", ioda_Here());

      // This block of code is a bit of a kludge in that we are switching from a packed
      // structure of strings to a packed structure of pointers of strings.
      // The Marshaller code in ioda/Types/Marshalling.h expects this format.
      // In the future, the string read interface in the frontend should use the Marshalling
      // interface to pass these objects back and forth without excessive data element copies.

      char** reint_buf = reinterpret_cast<char**>(data.data());  // NOLINT: casting
      for (size_t i = 0; i < numStrs; ++i) {
        // The malloced strings will be released in Marshalling.h.
        reint_buf[i]
          = (char*)malloc(strLen + 1);  // NOLINT: quite a deliberate call to malloc here.
        memset(reint_buf[i], 0, strLen + 1);
        memcpy(reint_buf[i], in_buf.data() + (strLen * i), strLen);
      }
    } else if (isAttrStrVar) {
      // Fixed-length in memory. Variable-length in attribute.
      // Rare conversion. Included for completeness. Read into a std::array? There is no
      // fixed-length string type in C++.

      size_t strLen      = H5Tget_size(in_memory_dataType());
      size_t numStrs     = getDimensions().numElements;
      size_t totalStrLen = strLen * numStrs;

      std::vector<char> in_buf(numStrs * sizeof(char *));

      if (H5Aread(attr_(), attrType(), in_buf.data()) < 0)
        throw Exception("H5Aread failed.", ioda_Here());

      // We could avoid using the temporary out_buf and write
      // directly to "data", but there is no strong need to do this.
      // 1. This is a very rare type conversion. C++ lacks a fixed-length string type.
      // 2. We are reading an attribute, which by definition is small.
      std::vector<char> out_buf
        = convertVariableLengthToFixedLength(in_buf, strLen, false);
      if (out_buf.size() != data.size())
        throw Exception("Unexpected sizes.", ioda_Here())
          .add("data.size()", data.size())
          .add("out_buf.size()", out_buf.size());
      std::copy(out_buf.begin(), out_buf.end(), data.begin());
    }
  } else {
    // Pass-through case
    herr_t ret = H5Aread(attr_(), in_memory_dataType(), static_cast<void*>(data.data()));
    if (ret < 0) throw Exception("H5Aread failed.", ioda_Here());
  }
}

Attribute HH_Attribute::read(gsl::span<char> data, const Type& in_memory_dataType) const {
  auto typeBackend = std::dynamic_pointer_cast<HH_Type>(in_memory_dataType.getBackend());
  read(data, typeBackend->handle);

  return Attribute{
    std::make_shared<HH_Attribute>(*this)};  // For const-ness instead of shared_from_this.
}

bool HH_Attribute::isA(HH_hid_t ttype) const {
  HH_hid_t otype = internalType();
  auto ret       = H5Tequal(ttype(), otype());
  if (ret < 0) throw Exception("H5Tequal failed.", ioda_Here());
  return (ret > 0) ? true : false;
}

bool HH_Attribute::isA(Type lhs) const {
  try {
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
    H5T_class_t cls_my  = H5Tget_class(internalType()());
    if (cls_lhs == H5T_STRING && cls_my == H5T_STRING) return true;

    return isA(typeBackend->handle);
  } catch (std::bad_cast) {
    std::throw_with_nested(Exception("lhs is not an HH_Type.", ioda_Here()));
  }
}

HH_hid_t HH_Attribute::internalType() const {
  return HH_hid_t(H5Aget_type(attr_()), Handles::Closers::CloseHDF5Datatype::CloseP);
}

Type HH_Attribute::getType() const {
  return Type{std::make_shared<HH_Type>(internalType()), typeid(HH_Type)};
}

HH_hid_t HH_Attribute::space() const {
  return HH_hid_t(H5Aget_space(attr_()), Handles::Closers::CloseHDF5Dataspace::CloseP);
}

Dimensions HH_Attribute::getDimensions() const {
  Dimensions ret;

  std::vector<hsize_t> dims;
  if (H5Sis_simple(space()()) < 0) throw Exception("H5Sis_simple failed.", ioda_Here());
  hssize_t numPoints = H5Sget_simple_extent_npoints(space()());
  int dimensionality = H5Sget_simple_extent_ndims(space()());
  if (dimensionality < 0) throw Exception("H5Sget_simple_extent_ndims failed.", ioda_Here());
  dims.resize(dimensionality);
  if (H5Sget_simple_extent_dims(space()(), dims.data(), nullptr) < 0)
    throw Exception("H5Sget_simple_extent_dims failed.", ioda_Here());

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
