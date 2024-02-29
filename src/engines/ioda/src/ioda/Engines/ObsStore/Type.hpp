/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \addtogroup ioda_internals_engines_obsstore
 *
 * @{
 * \file Type.hpp
 * \brief Functions for ObsStore Type
 */
#pragma once
#include <cstring>
#include <gsl/gsl-lite.hpp>
#include <memory>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <vector>

#include "../HH/HH/HH-types.h"

namespace ioda {
namespace ObsStore {
/// \brief ObsStore fundamental data type markers
/// \ingroup ioda_internals_engines_obsstore
/// \details ObsStore fundamental data type markers are one-for-one with C++ POD types.
///          These are needed for translating the frontend structure that holds
///          POD types to an equivalent in ObsStore. These are primarily used
///          for constructing templated objects that hold data values.
enum class ObsTypes {
  NOTYPE,

  FLOAT,
  DOUBLE,
  LDOUBLE,

  SCHAR,
  SHORT,
  INT,
  LONG,
  LLONG,

  UCHAR,
  UINT,
  USHORT,
  ULONG,
  ULLONG,

  CHAR,
  WCHAR,
  CHAR16,
  CHAR32,

  ARRAY,
  STRING
};

/// \brief ObsStore data type classes
/// \ingroup ioda_internals_engines_obsstore
/// \details ObsStore data type classes help with simplifying code in the data
/// marshalling step when translating between spans of the C++ data type and spans
/// of char.
enum class ObsTypeClasses {
  NOCLASS,

  INTEGER,
  FLOAT,
  STRING,

  BITFIELD,
  OPAQUE,
  COMPOUND,
  REFERENCE,
  ENUM,

  VLEN_ARRAY,
  FIXED_ARRAY
};

class Type {
 private:
  /// \brief dimensions (for arrayed type)
  std::vector<std::size_t> dims_;

  /// \brief ObsStore data type
  ObsTypes type_;

  /// \brief ObsStore class of primary data type
  ObsTypeClasses class_;

  /// \brief ObsStore base (fundamental) data type
  std::shared_ptr<Type> base_type_;

  /// \brief number of elements in this type
  /// \details If this is a fundamental type, then num_elements_ is set to 1.
  /// If this is an array type, then num_elements_ is set to the product of the
  /// dimension sizes.
  std::size_t num_elements_;

  /// \brief data type size, ie nubmer of bytes in one element
  std::size_t size_;

  /// \brief is data type signed (false, unless you have an explicitly signed data type)
  bool is_signed_;

 public:
  Type();

  /// \brief constructor for fundamental data type
  Type(const ObsTypes dataType, const ObsTypeClasses typeClass,
       const std::size_t typeSize, const bool isTypeSigned);

  /// \brief constructor for array, compound data type
  Type(const std::vector<std::size_t> & dims, const ObsTypes dataType,
       const ObsTypeClasses typeClass, std::shared_ptr<Type> baseType);

  ~Type() {}

  /// \brief return reference to dimensions
  const std::vector<std::size_t> & getDims() const { return dims_; }

  /// \brief return ObsStore data type
  ObsTypes getType() const { return type_; }

  /// \brief return ObsStore data type class
  ObsTypeClasses getClass() const { return class_; }

  /// \brief return ObsStore base data type
  std::shared_ptr<Type> getBaseType() const { return base_type_; }

  /// \brief return number of elements
  std::size_t getNumElements() const { return num_elements_; }

  /// \brief return size of element (number of bytes)
  std::size_t getSize() const { return size_; }

  /// \brief true if base data type is explicitly signed
  bool isTypeSigned() const { return is_signed_; }

  /// \brief Convert this data type to an equivalent HDF5 type.
  /// \details This is used to take advantage of HDF5's superior
  ///   type conversion functions.
  detail::Engines::HH::HH_Type getHDF5Type() const;

  /// \brief comparison operators
  bool operator==(const Type & rhs) const;
  bool operator!=(const Type & rhs) const;
};

}  // namespace ObsStore
}  // namespace ioda

/// @}
