/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \addtogroup ioda_internals_engines_obsstore
 *
 * @{
 * \file Selection.cpp
 * \brief Functions for ObsStore Selection
 */

#include "gsl/gsl-lite.hpp"

#include "./Selection.hpp"
#include "ioda/Exception.h"

namespace ioda {
namespace ObsStore {

//*************************************************************************
//           SelectCounter functions
//*************************************************************************
SelectCounter::SelectCounter() : icount_(0), ndigits_(0) {}

void SelectCounter::reset(const SelectionModes mode, const std::vector<std::size_t>& digit_sizes) {
  mode_        = mode;
  digit_sizes_ = digit_sizes;
  ndigits_     = digit_sizes.size();
  digits_.assign(digit_sizes.size(), 0);
  counter_end_ = false;
}

void SelectCounter::inc() {
  if (mode_ == SelectionModes::POINT) {
    if (digits_[0] == ndigits_ - 1) {
      // about to overflow the counter --> done
      counter_end_ = true;
      return;
    }

    // increment all digits: 0,0 1,1 2,2 etc.
    for (std::size_t i = 0; i < ndigits_; ++i) {
      digits_[i]++;
    }
  } else {
    // Increment the least significant digit
    icount_ = ndigits_ - 1;
    digits_[icount_]++;

    // Apply carry rules
    while (digits_[icount_] == digit_sizes_[icount_]) {
      if (icount_ == 0) {
        // About to overflow the counter --> done
        counter_end_ = true;
        return;
      }

      // Carry
      digits_[icount_] = 0;
      icount_--;
      digits_[icount_]++;
    }
  }
}

bool SelectCounter::finished() const { return counter_end_; }

const std::vector<std::size_t>& SelectCounter::count() const { return digits_; }

//*************************************************************************
//           Selection functions
//*************************************************************************
Selection::Selection(const std::size_t start, const std::size_t npoints)
    : mode_(SelectionModes::ALL),
      end_(gsl::narrow<int>(start + npoints) - 1),
      index_(0), npoints_(npoints) {
  max_index_ = end_;
}

Selection::Selection(const SelectionModes mode, const std::vector<SelectSpecs>& dim_selects,
                     const std::vector<Dimensions_t>& dim_sizes)
    : mode_(mode),
      end_(0),
      index_(0),
      npoints_(1),
      max_index_(1),
      dim_selects_(dim_selects),
      dim_sizes_(dim_sizes),
      // dim_select_sizes
      counter_(std::unique_ptr<SelectCounter>(new SelectCounter)) {
  // record the sizes of each dimension selection
  dim_select_sizes_.clear();
  for (std::size_t i = 0; i < dim_selects_.size(); ++i) {
    std::size_t dim_select_size = dim_selects_[i].size();
    dim_select_sizes_.push_back(dim_select_size);
    if ((i == 0) || (mode == SelectionModes::INTERSECT)) {
      npoints_ *= dim_select_size;
    }

    max_index_ *= dim_sizes[i];
  }
  // At this point, max_index_ equals the total number of points that can
  // be accessed by dimensions, so need to decrement by 1 for it to contain
  // the maximum allowed index.
  max_index_--;
}

Selection::Selection() = default;

SelectionModes Selection::mode() const { return mode_; }

// Following are a set of functions that provide an iterator style capability
// that generates the linear memory indices correspoding to the selection
// settings. The linear memory indices are generated by creating a counter
// with the same number of digits as the size of the dim_selects_ vector, and
// making the maximum values of each digit place match the size of the corresponding
// subvector in dim_selects_. This yields the same sequence as running through
// nested for loops that walk through the dim_selects_ structure.
void Selection::init_lin_indx() {
  if (mode_ == SelectionModes::ALL) {
    index_ = 0;
  } else {
    counter_->reset(mode_, dim_select_sizes_);
  }
}

std::size_t Selection::next_lin_indx() {
  std::size_t lin_index = 0;
  if (mode_ == SelectionModes::ALL) {
    lin_index = index_;
    index_++;
  } else {
    // Calculate linear index from current count
    const std::vector<std::size_t>& curCount = counter_->count();
    lin_index                                = dim_selects_[0][curCount[0]];
    for (std::size_t i = 1; i < dim_selects_.size(); ++i) {
      lin_index *= dim_sizes_[i];
      lin_index += dim_selects_[i][curCount[i]];
    }

    // increment counter for next time around
    counter_->inc();
  }

  // Make sure lin_index_ is in bounds.
  if (lin_index > max_index_)
    throw Exception("Next linear index is out of bounds.", ioda_Here())
      .add("  Next linear index: ", lin_index)
      .add("  Maximum allowed index: ", max_index_);

  return lin_index;
}

bool Selection::end_lin_indx() const {
  bool finished = false;
  ;
  if (mode_ == SelectionModes::ALL) {
    finished = (index_ > end_);
  } else {
    finished = counter_->finished();
  }
  return finished;
}

std::size_t Selection::npoints() const { return npoints_; }
}  // namespace ObsStore
}  // namespace ioda

/// @}
