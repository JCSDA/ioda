#pragma once
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
 * \file HH-Filters.h
 * \brief HDF5 filters
 */

#include <list>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "./Handles.h"
#include "ioda/defs.h"

namespace ioda {
namespace detail {
namespace Engines {
namespace HH {
/// @brief Check that the given filter is available for encoding/decoding in the HDF5 pipeline.
/// @ingroup ioda_internals_engines_hh
/// @param filt is the filter in question. Commonly H5Z_FILTER_SZIP.
/// @return A pair of (canEncode, canDecode).
IODA_HIDDEN std::pair<bool, bool> isFilteravailable(H5Z_filter_t filt);

/// @brief Can SZIP encoding be used for a datatype?
/// @ingroup ioda_internals_engines_hh
/// @detail This depends on the data type and the HDF5 build options.
///   SZIP cannot be applied to compound, array, variable-length,
///   enumerative or user-defined datatypes.
/// @param dtype is the data type.
/// @return true if SZIP can be used for compression, false otherwise.
IODA_HIDDEN bool CanUseSZIP(HH_hid_t dtype);

/** \brief Order-obeying filter insertions and replacements
\ingroup ioda_internals_engines_hh
\details
Filters will be repeatedly removed and reinserted to get the desired filter order.
The desired filter order is:
- Shuffling
- Compression
Life would be easier if HDF5 allowed for an easy way to insert filters at specified orderings.
**/
struct IODA_HIDDEN Filters {
private:
  HH_hid_t pl;

public:
  Filters(HH_hid_t newbase);
  virtual ~Filters();
  /// \see https://support.hdfgroup.org/HDF5/doc/RM/RM_H5P.html#Property-SetFilter for meanings
  struct filter_info {
    H5Z_filter_t id    = -1;
    unsigned int flags = 0;
    std::vector<unsigned int> cd_values;
  };
  /// Get a vector of the filters that are implemented
  std::vector<filter_info> get() const;
  /// Append the filters to a property list.
  void append(const std::vector<filter_info>& filters);
  /// Set the filters to a property list. Clears existing filters.
  void set(const std::vector<filter_info>& filters);
  void clear();

  enum class FILTER_T { SHUFFLE, COMPRESSION, SCALE, OTHER };

  bool has(H5Z_filter_t id) const;

  static FILTER_T getType(const filter_info& it);
  static bool isA(const filter_info& it, FILTER_T typ);
  void appendOfType(const std::vector<filter_info>& filters, FILTER_T typ);
  void removeOfType(FILTER_T typ);

  void setShuffle();
  void setSZIP(unsigned int optm, unsigned int ppb);
  void setGZIP(unsigned int level);
  void setScaleOffset(H5Z_SO_scale_type_t scale_type, int scale_factor);
};
}  // namespace HH
}  // namespace Engines
}  // namespace detail
}  // namespace ioda

/// @}
