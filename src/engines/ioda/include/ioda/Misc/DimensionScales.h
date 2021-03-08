#pragma once
/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// \file DimensionScales.h
/// \brief Convenience classes for constructing ObsSpaces and setting up new Dimension Scales.
#include <memory>
#include <string>
#include <vector>

#include "../defs.h"
#include "Dimensions.h"

namespace ioda {
/// \deprecated Draft idea. No longer needed.
/// \brief Makes it easy for the user to keep track of dimensions.
enum class NewDimensionScale_Type {
  /// Generic location numbering
  Location,
  /// Ex: Scan position, scan line, latitude, longitude, etc.
  Horizontal,
  /// Ex: Level, layer, pressure, altitude
  Vertical,
  /// Ex: Time step
  Temporal,
  /// Ex: Channel number
  Other
};

/// Specifies that a dimension is resizable to infinity.
constexpr int Unlimited = -1;
/// Specifies that a dimension has no specified size. Tells ioda to figure it out from elsewhere.
constexpr int Unspecified = -2;

/// \brief Used to specify a new dimension scale when making an ObsSpace.
/// \note Instantiation stored in ObsSpace.cpp.
/// \note shared_from_this for pybind11. Can't pass in a list
///       of unique_ptrs.
struct IODA_DL NewDimensionScale_Base : std::enable_shared_from_this<NewDimensionScale_Base> {
  /// Name of the dimension. Scan position, scan line, latitude, ...
  std::string name_;
  /// Type of the new dimension. Int, char, etc.
  std::type_index dataType_;
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
                         Dimensions_t size = 0, Dimensions_t maxSize = Unlimited,
                         Dimensions_t chunkingSize = Unspecified)
      : name_(name),
        dataType_(dataType),
        size_(size),
        maxSize_(maxSize),
        chunkingSize_(chunkingSize) {}
};
typedef std::vector<std::shared_ptr<NewDimensionScale_Base>> NewDimensionScales_t;

/// \brief Used to specify a new dimension scale when making an ObsSpace.
/// Templated version of NewDimensionScale_Base.
template <class DataType>
struct NewDimensionScale : public NewDimensionScale_Base {
  virtual ~NewDimensionScale() {}

  std::vector<DataType> initdata_;

  NewDimensionScale(const std::string& name, Dimensions_t size = 0,
                    Dimensions_t maxSize = Unlimited, Dimensions_t chunkingSize = Unspecified)
      : NewDimensionScale_Base(name, typeid(DataType), size, maxSize, chunkingSize),
        initdata_(gsl::narrow<size_t>(size)) {
    for (size_t i = 0; i < initdata_.size(); ++i) initdata_[i] = gsl::narrow<DataType>(i + 1);
  }

  void writeInitialData(Variable& v) const override { v.write<DataType>(initdata_); }

  std::shared_ptr<NewDimensionScale<DataType>> getShared() const {
    return std::make_shared<NewDimensionScale<DataType>>(*this);
  }
};
}  // namespace ioda
