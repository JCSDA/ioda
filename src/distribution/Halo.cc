/*
 * (C) Copyright 2017-2019 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/distribution/Halo.h"

#include <algorithm>
#include <iostream>
#include <numeric>
#include <set>
#include <vector>

#include "oops/util/DateTime.h"
#include "oops/util/Logger.h"

#include "eckit/config/LocalConfiguration.h"
#include "eckit/exception/Exceptions.h"

namespace ioda {
// -----------------------------------------------------------------------------
/*!
 * \brief Halo selector
 *
 * \details This distribution puts observations within a halo on the same
 *          processor
 *
 *
 */
// -----------------------------------------------------------------------------
Halo::Halo(const eckit::mpi::Comm & Comm,
           const eckit::Configuration & config) :
          Distribution(Comm, config) {
  // extract center point from configuration
  // if no patch center defined, distribute centers equi-distant along the equator
  std::vector<double> centerd(2, 0.0);
  if (config.has("center")) {
    centerd = config.getDoubleVector("center", centerd);
  } else {
    centerd[0] = static_cast<double>(comm_.rank())*
                             (360.0/static_cast<double>(comm_.size()));
    centerd[1] = 0.0;
  }
  eckit::geometry::Point2 center(centerd[0], centerd[1]);
  center_ = center;

  // assign radius that is a sum of the patch and localization radii
  // (1) this radius is the radius of the "patch"
  //     if not specified, use radius big enough to encompass all obs. on Earth
  double radius = config.getDouble("radius", 50000000.0);

  // (2) add localization radius (i.e. the "halo" radius)
  double locRadius = config.getDouble("localization.lengthscale", 0.0);
  radius += locRadius;

  radius_ = radius;

  oops::Log::debug() << "Halo constructed: center: " << center_ << " radius: "
                     << radius_ << std::endl;
}

// -----------------------------------------------------------------------------
Halo::~Halo() {
  oops::Log::trace() << "Halo destructed" << std::endl;
}

// -----------------------------------------------------------------------------
void Halo::assignRecord(const std::size_t RecNum, const std::size_t LocNum,
                        const eckit::geometry::Point2 & point) {
    double dist = eckit::geometry::Sphere::distance(radius_earth_, center_, point);
    oops::Log::debug() << "Point: " << point << " distance to center: " << center_
          << " = " << dist << std::endl;
    if (dist <= radius_) {
      haloObsRecord_.insert(RecNum);
      haloObsLoc_[LocNum] = dist;
      haloLocVector_.push_back(LocNum);
    }
}

// -----------------------------------------------------------------------------
bool Halo::isMyRecord(std::size_t RecNum) const {
    return (haloObsRecord_.count(RecNum) > 0);
}

// -----------------------------------------------------------------------------
std::size_t Halo::computePatchLocs(const std::size_t nglocs) {
  // define some constants for this PE
  double inf = std::numeric_limits<double>::infinity();
  size_t myRank = comm_.rank();

  // make structures holding pairs of {distance,rank} for reduce operation later
  // initialize the local array to dist == inf
  std::vector<std::pair<double, int>> dist_and_lidx_loc(nglocs);
  std::vector<std::pair<double, int>> dist_and_lidx_glb(nglocs);
  for ( size_t jj = 0; jj < nglocs; ++jj ) {
    dist_and_lidx_loc[jj] = std::make_pair(inf, myRank);
  }

  // populate local obs (stored in haloObsLoc_) with actual distances
  for (auto i : haloObsLoc_) {
    dist_and_lidx_loc[i.first] = std::make_pair(i.second, myRank);
  }

  // use reduce operation to find PE rank with minimal distance
  comm_.allReduce(dist_and_lidx_loc, dist_and_lidx_glb, eckit::mpi::minloc());

  // if this PE has the minimum distance then this PE owns this ob. as patch
  for (auto i : haloObsLoc_) {
    if ( dist_and_lidx_glb[i.first].second == myRank ) {
      patchObsLoc_.insert(i.first);
    }
  }

  // convert storage from unodered sets to a bool vector
  for (size_t jj = 0; jj < haloLocVector_.size(); ++jj) {
    if ( patchObsLoc_.count(haloLocVector_[jj]) ) {
      patchObsBool_.push_back(true);
    } else {
      patchObsBool_.push_back(false);
    }
  }

  // now that we have patchObsBool_ computed we can free memory for temp objects
  size_t uol = patchObsLoc_.size();
  haloObsLoc_.clear();
  haloLocVector_.clear();
  patchObsLoc_.clear();

  return uol;
}

// -----------------------------------------------------------------------------
void Halo::patchObs(std::vector<bool> & patchObsVec) const {
  patchObsVec = patchObsBool_;
}

// -----------------------------------------------------------------------------
double Halo::dot_product(const std::vector<double> &v1, const std::vector<double> &v2)
                const {
  ASSERT(v1.size() == v2.size());
  double missingValue = util::missingValue(missingValue);
  size_t nvars = v1.size()/patchObsBool_.size();

  double zz = 0.0;
  for (size_t jj = 0; jj < v1.size() ; ++jj) {
    size_t iLoc = jj % nvars;
    if (v1[jj] != missingValue && v2[jj] != missingValue &&
        patchObsBool_[iLoc]) {
      zz += v1[jj] * v2[jj];
    }
  }

  this->sum(zz);
  return zz;
}

// -----------------------------------------------------------------------------
double Halo::dot_product(const std::vector<int> &v1, const std::vector<int> &v2) const {
  std::vector<double> v1d(v1.begin(), v1.end());
  std::vector<double> v2d(v2.begin(), v2.end());
  double rv = this->dot_product(v1d, v2d);
  return rv;
}

// -----------------------------------------------------------------------------
size_t Halo::globalNumNonMissingObs(const std::vector<double> &v) const {
  return globalNumNonMissingObsImpl(v);
}

size_t Halo::globalNumNonMissingObs(const std::vector<float> &v) const {
  return globalNumNonMissingObsImpl(v);
}

size_t Halo::globalNumNonMissingObs(const std::vector<int> &v) const {
  return globalNumNonMissingObsImpl(v);
}

size_t Halo::globalNumNonMissingObs(const std::vector<std::string> &v) const {
  return globalNumNonMissingObsImpl(v);
}

size_t Halo::globalNumNonMissingObs(const std::vector<util::DateTime> &v) const {
  return globalNumNonMissingObsImpl(v);
}

template <typename T>
size_t Halo::globalNumNonMissingObsImpl(const std::vector<T> &v) const {
  T missingValue = util::missingValue(missingValue);
  size_t nvars = v.size()/patchObsBool_.size();

  size_t nobs = 0;
  for (size_t jj = 0; jj < v.size(); ++jj) {
    size_t iLoc = jj % nvars;
    if (v[jj] != missingValue && patchObsBool_[iLoc]) ++nobs;
  }

  this->sum(nobs);
  return nobs;
}

// -----------------------------------------------------------------------------
void Halo::sum(double &x) const {
    comm_.allReduceInPlace(x, eckit::mpi::sum());
}

void Halo::sum(int &x) const {
    comm_.allReduceInPlace(x, eckit::mpi::sum());
}

void Halo::sum(size_t &x) const {
    comm_.allReduceInPlace(x, eckit::mpi::sum());
}

void Halo::sum(std::vector<double> &x) const {
  // replace items not owned by this PE by 0 before reduce to prevent double counting
  oops::Log::debug() << "size(x)=" << x.size() << " size(patchObsBool_)=" <<
                        patchObsBool_.size() << std::endl;

  for (size_t i = 0; i < x.size(); ++i) { if (patchObsBool_[i] == false) {x[i] = 0.0;} }
  comm_.allReduceInPlace(x.begin(), x.end(), eckit::mpi::sum());
}

void Halo::sum(std::vector<size_t> &x) const {
  for (size_t i = 0; i < x.size(); ++i) { if (patchObsBool_[i] == false) {x[i] = 0;} }
    comm_.allReduceInPlace(x.begin(), x.end(), eckit::mpi::sum());
}

void Halo::min(double &x) const {
    comm_.allReduceInPlace(x, eckit::mpi::min());
}

void Halo::min(float &x) const {
    comm_.allReduceInPlace(x, eckit::mpi::min());
}

void Halo::min(int &x) const {
    comm_.allReduceInPlace(x, eckit::mpi::min());
}

void Halo::max(double &x) const {
    comm_.allReduceInPlace(x, eckit::mpi::max());
}

void Halo::max(float &x) const {
    comm_.allReduceInPlace(x, eckit::mpi::max());
}

void Halo::max(int &x) const {
    comm_.allReduceInPlace(x, eckit::mpi::max());
}

// TODO(issue #45) implement methods below
void Halo::allGatherv(std::vector<size_t> &x) const {
  throw std::runtime_error("allGatherv not implemented for Halo distribution");
}

void Halo::allGatherv(std::vector<int> &x) const {
  throw std::runtime_error("allGatherv not implemented for Halo distribution");
}

void Halo::allGatherv(std::vector<float> &x) const {
  throw std::runtime_error("allGatherv not implemented for Halo distribution");
}

void Halo::allGatherv(std::vector<double> &x) const {
  throw std::runtime_error("allGatherv not implemented for Halo distribution");
}

void Halo::allGatherv(std::vector<util::DateTime> &x) const {
  throw std::runtime_error("allGatherv not implemented for Halo distribution");
}

void Halo::allGatherv(std::vector<std::string> &x) const {
  throw std::runtime_error("allGatherv not implemented for Halo distribution");
}

void Halo::exclusiveScan(size_t &x) const {
  throw std::runtime_error("exclusiveScan not implemented for Halo distribution");
}

// -----------------------------------------------------------------------------

}  // namespace ioda
