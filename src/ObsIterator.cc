/*
 * (C) Copyright 2026-2026 UCAR.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "eckit/geometry/Point3.h"

#include "ioda/ObsIterator.h"
#include "ioda/ObsSpace.h"


namespace ioda {

// -----------------------------------------------------------------------------

ObsIterator::ObsIterator(const ObsSpace & obsSpace, size_t index)
    : obsSpace_(obsSpace),
      vcName_(obsSpace.verticalCoordinate()),
      obIndex_(index) {}

// -----------------------------------------------------------------------------

bool ObsIterator::operator==(const ObsIterator & other) const {
  return obIndex_ == other.obIndex_ && &obsSpace_ == &other.obsSpace_;
}

// -----------------------------------------------------------------------------

bool ObsIterator::operator!=(const ObsIterator & other) const {
  return !(*this == other);
}

// -----------------------------------------------------------------------------

eckit::geometry::Point3 ObsIterator::operator*() const {
  // TODO(Travis) I still wish there was a more efficient way of doing this
  if (!lats_) {
    lats_ = std::make_shared<std::vector<float>>();
    obsSpace_.get_db("MetaData", "latitude", *lats_);
  }
  if (!lons_) {
    lons_ = std::make_shared<std::vector<float>>();
    obsSpace_.get_db("MetaData", "longitude", *lons_);
  }
  double z = 0.0;
  if (vcName_) {
    if (!vcoord_) {
      vcoord_ = std::make_shared<std::vector<float>>();
      obsSpace_.get_db("MetaData", *vcName_, *vcoord_);
    }
    z = static_cast<double>((*vcoord_)[obIndex_]);
  }
  return eckit::geometry::Point3((*lons_)[obIndex_], (*lats_)[obIndex_], z);
}

// -----------------------------------------------------------------------------

ObsIterator& ObsIterator::operator++() {
  ++obIndex_;
  return *this;
}

// -----------------------------------------------------------------------------

void ObsIterator::print(std::ostream & os) const {
  os << "ObsIterator: index=" << obIndex_;
}

// -----------------------------------------------------------------------------

}  // namespace ioda
