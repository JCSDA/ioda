#pragma once
/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \addtogroup ioda_cxx_variable
 *
 * @{
 * \file Selection.h
 * \brief Dataspace selections for reading and writing ioda::Variable data.
 */
#include <utility>
#include <vector>

#include "ioda/defs.h"

namespace ioda {
/// \brief Selection enum
/// \ingroup ioda_cxx_variable
enum class SelectionOperator { SET, OR, AND, XOR, NOT_B, NOT_A, APPEND, PREPEND };
/// \brief The "default" for the selection.
/// \ingroup ioda_cxx_variable
enum class SelectionState { ALL, NONE };

/// \brief A Selection represents the bounds of the data, in ioda or in userspace, that
///        you are reading or writing.
/// \ingroup ioda_cxx_variable
///
/// It is made of a series of SingleSelection objects.
/// Each of these objects represents a selection operation that filters the range that
/// came before.
///
/// \note In user-space, you need to specify the bounds of your multi-dimensional storage
///       container. Use the extent function to do this.
class Selection {
public:
  typedef std::vector<Dimensions_t> VecDimensions_t;
  /// \brief Represents a hyperslab or a series of points in a selection, coupled with
  /// a SelectionOperator "action".
  /// \see SelectionOperator
  /// \see Selection
  /// \details There are three types of selections that you can make.
  /// 1) A hyperslab selection, where you can define the bounds using
  ///    a start point, a span (count), stride, and block. For details
  ///    of what these all mean, see the HDF5 documentation.
  /// 2) Individual points.
  /// 3) Axis + indices along the axis. You select a fundamental axis
  ///    (e.g. location, channel), and then list the indices along
  ///    that axis that you want to select.
  /// \note SelectionOperator can be a bit troublesome for case #3.
  ///    If the result is not what you would naturally expect,
  ///    please read the code and file an issue.
  /// \note Case 3 might not be supported on the HDF5 backend. Requires HDF5 version >= 1.12.0.
  struct SingleSelection {
    SelectionOperator op_;
    // Selection type 1: hyperslab
    VecDimensions_t start_, count_, stride_, block_;
    // Selection type 2: individual points
    std::vector<VecDimensions_t> points_;
    // Selection type 3: axis + indices along axis
    size_t dimension_;
    VecDimensions_t dimension_indices_starts_, dimension_indices_counts_;
    // Constructors
    SingleSelection(SelectionOperator op, const VecDimensions_t& start,
                    const VecDimensions_t& count, const VecDimensions_t& stride = {},
                    const VecDimensions_t& block = {})
        : op_(op), start_(start), count_(count), stride_(stride), block_(block), dimension_(0) {}
    SingleSelection(SelectionOperator op, const std::vector<VecDimensions_t>& points)
        : op_(op), points_(points), dimension_(0) {}
    SingleSelection(SelectionOperator op, size_t dimension, const VecDimensions_t& indices_starts,
                    const VecDimensions_t& indices_counts = {})
        : op_(op),
          dimension_(dimension),
          dimension_indices_starts_(indices_starts),
          dimension_indices_counts_(indices_counts) {}
    SingleSelection() : op_(SelectionOperator::SET), dimension_(0) {}
  };

  SelectionState default_;
  std::vector<SingleSelection> actions_;
  /// The offset is a way to quickly shift the selection.
  VecDimensions_t offset_;
  /// The extent is the dimensions of the object that you are selecting from.
  VecDimensions_t extent_;

  Selection(const VecDimensions_t& extent = {}, SelectionState sel = SelectionState::ALL)
      : default_(sel), extent_(extent) {}
  ~Selection() {}
  /// Shift the selection by an offset.
  Selection& setOffset(const std::vector<Dimensions_t>& newOffset) {
    offset_ = newOffset;
    return *this;
  }
  /// Append a new selection
  Selection& select(const SingleSelection& s) {
    actions_.push_back(s);
    return *this;
  }
  /// Provide the dimensions of the object that you are selecting from.
  Selection& extent(const VecDimensions_t& sz) {
    extent_ = sz;
    return *this;
  }

  static IODA_DL const Selection all;
  static IODA_DL const Selection none;
};

}  // namespace ioda
