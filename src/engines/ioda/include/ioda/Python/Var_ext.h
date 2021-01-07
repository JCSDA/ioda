#pragma once
/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// \file Var_ext.h
/// \brief Python extensions to ioda::Variable.

#include <string>
#include <vector>

#include "ioda/Variables/Selection.h"

namespace ioda {
class Variable;
struct VariableCreationParameters;
namespace detail {
/** \brief Implements wrappers that isolate the read and write functions.
 **/
namespace python_bindings {

template <class C = Variable>
class VariableIsA {
  C* parent_;

public:
  VariableIsA(C* p) : parent_{p} {}
  template <class T>
  bool isA() const {
    return parent_->template isA<T>();
  }
};

template <class C = Variable>
class VariableReadVector {
  C* parent_;

public:
  VariableReadVector(C* p) : parent_{p} {}
  template <class T>
  std::vector<T> read(const Selection& mem_selection = Selection::all,
                      const Selection& file_selection = Selection::all) const {
    std::vector<T> vals;
    parent_->template read<T>(vals, mem_selection, file_selection);
    return vals;
  }
};

template <class C = Variable>
class VariableReadNPArray {
  C* parent_;

public:
  VariableReadNPArray(C* p) : parent_{p} {}
  template <class T>
  Eigen::Array<T, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> read(
    const Selection& mem_selection = Selection::all, const Selection& file_selection = Selection::all) const {
    Eigen::Array<T, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> vals;
#ifdef _MSC_FULL_VER
    parent_->readWithEigenRegular(vals, mem_selection, file_selection);
#else
    parent_->template readWithEigenRegular(vals, mem_selection, file_selection);
#endif
    return vals;
  }
};

template <class C = Variable>
class VariableWriteVector {
  C* parent_;

public:
  VariableWriteVector(C* p) : parent_{p} {}
  template <class T>
  void write(const std::vector<T>& vals, const Selection& mem_selection = Selection::all,
             const Selection& file_selection = Selection::all) const {
    parent_->template write<T>(vals, mem_selection, file_selection);
  }
};

template <class C = Variable>
class VariableWriteNPArray {
  C* parent_;

public:
  VariableWriteNPArray(C* p) : parent_{p} {}
  template <class T>
  void write(const Eigen::Array<T, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>& vals,
             const Selection& mem_selection = Selection::all,
             const Selection& file_selection = Selection::all) const {
#ifdef _MSC_FULL_VER
    parent_->writeWithEigenRegular(vals, mem_selection, file_selection);
#else
    parent_->template writeWithEigenRegular(vals, mem_selection, file_selection);
#endif
  }
};

template <class C = Variable>
class VariableScales {
  C* parent_;

public:
  VariableScales(C* p) : parent_{p} {}
  inline void attach(unsigned int DimensionNumber, const C& scale) {
    parent_->attachDimensionScale(DimensionNumber, scale);
  }
  inline void detach(unsigned int DimensionNumber, const C& scale) {
    parent_->detachDimensionScale(DimensionNumber, scale);
  }

  inline void set(const std::vector<C>& scales) { parent_->setDimScale(scales); }

  inline bool isScale() const { return parent_->isDimensionScale(); }
  inline void setIsScale(const std::string& name) { parent_->setIsDimensionScale(name); }
  inline std::string getScaleName() const { return parent_->getDimensionScaleName(); }

  inline bool isAttached(unsigned int DimensionNumber, const C& scale) const {
    return parent_->isDimensionScaleAttached(DimensionNumber, scale);
  }
};

template <class C = Variable>
class VariableCreationFillValues {
  C* parent_;

public:
  VariableCreationFillValues(C* p) : parent_{p} {}
  template <class T>
  void setFillValue(T fill) const {
    parent_->template setFillValue<T>(fill);
  }
};
}  // namespace python_bindings
}  // namespace detail
}  // namespace ioda
