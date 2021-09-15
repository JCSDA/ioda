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

void HH_Attribute::write(gsl::span<const char> data, HH_hid_t in_memory_dataType) {
  if (H5Awrite(attr_(), in_memory_dataType(), data.data()) < 0)
    throw Exception("H5Awrite failed.", ioda_Here());
}

Attribute HH_Attribute::write(gsl::span<char> data, const Type& in_memory_dataType) {
  auto typeBackend = std::dynamic_pointer_cast<HH_Type>(in_memory_dataType.getBackend());
  write(data, typeBackend->handle);
  return Attribute{shared_from_this()};
}

void HH_Attribute::read(gsl::span<char> data, HH_hid_t in_memory_dataType) const {
  herr_t ret;
  // Need to determine the memory and attribute (from backend) data types to figure
  // out what to do with string types, primarily with fixed- vs variable-length strings.
  hid_t memType = in_memory_dataType();
  H5T_class_t memTypeClass = H5Tget_class(memType);

  hid_t attrType = H5Aget_type(attr_());
  H5T_class_t attrTypeClass = H5Tget_class(attrType);


  if ((memTypeClass == H5T_STRING) && (attrTypeClass == H5T_STRING)) {
    // Both memory and attribute types are string. Need to check fixed vs variable length
    // string types to decide what to do. In the case of when the ioda::Attribute::read
    // function was called with a variable length string type (eg, std::string), a pointer
    // to a C style string needs to be passed back through the data paramter. In the case
    // where a fixed length string type (eg, std::vector<char>), the actual characters need
    // to be passed back (instead of a pointer to a string).
    //
    // The H5Tis_variable_str function returns:
    //    > 0 --> variable length string
    //   == 0 --> fixed length string
    //    < 0 --> error
    htri_t isMemStrVar = H5Tis_variable_str(memType);
    htri_t isAttrStrVar = H5Tis_variable_str(attrType);
    if (isMemStrVar < 0) {
      throw Exception("H5Tis_variable_str failed on memory data type.", ioda_Here());
    }
    if (isAttrStrVar < 0)  {
      throw Exception("H5Tis_variable_str failed on backend attribute data type.", ioda_Here());
    }
    if (isMemStrVar > 0) {
      if (isAttrStrVar > 0) {
        // memory is variable length string
        // attribute is variable length string
        ret = H5Aread(attr_(), memType, static_cast<void*>(data.data()));
        if (ret < 0) {
          throw Exception("H5Aread of string failed (mem: vlen, attr: vlen).", ioda_Here());
        }
      } else {
        // memory is variable length string
        // attribute is fixed length string
        //
        // We need to return a char pointer (to a copy of the attribute value) in data.
        // First get the length of the string, and allocate on the heap a buffer to hold
        // the resulting string value. Then pass a pointer to that string data back in
        // the parameter "data". Note initialization of stringValue heap allocation to
        // all zeros (string terminator).
        size_t strLen = H5Tget_size(attrType);
        char * stringValue = new char[strLen + 1]{ '\0' };
        ret = H5Aread(attr_(), attrType, static_cast<void*>(stringValue));
        if (ret < 0) throw Exception("H5Aread fixed length string failed.", ioda_Here());

        // At this point we have stringValue which is a (char *) and the output data which is
        // a char span. In the case of strings, data will be 8 bytes long with the intention
        // of placing a (char *) pointing to the string value.
        size_t strPtrSize = sizeof(char *);
        gsl::span<char> strPtrSpan(reinterpret_cast<char*>(&stringValue), strPtrSize);
        for (size_t i = 0; i < strPtrSize; ++i) {
          data[i] = strPtrSpan[i];
        }
      }
    } else {
      if (isAttrStrVar > 0) {
        // memory is fixed length string
        // attribute is variable length string
        //
        // attribute read will return a char * pointing to the string value
        char * stringValue;
        ret = H5Aread(attr_(), attrType, static_cast<void *>(&stringValue));
        if (ret < 0) throw Exception("H5Aread variable length string failed.", ioda_Here());

        // At this point, we don't know the actual size of the string in the attribute
        // since it is recorded as a variable length string. To determine this, we can
        // use stringValue to initialize a std::string and then check that we have room
        // in the memory buffer.
        std::string attrStringValue(stringValue);
        if (attrStringValue.size() > data.size()) {
          throw Exception(
            "H5Aread of string: attr string length is greater than mem allocation).",
            ioda_Here());
        }

        // Copy string into memory buffer
        for (size_t i = 0; i < attrStringValue.size(); ++i) {
          data[i] = attrStringValue[i];
        }
      } else {
        // memory is fixed length string
        // attribute is fixed length string
        size_t strLen = H5Tget_size(attrType);
        if (strLen > data.size()) {
          throw Exception(
            "H5Aread of string: attr string length is greater than mem allocation).",
            ioda_Here());
        }

        ret = H5Aread(attr_(), attrType, static_cast<void*>(data.data()));
        if (ret < 0) {
          throw Exception("H5Aread of string failed (mem: flen, attr: flen).", ioda_Here());
        }
      }
    }
  } else if ((memTypeClass == H5T_STRING) || (attrTypeClass == H5T_STRING)) {
    // One of memory or attribute types is a string while the other is numeric.
    throw Exception("H5Aread: memory and attribute data types are inconsistent.", ioda_Here());
  } else {
    // Have numeric types for both memory and file
    ret = H5Aread(attr_(), memType, static_cast<void*>(data.data()));
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
