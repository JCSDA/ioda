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

#include "ioda/distribution/DistributionFactory.h"
#include "eckit/config/LocalConfiguration.h"
#include "eckit/exception/Exceptions.h"

namespace ioda {

// -----------------------------------------------------------------------------
static DistributionMaker<Halo> maker("Halo");

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
  return dot_productImpl(v1, v2);
}

// -----------------------------------------------------------------------------
double Halo::dot_product(const std::vector<float> &v1, const std::vector<float> &v2)
                const {
  return dot_productImpl(v1, v2);
}

// -----------------------------------------------------------------------------
double Halo::dot_product(const std::vector<int> &v1, const std::vector<int> &v2) const {
  return dot_productImpl(v1, v2);
}

template <typename T>
double Halo::dot_productImpl(const std::vector<T> &v1, const std::vector<T> &v2) const {
  ASSERT(v1.size() == v2.size());
  T missingValue = util::missingValue(missingValue);

  double zz = 0.0;
  if (patchObsBool_.size() > 0) {
    size_t nvars = v1.size()/patchObsBool_.size();
    for (size_t jj = 0; jj < v1.size() ; ++jj) {
      size_t iLoc = jj / nvars;
      if (v1[jj] != missingValue && v2[jj] != missingValue &&
          patchObsBool_[iLoc]) {
        zz += v1[jj] * v2[jj];
      }
    }
  }

  comm_.allReduceInPlace(zz, eckit::mpi::sum());
  return zz;
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

  size_t nobs = 0;
  if (patchObsBool_.size() > 0) {
    size_t nvars = v.size()/patchObsBool_.size();
    for (size_t jj = 0; jj < v.size(); ++jj) {
      size_t iLoc = jj / nvars;
      if (v[jj] != missingValue && patchObsBool_[iLoc]) ++nobs;
    }
  }

  comm_.allReduceInPlace(nobs, eckit::mpi::sum());
  return nobs;
}

// -----------------------------------------------------------------------------
void Halo::allReduceInPlace(double &x, eckit::mpi::Operation::Code op) const {
  allReduceInPlaceImpl(x, op);
}

void Halo::allReduceInPlace(float &x, eckit::mpi::Operation::Code op) const {
  allReduceInPlaceImpl(x, op);
}

void Halo::allReduceInPlace(int &x, eckit::mpi::Operation::Code op) const {
  allReduceInPlaceImpl(x, op);
}

void Halo::allReduceInPlace(size_t &x, eckit::mpi::Operation::Code op) const {
  allReduceInPlaceImpl(x, op);
}

void Halo::allReduceInPlace(std::vector<double> &x, eckit::mpi::Operation::Code op) const {
  allReduceInPlaceImpl(x, op);
}


void Halo::allReduceInPlace(std::vector<float> &x, eckit::mpi::Operation::Code op) const {
  allReduceInPlaceImpl(x, op);
}

void Halo::allReduceInPlace(std::vector<size_t> &x, eckit::mpi::Operation::Code op) const {
  allReduceInPlaceImpl(x, op);
}

template<typename T>
void Halo::allReduceInPlaceImpl(T &x, eckit::mpi::Operation::Code op) const {
  // reduce is well defined only for min & max for Halo distribution
  if ((op == eckit::mpi::min()) || (op == eckit::mpi::max())) {
    comm_.allReduceInPlace(x, op);
  } else {
    throw eckit::NotImplemented("Reduce operation is not defined for Halo distribution", Here());
  }
}

template<typename T>
void Halo::allReduceInPlaceImpl(std::vector<T> &x, eckit::mpi::Operation::Code op) const {
  if ((op == eckit::mpi::min()) || (op == eckit::mpi::max())) {
    comm_.allReduceInPlace(x.begin(), x.end(), op);
  } else if (op == eckit::mpi::sum()) {
    // reduce for vector is well defined only when values passed in \p x are corresponding
    // to global locations. the below check for sizes is not safe if size(\p x) is only
    // coincidentally same as size(patchObsBool_)
    ASSERT(x.size() == patchObsBool_.size());
    for (size_t i = 0; i < x.size(); ++i) {
      if (!patchObsBool_[i])    x[i] = 0.0;
    }
    comm_.allReduceInPlace(x.begin(), x.end(), op);
  } else {
    throw eckit::NotImplemented(std::to_string(op) + " reduce operation is not defined " +
                                "for Halo distribution", Here());
  }
}

// -----------------------------------------------------------------------------
// TODO(issue #45) implement methods below
void Halo::allGatherv(std::vector<size_t> &x) const {
  throw eckit::NotImplemented("allGatherv not implemented for Halo distribution", Here());
}

void Halo::allGatherv(std::vector<int> &x) const {
  throw eckit::NotImplemented("allGatherv not implemented for Halo distribution", Here());
}

void Halo::allGatherv(std::vector<float> &x) const {
  throw eckit::NotImplemented("allGatherv not implemented for Halo distribution", Here());
}

void Halo::allGatherv(std::vector<double> &x) const {
  throw eckit::NotImplemented("allGatherv not implemented for Halo distribution", Here());
}

void Halo::allGatherv(std::vector<util::DateTime> &x) const {
  throw eckit::NotImplemented("allGatherv not implemented for Halo distribution", Here());
}

void Halo::allGatherv(std::vector<std::string> &x) const {
  throw eckit::NotImplemented("allGatherv not implemented for Halo distribution", Here());
}

void Halo::exclusiveScan(size_t &x) const {
  throw eckit::NotImplemented("exclusiveScan not implemented for Halo distribution", Here());
}

// -----------------------------------------------------------------------------

}  // namespace ioda
