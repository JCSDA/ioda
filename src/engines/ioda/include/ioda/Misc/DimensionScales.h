#pragma once
/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// \file DimensionScales.h
/// \brief Convenience classes for constructing ObsSpaces and setting up new Dimension Scales.
#include <memory>
#include <string>
#include <typeindex>
#include <vector>

#include "../defs.h"
#include "Dimensions.h"
#include "../Types/Type.h"
#include "../Variables/Has_Variables.h"
#include "../Variables/Variable.h"

namespace ioda {
class Type;
class Variable;
class Has_Variables;

/// Specifies that a dimension is resizable to infinity.
constexpr int Unlimited = -1;
/// Specifies that a dimension has no specified size. Tells ioda to figure it out from elsewhere.
constexpr int Unspecified = -2;

struct ScaleSizes {
  /// Initial size of the new dimension.
  Dimensions_t size_ = Unspecified;
  /// Maximum size of the new dimension.
  Dimensions_t maxSize_ = Unspecified;
  /// Chunking size of the new dimension.
  Dimensions_t chunkingSize_ = Unspecified;

  ScaleSizes(Dimensions_t size = Unspecified, Dimensions_t maxSize = Unspecified,
             Dimensions_t chunkingSize = Unspecified)
      : size_(size), maxSize_(maxSize), chunkingSize_(chunkingSize) {}
};

/// \brief Used to specify a new dimension scale when making an ObsSpace.
/// \note Instantiation stored in ObsSpace.cpp.
/// \note shared_from_this for pybind11. Can't pass in a list
///       of unique_ptrs.
struct IODA_DL NewDimensionScale_Base : std::enable_shared_from_this<NewDimensionScale_Base> {
  /// Name of the dimension. Scan position, scan line, latitude, ...
  std::string name_;
  /// Type of the new dimension. Int, char, etc. Used if a type is not passed directly.
  std::type_index dataType_;
  /// Type of the new dimension. Used if a type is passed directly.
  Type dataTypeKnown_;
  /// Initial size of the new dimension.
  Dimensions_t size_;
  /// Maximum size of the new dimension. Unlimited (< 0) by default.
  Dimensions_t maxSize_;
  /// \brief Chunking size of the new dimension. May be used as a
  ///   hint when creating new Variables based on this dimension.
  /// \details Matches size by default, but will throw an error if the size is zero.
  Dimensions_t chunkingSize_;

  /// \note Not pure virtual to avoid pybind11 headaches with adding a trampoline class.
  ///   The base class should never be invoked directly.
  /// \see https://pybind11.readthedocs.io/en/stable/advanced/classes.html
  virtual void writeInitialData(Variable&) const {}

  virtual ~NewDimensionScale_Base();

  /// \note This should not be used directly. Keeping it public because of cross-language bindings.
  /// \see NewDimensionScale
  NewDimensionScale_Base(const std::string& name, const std::type_index& dataType,
                         Dimensions_t size, Dimensions_t maxSize,
                         Dimensions_t chunkingSize)
      : name_(name),
        dataType_(dataType),
        size_(size),
        maxSize_(maxSize),
        chunkingSize_(chunkingSize) {}

  NewDimensionScale_Base(const std::string& name, const Type& dataType,
                         Dimensions_t size, Dimensions_t maxSize, Dimensions_t chunkingSize)
      : name_(name),
        dataTypeKnown_(dataType),
        dataType_(typeid(void)),
        size_(size),
        maxSize_(maxSize),
        chunkingSize_(chunkingSize) {}
};
typedef std::vector<std::shared_ptr<NewDimensionScale_Base>> NewDimensionScales_t;

/// \brief Used to specify a new dimension scale when making an ObsSpace.
/// Templated version of NewDimensionScale_Base.
template <class DataType>
struct NewDimensionScale_Object : public NewDimensionScale_Base {
  virtual ~NewDimensionScale_Object() {}

  std::vector<DataType> initdata_;

  NewDimensionScale_Object(const std::string& name, Dimensions_t size,
                    Dimensions_t maxSize, Dimensions_t chunkingSize)
      : NewDimensionScale_Base(name, typeid(DataType), size, maxSize, chunkingSize),
        initdata_(gsl::narrow<size_t>(size)) {
    for (size_t i = 0; i < initdata_.size(); ++i) initdata_[i] = gsl::narrow<DataType>(i + 1);
  }

  void writeInitialData(Variable& v) const override { v.write<DataType>(initdata_); }

  std::shared_ptr<NewDimensionScale_Object<DataType>> getShared() const {
    return std::make_shared<NewDimensionScale_Object<DataType>>(*this);
  }
};

/// @brief Wrapper function used when listing new dimension scales to construct.
/// @tparam DataType is the type of data used in the scale.
/// @param name is the new scale's name.
/// @param size is the initial size (in elements).
/// @param maxSize is the maximum size, in elements.
///   ioda::Unspecified sets the max dimension size to the initial size.
///   ioda::Unlimited specifies an unlimited dimension, which should be seldom used.
/// @param chunkingSize is a "hint" parameter that guides how data are grouped in memory. New
///   variables using this scale take this hint to specify their chunk sizes.
/// @return A shared_ptr to a NewDimensionScale_Object, which can be inserted into a
///   NewDimensionScales_t container.
template <class DataType>
inline std::shared_ptr<NewDimensionScale_Object<DataType>> NewDimensionScale(
  const std::string& name, Dimensions_t size, Dimensions_t maxSize = Unspecified,
  Dimensions_t chunkingSize = Unspecified) {
  if (maxSize == Unspecified) maxSize = size;
  if (chunkingSize == Unspecified) chunkingSize = maxSize;
  return std::make_shared<NewDimensionScale_Object<DataType>>(name, size, maxSize, chunkingSize);
}

template <class DataType>
inline std::shared_ptr<NewDimensionScale_Object<DataType>> NewDimensionScale(
  const std::string& name, ScaleSizes sizes) {
  return NewDimensionScale<DataType>(name, sizes.size_, sizes.maxSize_, sizes.chunkingSize_);
}

IODA_DL std::shared_ptr<NewDimensionScale_Base> NewDimensionScale(
  const std::string& name, const Type& t, Dimensions_t size, Dimensions_t maxSize = Unspecified,
  Dimensions_t chunkingSize = Unspecified);

IODA_DL std::shared_ptr<NewDimensionScale_Base> NewDimensionScale(
  const std::string& name, const Variable& scale, const ScaleSizes &overrides = ScaleSizes());

/// \brief Return the list of all variables among \p allVarNames that belong to \p hasVars and
/// are dimension scales.
//
// Partially copied from IodaUtils.cpp
IODA_DL std::list<ioda::Named_Variable> identifyDimensionScales(
  const detail::Has_Variables_Base &hasVars, std::vector<std::string> &allVarNames);

}  // namespace ioda
