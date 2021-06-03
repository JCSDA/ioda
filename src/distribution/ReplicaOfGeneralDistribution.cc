/*
 * (C) Crown copyright 2021, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/distribution/ReplicaOfGeneralDistribution.h"

#include <boost/make_unique.hpp>

#include "oops/mpi/mpi.h"
#include "oops/util/DateTime.h"
#include "oops/util/Logger.h"

#include "ioda/distribution/GeneralDistributionAccumulator.h"
#include "eckit/exception/Exceptions.h"

namespace ioda {

// -----------------------------------------------------------------------------
// Note: we don't declare an instance of DistributionMaker<ReplicaOfGeneralDistribution>,
// since this distribution must be created programmatically (not from YAML).

// -----------------------------------------------------------------------------
ReplicaOfGeneralDistribution::ReplicaOfGeneralDistribution(
    const eckit::mpi::Comm & comm,
    std::shared_ptr<const Distribution> masterDist,
    const std::vector<std::size_t> &masterRecordNumbers)
  : Distribution(comm),
    masterDist_(std::move(masterDist)),
    numMasterLocs_(masterRecordNumbers.size())
{
  // Identify records belonging to the master distribution's patch.
  std::vector<bool> isMasterPatchObs(numMasterLocs_);
  masterDist_->patchObs(isMasterPatchObs);
  for (size_t loc = 0; loc < numMasterLocs_; ++loc)
    if (isMasterPatchObs[loc])
      masterPatchRecords_.insert(masterRecordNumbers[loc]);

  oops::Log::trace() << "ReplicaOfGeneralDistribution constructed" << std::endl;
}

// -----------------------------------------------------------------------------
ReplicaOfGeneralDistribution::~ReplicaOfGeneralDistribution() {
  oops::Log::trace() << "ReplicaOfGeneralDistribution destructed" << std::endl;
}

// -----------------------------------------------------------------------------
void ReplicaOfGeneralDistribution::assignRecord(const std::size_t RecNum, const std::size_t LocNum,
                                                const eckit::geometry::Point2 & /*point*/) {
  if (masterDist_->isMyRecord(RecNum)) {
    myRecords_.insert(RecNum);
    myGlobalLocs_.push_back(LocNum);
    isMyPatchObs_.push_back(masterPatchRecords_.find(RecNum) != masterPatchRecords_.end());
  }
}

// -----------------------------------------------------------------------------
bool ReplicaOfGeneralDistribution::isMyRecord(std::size_t RecNum) const {
  return myRecords_.find(RecNum) != myRecords_.end();
}

// -----------------------------------------------------------------------------
void ReplicaOfGeneralDistribution::computePatchLocs(const std::size_t nglocs) {
  ASSERT(std::all_of(myGlobalLocs_.begin(), myGlobalLocs_.end(),
                     [=](std::size_t gloc) { return gloc < nglocs; }));

  // Collect the global location indices of all patch obs on the current process.
  std::vector<std::size_t> patchObsGlobalLocs;
  for (std::size_t i = 0, n = myGlobalLocs_.size(); i < n; ++i)
    if (isMyPatchObs_[i])
      patchObsGlobalLocs.push_back(myGlobalLocs_[i]);
  // Merge with vectors collected on other processes (ordered by MPI rank).
  oops::mpi::allGatherv(comm_, patchObsGlobalLocs);

  // Assign consecutive indices to patch obs ordered by MPI rank.
  // (It is assumed that each location belongs to the patch of some process).
  const std::size_t UNASSIGNED = static_cast<size_t>(-1);
  std::vector<std::size_t> consecutiveLocIndices(nglocs, UNASSIGNED);
  for (std::size_t i = 0, n = patchObsGlobalLocs.size(); i < n; ++i)
    consecutiveLocIndices.at(patchObsGlobalLocs[i]) = i;

  // Find and save the indices of all obs held on the current process.
  globalUniqueConsecutiveLocIndices_.resize(myGlobalLocs_.size());
  for (std::size_t loc = 0; loc < myGlobalLocs_.size(); ++loc) {
    globalUniqueConsecutiveLocIndices_[loc] = consecutiveLocIndices[myGlobalLocs_[loc]];
    if (globalUniqueConsecutiveLocIndices_[loc] == UNASSIGNED)
      throw eckit::SeriousBug("A location does not belong to the patch of any process");
  }

  // Free memory.
  masterPatchRecords_.clear();
  myGlobalLocs_.clear();
  myGlobalLocs_.shrink_to_fit();
}

// -----------------------------------------------------------------------------
void ReplicaOfGeneralDistribution::patchObs(std::vector<bool> & patchObsVec) const {
  patchObsVec = isMyPatchObs_;
}

// -----------------------------------------------------------------------------
void ReplicaOfGeneralDistribution::min(int & x) const {
  minImpl(x);
}

void ReplicaOfGeneralDistribution::min(std::size_t & x) const {
  minImpl(x);
}

void ReplicaOfGeneralDistribution::min(float & x) const {
  minImpl(x);
}

void ReplicaOfGeneralDistribution::min(double & x) const {
  minImpl(x);
}

void ReplicaOfGeneralDistribution::min(std::vector<int> & x) const {
  minImpl(x);
}

void ReplicaOfGeneralDistribution::min(std::vector<std::size_t> & x) const {
  minImpl(x);
}

void ReplicaOfGeneralDistribution::min(std::vector<float> & x) const {
  minImpl(x);
}

void ReplicaOfGeneralDistribution::min(std::vector<double> & x) const {
  minImpl(x);
}

template <typename T>
void ReplicaOfGeneralDistribution::minImpl(T & x) const {
  reductionImpl(x, eckit::mpi::min());
}

// -----------------------------------------------------------------------------
void ReplicaOfGeneralDistribution::max(int & x) const {
  maxImpl(x);
}

void ReplicaOfGeneralDistribution::max(std::size_t & x) const {
  maxImpl(x);
}

void ReplicaOfGeneralDistribution::max(float & x) const {
  maxImpl(x);
}

void ReplicaOfGeneralDistribution::max(double & x) const {
  maxImpl(x);
}

void ReplicaOfGeneralDistribution::max(std::vector<int> & x) const {
  maxImpl(x);
}

void ReplicaOfGeneralDistribution::max(std::vector<std::size_t> & x) const {
  maxImpl(x);
}

void ReplicaOfGeneralDistribution::max(std::vector<float> & x) const {
  maxImpl(x);
}

void ReplicaOfGeneralDistribution::max(std::vector<double> & x) const {
  maxImpl(x);
}

template <typename T>
void ReplicaOfGeneralDistribution::maxImpl(T & x) const {
  reductionImpl(x, eckit::mpi::max());
}

// -----------------------------------------------------------------------------
template <typename T>
void ReplicaOfGeneralDistribution::reductionImpl(T & x, eckit::mpi::Operation::Code op) const {
  comm_.allReduceInPlace(x, op);
}

template <typename T>
void ReplicaOfGeneralDistribution::reductionImpl(std::vector<T> & x,
                                                 eckit::mpi::Operation::Code op) const {
  comm_.allReduceInPlace(x.begin(), x.end(), op);
}

// -----------------------------------------------------------------------------
std::unique_ptr<Accumulator<int>>
ReplicaOfGeneralDistribution::createAccumulatorImpl(int init) const {
  return createAccumulatorImplT(init);
}

std::unique_ptr<Accumulator<std::size_t>>
ReplicaOfGeneralDistribution::createAccumulatorImpl(std::size_t init) const {
  return createAccumulatorImplT(init);
}

std::unique_ptr<Accumulator<float>>
ReplicaOfGeneralDistribution::createAccumulatorImpl(float init) const {
  return createAccumulatorImplT(init);
}

std::unique_ptr<Accumulator<double>>
ReplicaOfGeneralDistribution::createAccumulatorImpl(double init) const {
  return createAccumulatorImplT(init);
}

std::unique_ptr<Accumulator<std::vector<int>>>
ReplicaOfGeneralDistribution::createAccumulatorImpl(const std::vector<int> &init) const {
  return createAccumulatorImplT(init);
}

std::unique_ptr<Accumulator<std::vector<std::size_t>>>
ReplicaOfGeneralDistribution::createAccumulatorImpl(const std::vector<std::size_t> &init) const {
  return createAccumulatorImplT(init);
}

std::unique_ptr<Accumulator<std::vector<float>>>
ReplicaOfGeneralDistribution::createAccumulatorImpl(const std::vector<float> &init) const {
  return createAccumulatorImplT(init);
}

std::unique_ptr<Accumulator<std::vector<double>>>
ReplicaOfGeneralDistribution::createAccumulatorImpl(const std::vector<double> &init) const {
  return createAccumulatorImplT(init);
}

template <typename T>
std::unique_ptr<Accumulator<T>>
ReplicaOfGeneralDistribution::createAccumulatorImplT(const T &init) const {
  return boost::make_unique<GeneralDistributionAccumulator<T>>(init, comm_, isMyPatchObs_);
}

// -----------------------------------------------------------------------------
void ReplicaOfGeneralDistribution::allGatherv(std::vector<size_t> &x) const {
  allGathervImpl(x);
}

void ReplicaOfGeneralDistribution::allGatherv(std::vector<int> &x) const {
  allGathervImpl(x);
}

void ReplicaOfGeneralDistribution::allGatherv(std::vector<float> &x) const {
  allGathervImpl(x);
}

void ReplicaOfGeneralDistribution::allGatherv(std::vector<double> &x) const {
  allGathervImpl(x);
}

void ReplicaOfGeneralDistribution::allGatherv(std::vector<util::DateTime> &x) const {
  allGathervImpl(x);
}

void ReplicaOfGeneralDistribution::allGatherv(std::vector<std::string> &x) const {
  allGathervImpl(x);
}

template<typename T>
void ReplicaOfGeneralDistribution::allGathervImpl(std::vector<T> &x) const {
  // As Halo
  ASSERT(x.size() == isMyPatchObs_.size());

  std::vector<T> xAtPatchObs;
  xAtPatchObs.reserve(x.size());
  for (size_t i = 0; i < x.size(); ++i)
    if (isMyPatchObs_[i])
      xAtPatchObs.push_back(x[i]);
  oops::mpi::allGatherv(comm_, xAtPatchObs);
  x = std::move(xAtPatchObs);
}

// -----------------------------------------------------------------------------

size_t ReplicaOfGeneralDistribution::globalUniqueConsecutiveLocationIndex(size_t loc) const {
  return globalUniqueConsecutiveLocIndices_[loc];
}

// -----------------------------------------------------------------------------

}  // namespace ioda
