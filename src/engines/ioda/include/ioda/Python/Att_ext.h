#pragma once
/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \defgroup ioda_cxx_py C++ / Python Bindings
 * \brief Python binding helper classes, functions, and templates.
 * \ingroup ioda_internals
 *
 * @{
 * \file Att_ext.h
 * \brief Python extensions to ioda::Attribute.
 */

#include <vector>

#include "ioda/Variables/Selection.h"

namespace ioda {
class Attribute;
namespace detail {
/** \brief Implements wrappers that isolate the read and write functions.
 **/
namespace python_bindings {

/// \ingroup ioda_cxx_attribute_py
template <class C = Attribute>
class AttributeIsA {
  C* parent_;

public:
  AttributeIsA(C* p) : parent_{p} {}
  template <class T>
  bool isA() const {
    return parent_->template isA<T>();
  }
};

/// \ingroup ioda_cxx_attribute_py
template <class C = Attribute>
class AttributeReadSingle {
  C* parent_;

public:
  AttributeReadSingle(C* p) : parent_{p} {}
  template <class T>
  T read() const {
    return parent_->template read<T>();
  }
};

/// \ingroup ioda_cxx_attribute_py
template <class C = Attribute>
class AttributeReadVector {
  C* parent_;

public:
  AttributeReadVector(C* p) : parent_{p} {}
  template <class T>
  std::vector<T> read() const {
    std::vector<T> vals;
    parent_->template read<T>(vals);
    return vals;
  }
};

/// \ingroup ioda_cxx_attribute_py
template <class C = Attribute>
class AttributeReadNPArray {
  C* parent_;

public:
  AttributeReadNPArray(C* p) : parent_{p} {}
  template <class T>
  Eigen::Array<T, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> read() const {
    Eigen::Array<T, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> vals;
#ifdef _MSC_FULL_VER
    parent_->readWithEigenRegular(vals);
#else
    parent_->template readWithEigenRegular(vals);
#endif
    return vals;
  }
};

/// \ingroup ioda_cxx_attribute_py
template <class C = Attribute>
class AttributeWriteSingle {
  C* parent_;

public:
  AttributeWriteSingle(C* p) : parent_{p} {}
  template <class T>
  void write(T data) const {
    parent_->template write<T>(data);
  }
};

/// \ingroup ioda_cxx_attribute_py
template <class C = Attribute>
class AttributeWriteVector {
  C* parent_;

public:
  AttributeWriteVector(C* p) : parent_{p} {}
  template <class T>
  void write(const std::vector<T>& vals) const {
    parent_->template write<T>(vals);
  }
};

/// \ingroup ioda_cxx_attribute_py
template <class C = Attribute>
class AttributeWriteNPArray {
  C* parent_;

public:
  AttributeWriteNPArray(C* p) : parent_{p} {}
  template <class T>
  void write(const Eigen::Array<T, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>& vals) const {
#ifdef _MSC_FULL_VER
    parent_->writeWithEigenRegular(vals);
#else
    parent_->template writeWithEigenRegular(vals);
#endif
  }
};

}  // namespace python_bindings
}  // namespace detail
}  // namespace ioda

/// @}
