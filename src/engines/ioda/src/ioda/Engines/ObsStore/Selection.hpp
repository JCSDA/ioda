/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \addtogroup ioda_internals_engines_obsstore
 *
 * @{
 * \file Selection.hpp
 * \brief Functions for ObsStore Selection
 */
#pragma once
#include <cstring>
#include <gsl/gsl-lite.hpp>
#include <memory>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <vector>

#include "ioda/defs.h"

namespace ioda {
namespace ObsStore {
/// \brief container of selection indices
/// \ingroup ioda_internals_engines_obsstore
typedef std::vector<std::size_t> SelectSpecs;

/// \brief ObsStore selection modes
/// \ingroup ioda_internals_engines_obsstore
/// \details Selection mode meanings
///     ALL - select all points
///     INTERSECT - select points in intersection of dim indices
///                 if dim indices are
///                     dim 0: 1, 7, 8
///                     dim 1: 2, 4, 10
///                  then select the 9 points:
///                     (1,2)
///                     (1,4)
///                     (1,10)
///                     (7,2)
///                     (7,4)
///                     (7,10)
///                     (8,2)
///                     (8,4)
///                     (8,10)
///     POINT - select points given by each entry in dim indices
///                 if dim indices are
///                     dim 0: 1, 7, 8
///                     dim 1: 2, 4, 10
///                  then select the 3 points:
///                     (1,2)
///                     (7,4)
///                     (8,10)
enum class SelectionModes { ALL, INTERSECT, POINT };

/// \ingroup ioda_internals_engines_obsstore
class SelectCounter {
private:
  /// \brief mode of selection
  SelectionModes mode_ = SelectionModes::ALL;

  /// \brief counter digits
  std::vector<std::size_t> digits_;
  /// \brief index to counter digit
  std::size_t icount_ = 0;
  /// \brief number of digits in counter
  std::size_t ndigits_ = 0;
  /// \brief size (range) for each digit place
  std::vector<std::size_t> digit_sizes_;
  /// \brief true when finished counting
  bool counter_end_ = false;

public:
  SelectCounter();

  /// \brief allocate n digits for counter and set count to zero
  /// \details Most significant digit is at the front (position 0) and
  ///          the least significant digit is at the back (position n-1).
  void reset(const SelectionModes mode, const std::vector<std::size_t>& digit_sizes_);
  /// \brief increment counter
  void inc();
  /// \brief returns true when inc attempts to go past final state of counter
  bool finished() const;

  /// \brief return current counter value
  const std::vector<std::size_t>& count() const;
};

/// \ingroup ioda_internals_engines_obsstore
class Selection {
private:
  /// \brief mode of selection (which impacts how linear memory is accessed)
  SelectionModes mode_ = SelectionModes::ALL;

  /// \brief end of selection for ALL mode
  int end_ = 0;
  /// \brief index value for ALL mode
  int index_ = 0;

  /// \brief total number of points in selection
  std::size_t npoints_ = 0;
  /// \brief maximum allowed index value
  std::size_t max_index_ = 0;

  /// \brief selection indices for each dimension
  std::vector<SelectSpecs> dim_selects_;
  /// \brief sizes of data dimensions (length is rank of dimensions)
  std::vector<Dimensions_t> dim_sizes_;
  /// \brief number of dimension selections per dimension
  std::vector<std::size_t> dim_select_sizes_;

  /// \brief counter for generating linear memory indices
  std::unique_ptr<SelectCounter> counter_;

public:
  Selection(const std::size_t start, const std::size_t npoints);
  Selection(const SelectionModes mode, const std::vector<SelectSpecs>& dim_selects,
            const std::vector<Dimensions_t>& dim_sizes);
  Selection();

  /// \brief returns selection mode
  SelectionModes mode() const;

  /// \brief initializes iterator for walking through linear memory indices
  void init_lin_indx();
  /// \brief returns next linear memory index
  std::size_t next_lin_indx();
  /// \brief returns true when at the end of the linear memory indices
  bool end_lin_indx() const;

  /// \brief returns number of points in selection
  std::size_t npoints() const;
};
}  // namespace ObsStore
}  // namespace ioda

/// @}
