/*
 * (C) Copyright 2017-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/distribution/NonoverlappingDistribution.h"

#include <algorithm>
#include <iostream>
#include <numeric>
#include <set>

#include <boost/make_unique.hpp>

#include "ioda/distribution/DistributionFactory.h"
#include "ioda/distribution/NonoverlappingDistributionAccumulator.h"
#include "oops/mpi/mpi.h"
#include "oops/util/DateTime.h"
#include "oops/util/Logger.h"

namespace ioda {

// -----------------------------------------------------------------------------
NonoverlappingDistribution::NonoverlappingDistribution(const eckit::mpi::Comm & Comm)
  : Distribution(Comm) {
    oops::Log::trace() << "NonoverlappingDistribution constructed" << std::endl;
}

// -----------------------------------------------------------------------------
NonoverlappingDistribution::~NonoverlappingDistribution() {
    oops::Log::trace() << "NonoverlappingDistribution destructed" << std::endl;
}

// -----------------------------------------------------------------------------
void NonoverlappingDistribution::assignRecord(const std::size_t RecNum,
                                              const std::size_t /*LocNum*/,
                                              const eckit::geometry::Point2 & /*point*/) {
  if (isMyRecord(RecNum))
    ++numLocationsOnThisRank_;
}

// -----------------------------------------------------------------------------
void NonoverlappingDistribution::computePatchLocs(const std::size_t /*nglocs*/) {
  numLocationsOnLowerRanks_ = numLocationsOnThisRank_;
  oops::mpi::exclusiveScan(comm_, numLocationsOnLowerRanks_);
}

// -----------------------------------------------------------------------------
void NonoverlappingDistribution::patchObs(std::vector<bool> & patchObsVec) const {
  std::fill(patchObsVec.begin(), patchObsVec.end(), true);
}

// -----------------------------------------------------------------------------
void NonoverlappingDistribution::min(int & x) const {
  minImpl(x);
}

void NonoverlappingDistribution::min(std::size_t & x) const {
  minImpl(x);
}

void NonoverlappingDistribution::min(float & x) const {
  minImpl(x);
}

void NonoverlappingDistribution::min(double & x) const {
  minImpl(x);
}

void NonoverlappingDistribution::min(std::vector<int> & x) const {
  minImpl(x);
}

void NonoverlappingDistribution::min(std::vector<std::size_t> & x) const {
  minImpl(x);
}

void NonoverlappingDistribution::min(std::vector<float> & x) const {
  minImpl(x);
}

void NonoverlappingDistribution::min(std::vector<double> & x) const {
  minImpl(x);
}

template <typename T>
void NonoverlappingDistribution::minImpl(T & x) const {
  reductionImpl(x, eckit::mpi::min());
}

// -----------------------------------------------------------------------------
void NonoverlappingDistribution::max(int & x) const {
  maxImpl(x);
}

void NonoverlappingDistribution::max(std::size_t & x) const {
  maxImpl(x);
}

void NonoverlappingDistribution::max(float & x) const {
  maxImpl(x);
}

void NonoverlappingDistribution::max(double & x) const {
  maxImpl(x);
}

void NonoverlappingDistribution::max(std::vector<int> & x) const {
  maxImpl(x);
}

void NonoverlappingDistribution::max(std::vector<std::size_t> & x) const {
  maxImpl(x);
}

void NonoverlappingDistribution::max(std::vector<float> & x) const {
  maxImpl(x);
}

void NonoverlappingDistribution::max(std::vector<double> & x) const {
  maxImpl(x);
}

template <typename T>
void NonoverlappingDistribution::maxImpl(T & x) const {
  reductionImpl(x, eckit::mpi::max());
}

// -----------------------------------------------------------------------------
template <typename T>
void NonoverlappingDistribution::reductionImpl(T & x, eckit::mpi::Operation::Code op) const {
  comm_.allReduceInPlace(x, op);
}

template <typename T>
void NonoverlappingDistribution::reductionImpl(std::vector<T> & x,
                                               eckit::mpi::Operation::Code op) const {
  comm_.allReduceInPlace(x.begin(), x.end(), op);
}

// -----------------------------------------------------------------------------
std::unique_ptr<Accumulator<int>>
NonoverlappingDistribution::createAccumulatorImpl(int init) const {
  return createAccumulatorImplT(init);
}

std::unique_ptr<Accumulator<std::size_t>>
NonoverlappingDistribution::createAccumulatorImpl(std::size_t init) const {
  return createAccumulatorImplT(init);
}

std::unique_ptr<Accumulator<float>>
NonoverlappingDistribution::createAccumulatorImpl(float init) const {
  return createAccumulatorImplT(init);
}

std::unique_ptr<Accumulator<double>>
NonoverlappingDistribution::createAccumulatorImpl(double init) const {
  return createAccumulatorImplT(init);
}

std::unique_ptr<Accumulator<std::vector<int>>>
NonoverlappingDistribution::createAccumulatorImpl(const std::vector<int> &init) const {
  return createAccumulatorImplT(init);
}

std::unique_ptr<Accumulator<std::vector<std::size_t>>>
NonoverlappingDistribution::createAccumulatorImpl(const std::vector<std::size_t> &init) const {
  return createAccumulatorImplT(init);
}

std::unique_ptr<Accumulator<std::vector<float>>>
NonoverlappingDistribution::createAccumulatorImpl(const std::vector<float> &init) const {
  return createAccumulatorImplT(init);
}

std::unique_ptr<Accumulator<std::vector<double>>>
NonoverlappingDistribution::createAccumulatorImpl(const std::vector<double> &init) const {
  return createAccumulatorImplT(init);
}

template <typename T>
std::unique_ptr<Accumulator<T>>
NonoverlappingDistribution::createAccumulatorImplT(const T &init) const {
  return boost::make_unique<NonoverlappingDistributionAccumulator<T>>(init, comm_);
}

// -----------------------------------------------------------------------------
void NonoverlappingDistribution::allGatherv(std::vector<size_t> &x) const {
  ASSERT(x.size() == numLocationsOnThisRank_);
  oops::mpi::allGatherv(comm_, x);
}

void NonoverlappingDistribution::allGatherv(std::vector<int> &x) const {
  ASSERT(x.size() == numLocationsOnThisRank_);
  oops::mpi::allGatherv(comm_, x);
}

void NonoverlappingDistribution::allGatherv(std::vector<float> &x) const {
  ASSERT(x.size() == numLocationsOnThisRank_);
  oops::mpi::allGatherv(comm_, x);
}

void NonoverlappingDistribution::allGatherv(std::vector<double> &x) const {
  ASSERT(x.size() == numLocationsOnThisRank_);
  oops::mpi::allGatherv(comm_, x);
}

void NonoverlappingDistribution::allGatherv(std::vector<util::DateTime> &x) const {
  ASSERT(x.size() == numLocationsOnThisRank_);
  oops::mpi::allGatherv(comm_, x);
}

void NonoverlappingDistribution::allGatherv(std::vector<std::string> &x) const {
  ASSERT(x.size() == numLocationsOnThisRank_);
  oops::mpi::allGatherv(comm_, x);
}

size_t NonoverlappingDistribution::globalUniqueConsecutiveLocationIndex(size_t loc) const {
  return numLocationsOnLowerRanks_ + loc;
}

// -----------------------------------------------------------------------------

}  // namespace ioda
