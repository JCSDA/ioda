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

#include <boost/make_unique.hpp>

#include "oops/mpi/mpi.h"
#include "oops/util/DateTime.h"
#include "oops/util/Logger.h"

#include "ioda/distribution/DistributionFactory.h"
#include "ioda/distribution/GeneralDistributionAccumulator.h"
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
          Distribution(Comm) {
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
  double locRadius = config.getDouble("obs localization.lengthscale", 0.0);
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
  if (recordsOutsideHalo_.find(RecNum) != recordsOutsideHalo_.end()) {
    // We've already seen the first location in this record, and it was too far away from center_.
    return;
  }

  if (recordsInHalo_.find(RecNum) == recordsInHalo_.end()) {
    // This is the first location from this record. Find out whether to assign it to this PE.

    const double dist = eckit::geometry::Sphere::distance(radius_earth_, center_, point);
    oops::Log::debug() << "Point: " << point << " distance to center: " << center_
          << " = " << dist << std::endl;
    if (dist <= radius_) {
      // Yes!
      recordsInHalo_.insert(RecNum);
      recordDistancesFromCenter_[RecNum] = dist;
    } else {
      // No, it's too far from center_.
      recordsOutsideHalo_.insert(RecNum);
      return;
    }
  }

  // Now we know this record has been assigned to this PE. Store information about location LocNum.
  haloLocVector_.push_back(LocNum);
  haloLocRecords_.push_back(RecNum);
}

// -----------------------------------------------------------------------------
bool Halo::isMyRecord(std::size_t RecNum) const {
    return (recordsInHalo_.count(RecNum) > 0);
}

// -----------------------------------------------------------------------------
void Halo::computePatchLocs(const std::size_t nglocs) {
  // define some constants for this PE
  double inf = std::numeric_limits<double>::infinity();
  size_t myRank = comm_.rank();

  // All records have now been assigned, so this container is no longer needed.
  recordsOutsideHalo_.clear();

  if ( nglocs > 0 ) {
    // make structures holding pairs of {distance,rank} for reduce operation later
    // initialize the local array to dist == inf
    std::vector<std::pair<double, int>> dist_and_lidx_loc(nglocs);
    std::vector<std::pair<double, int>> dist_and_lidx_glb(nglocs);
    for ( size_t jj = 0; jj < nglocs; ++jj ) {
      dist_and_lidx_loc[jj] = std::make_pair(inf, myRank);
    }

    // assign actual distances to local obs (stored in haloLocVector_)
    for (size_t loc = 0; loc < haloLocVector_.size(); ++loc) {
      const size_t gloc = haloLocVector_[loc];
      const size_t recNum = haloLocRecords_[loc];
      const double dist = recordDistancesFromCenter_.at(recNum);
      dist_and_lidx_loc[gloc] = std::make_pair(dist, myRank);
    }

    // use reduce operation to find PE rank with minimal distance
    comm_.allReduce(dist_and_lidx_loc, dist_and_lidx_glb, eckit::mpi::minloc());

    // ids of patch observations owned by this PE
    std::unordered_set<std::size_t> patchObsLoc;

    // if this PE has the minimum distance then this PE owns this ob. as patch
    for (auto i : haloLocVector_) {
      if ( dist_and_lidx_glb[i].second == myRank ) {
        patchObsLoc.insert(i);
      }
    }

    // convert storage from unodered sets to a bool vector
    for (size_t jj = 0; jj < haloLocVector_.size(); ++jj) {
      if ( patchObsLoc.count(haloLocVector_[jj]) ) {
        patchObsBool_.push_back(true);
      } else {
        patchObsBool_.push_back(false);
      }
    }

    // now that we have patchObsBool_ computed we can free memory occupied by some temp objects
    recordDistancesFromCenter_.clear();
    haloLocRecords_.clear();
    haloLocRecords_.shrink_to_fit();

    computeGlobalUniqueConsecutiveLocIndices(dist_and_lidx_glb);

    // and now the remaining temp object
    haloLocVector_.clear();
    haloLocVector_.shrink_to_fit();
  }
}

// -----------------------------------------------------------------------------
void Halo::computeGlobalUniqueConsecutiveLocIndices(
    const std::vector<std::pair<double, int>> &dist_and_lidx_glb) {
  globalUniqueConsecutiveLocIndices_.reserve(haloLocVector_.size());

  // Step 0: enable quick checks of whether a location belongs to this rank's halo.
  std::unordered_set<size_t> haloLocSet(haloLocVector_.begin(), haloLocVector_.end());

  // Step 1: index patch observations owned by each rank consecutively (starting from 0 on each
  // rank). For each observation i held on this rank, set globalConsecutiveLocIndices_[i] to
  // the index of the corresponding patch observation on the rank that owns it.
  std::vector<size_t> patchObsCountOnRank(comm_.size(), 0);
  for (size_t gloc = 0, nglocs = dist_and_lidx_glb.size(); gloc < nglocs; ++gloc) {
    const size_t rankOwningPatchObs = dist_and_lidx_glb[gloc].second;
    if (haloLocSet.find(gloc) != haloLocSet.end()) {
      // This obs is held on the current PE (but not necessarily as a patch obs)
      globalUniqueConsecutiveLocIndices_.push_back(patchObsCountOnRank[rankOwningPatchObs]);
    }
    ++patchObsCountOnRank[rankOwningPatchObs];
  }

  // Step 2: make indices of patch observations globally unique by incrementing the index
  // of each patch observation held by rank r by the total number of patch observations owned
  // by ranks r' < r.

  // Perform an exclusive scan
  std::vector<size_t> patchObsCountOnPreviousRanks(patchObsCountOnRank.size(), 0);
  for (size_t rank = 1; rank < patchObsCountOnRank.size(); ++rank) {
    patchObsCountOnPreviousRanks[rank] =
        patchObsCountOnPreviousRanks[rank - 1] + patchObsCountOnRank[rank - 1];
  }

  // Increment patch observation indices
  for (size_t loc = 0; loc < globalUniqueConsecutiveLocIndices_.size(); ++loc) {
    const size_t gloc = haloLocVector_[loc];
    const size_t rankOwningPatchObs = dist_and_lidx_glb[gloc].second;
    globalUniqueConsecutiveLocIndices_[loc] += patchObsCountOnPreviousRanks[rankOwningPatchObs];
  }
}

// -----------------------------------------------------------------------------
void Halo::patchObs(std::vector<bool> & patchObsVec) const {
  patchObsVec = patchObsBool_;
}

// -----------------------------------------------------------------------------
void Halo::min(int & x) const {
  minImpl(x);
}

void Halo::min(std::size_t & x) const {
  minImpl(x);
}

void Halo::min(float & x) const {
  minImpl(x);
}

void Halo::min(double & x) const {
  minImpl(x);
}

void Halo::min(std::vector<int> & x) const {
  minImpl(x);
}

void Halo::min(std::vector<std::size_t> & x) const {
  minImpl(x);
}

void Halo::min(std::vector<float> & x) const {
  minImpl(x);
}

void Halo::min(std::vector<double> & x) const {
  minImpl(x);
}

template <typename T>
void Halo::minImpl(T & x) const {
  reductionImpl(x, eckit::mpi::min());
}

// -----------------------------------------------------------------------------
void Halo::max(int & x) const {
  maxImpl(x);
}

void Halo::max(std::size_t & x) const {
  maxImpl(x);
}

void Halo::max(float & x) const {
  maxImpl(x);
}

void Halo::max(double & x) const {
  maxImpl(x);
}

void Halo::max(std::vector<int> & x) const {
  maxImpl(x);
}

void Halo::max(std::vector<std::size_t> & x) const {
  maxImpl(x);
}

void Halo::max(std::vector<float> & x) const {
  maxImpl(x);
}

void Halo::max(std::vector<double> & x) const {
  maxImpl(x);
}

template <typename T>
void Halo::maxImpl(T & x) const {
  reductionImpl(x, eckit::mpi::max());
}

// -----------------------------------------------------------------------------
template <typename T>
void Halo::reductionImpl(T & x, eckit::mpi::Operation::Code op) const {
  comm_.allReduceInPlace(x, op);
}

template <typename T>
void Halo::reductionImpl(std::vector<T> & x, eckit::mpi::Operation::Code op) const {
  comm_.allReduceInPlace(x.begin(), x.end(), op);
}

// -----------------------------------------------------------------------------
std::unique_ptr<Accumulator<int>>
Halo::createAccumulatorImpl(int init) const {
  return createAccumulatorImplT(init);
}

std::unique_ptr<Accumulator<std::size_t>>
Halo::createAccumulatorImpl(std::size_t init) const {
  return createAccumulatorImplT(init);
}

std::unique_ptr<Accumulator<float>>
Halo::createAccumulatorImpl(float init) const {
  return createAccumulatorImplT(init);
}

std::unique_ptr<Accumulator<double>>
Halo::createAccumulatorImpl(double init) const {
  return createAccumulatorImplT(init);
}

std::unique_ptr<Accumulator<std::vector<int>>>
Halo::createAccumulatorImpl(const std::vector<int> &init) const {
  return createAccumulatorImplT(init);
}

std::unique_ptr<Accumulator<std::vector<std::size_t>>>
Halo::createAccumulatorImpl(const std::vector<std::size_t> &init) const {
  return createAccumulatorImplT(init);
}

std::unique_ptr<Accumulator<std::vector<float>>>
Halo::createAccumulatorImpl(const std::vector<float> &init) const {
  return createAccumulatorImplT(init);
}

std::unique_ptr<Accumulator<std::vector<double>>>
Halo::createAccumulatorImpl(const std::vector<double> &init) const {
  return createAccumulatorImplT(init);
}

template <typename T>
std::unique_ptr<Accumulator<T>>
Halo::createAccumulatorImplT(const T &init) const {
  return boost::make_unique<GeneralDistributionAccumulator<T>>(init, comm_, patchObsBool_);
}

// -----------------------------------------------------------------------------
void Halo::allGatherv(std::vector<size_t> &x) const {
  allGathervImpl(x);
}

void Halo::allGatherv(std::vector<int> &x) const {
  allGathervImpl(x);
}

void Halo::allGatherv(std::vector<float> &x) const {
  allGathervImpl(x);
}

void Halo::allGatherv(std::vector<double> &x) const {
  allGathervImpl(x);
}

/// overloading for util::DateTime
void Halo::allGatherv(std::vector<util::DateTime> &x) const {
  allGathervImpl(x);
}

/// overloading for std::string
void Halo::allGatherv(std::vector<std::string> &x) const {
  allGathervImpl(x);
}

template <typename T>
void Halo::allGathervImpl(std::vector<T> &x) const {
  // operation is only well defined if size x is the size of local obs
  ASSERT(x.size() == patchObsBool_.size());

  // make a temp vector that only holds patch obs.
  std::vector<T> xtmp(std::count(patchObsBool_.begin(), patchObsBool_.end(), true));
  size_t counter = 0;
  for (size_t ii = 0; ii < x.size(); ++ii) {
    if ( patchObsBool_[ii] ) {
      xtmp[counter] = x[ii];
      ++counter;
    }
  }
  // gather all patch obs into a single vector
  oops::mpi::allGatherv(comm_, xtmp);

  // return the result
  x = xtmp;
}

// -----------------------------------------------------------------------------

size_t Halo::globalUniqueConsecutiveLocationIndex(size_t loc) const {
  return globalUniqueConsecutiveLocIndices_[loc];
}

// -----------------------------------------------------------------------------

}  // namespace ioda
