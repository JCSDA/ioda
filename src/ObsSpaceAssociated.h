/*
 * (C) Copyright 2026 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef OBSSPACEASSOCIATED_H_
#define OBSSPACEASSOCIATED_H_

#include <vector>

namespace ioda {

/// \brief Base class for data structures associated with ObsSpace.
///
/// \details Any associated data structures change their state (e.g. reduce or append)
/// when ObsSpace changes its state. ObsSpaceAssociated is equivalent to Observer
/// in the Observer pattern (don't confuse with oops::Observer!)
class ObsSpaceAssociated {
 public:
  virtual ~ObsSpaceAssociated() = default;
  virtual void reduce(const std::vector<bool> & keepLocs) = 0;
  virtual void append() = 0;
  /// \brief Sync internal append bookkeeping with the current nlocs (default no-op).
  virtual void syncAppend() {}
};

}  // namespace ioda

#endif  // OBSSPACEASSOCIATED_H_
