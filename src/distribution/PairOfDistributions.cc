/*
 * (C) Crown copyright 2021, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/distribution/PairOfDistributions.h"

#include <boost/make_unique.hpp>

#include "oops/util/DateTime.h"
#include "oops/util/Logger.h"

#include "ioda/distribution/PairOfDistributionsAccumulator.h"
#include "eckit/exception/Exceptions.h"

namespace ioda {

// -----------------------------------------------------------------------------
// Note: we don't declare an instance of DistributionMaker<PairOfDistributions>,
// since this distribution must be created programmatically (not from YAML).

// -----------------------------------------------------------------------------
PairOfDistributions::PairOfDistributions(
    const eckit::mpi::Comm & comm,
    std::shared_ptr<const Distribution> first,
    std::shared_ptr<const Distribution> second,
    std::size_t firstNumLocs,
    std::size_t secondRecordNumOffset)
  : Distribution(comm),
    first_(std::move(first)),
    second_(std::move(second)),
    firstNumLocs_(firstNumLocs),
    secondRecordNumOffset_(secondRecordNumOffset)
{
  if (firstNumLocs_ > 0)
    secondGlobalUniqueConsecutiveLocationIndexOffset_ =
      first_->globalUniqueConsecutiveLocationIndex(firstNumLocs_ - 1) + 1;
  else
    secondGlobalUniqueConsecutiveLocationIndexOffset_ = 0;
  first_->max(secondGlobalUniqueConsecutiveLocationIndexOffset_);

  oops::Log::trace() << "PairOfDistributions constructed" << std::endl;
}

// -----------------------------------------------------------------------------
PairOfDistributions::~PairOfDistributions() {
  oops::Log::trace() << "PairOfDistributions destructed" << std::endl;
}

// -----------------------------------------------------------------------------
void PairOfDistributions::assignRecord(const std::size_t /*RecNum*/,
                                                const std::size_t /*LocNum*/,
                                                const eckit::geometry::Point2 & /*point*/) {
  throw eckit::NotImplemented("No new records should be assigned to PairOfDistributions "
                              "after its creation", Here());
}

// -----------------------------------------------------------------------------
bool PairOfDistributions::isMyRecord(std::size_t RecNum) const {
  if (RecNum < secondRecordNumOffset_)
    return first_->isMyRecord(RecNum);
  else
    return second_->isMyRecord(RecNum - secondRecordNumOffset_);
}

// -----------------------------------------------------------------------------
void PairOfDistributions::patchObs(std::vector<bool> & patchObsVec) const {
  // Concatenate vectors produced by the first and second distributions.

  ASSERT(patchObsVec.size() >= firstNumLocs_);
  const std::size_t secondNumObs = patchObsVec.size() - firstNumLocs_;

  std::vector<bool> secondPatchObsVec(secondNumObs);
  second_->patchObs(secondPatchObsVec);

  patchObsVec.resize(firstNumLocs_);
  first_->patchObs(patchObsVec);

  patchObsVec.insert(patchObsVec.end(), secondPatchObsVec.begin(), secondPatchObsVec.end());
}

// -----------------------------------------------------------------------------
void PairOfDistributions::min(int & x) const {
  minImpl(x);
}

void PairOfDistributions::min(std::size_t & x) const {
  minImpl(x);
}

void PairOfDistributions::min(float & x) const {
  minImpl(x);
}

void PairOfDistributions::min(double & x) const {
  minImpl(x);
}

void PairOfDistributions::min(std::vector<int> & x) const {
  minImpl(x);
}

void PairOfDistributions::min(std::vector<std::size_t> & x) const {
  minImpl(x);
}

void PairOfDistributions::min(std::vector<float> & x) const {
  minImpl(x);
}

void PairOfDistributions::min(std::vector<double> & x) const {
  minImpl(x);
}

template <typename T>
void PairOfDistributions::minImpl(T & x) const {
  first_->min(x);
  second_->min(x);
}

// -----------------------------------------------------------------------------
void PairOfDistributions::max(int & x) const {
  maxImpl(x);
}

void PairOfDistributions::max(std::size_t & x) const {
  maxImpl(x);
}

void PairOfDistributions::max(float & x) const {
  maxImpl(x);
}

void PairOfDistributions::max(double & x) const {
  maxImpl(x);
}

void PairOfDistributions::max(std::vector<int> & x) const {
  maxImpl(x);
}

void PairOfDistributions::max(std::vector<std::size_t> & x) const {
  maxImpl(x);
}

void PairOfDistributions::max(std::vector<float> & x) const {
  maxImpl(x);
}

void PairOfDistributions::max(std::vector<double> & x) const {
  maxImpl(x);
}

template <typename T>
void PairOfDistributions::maxImpl(T & x) const {
  first_->max(x);
  second_->max(x);
}

// -----------------------------------------------------------------------------
std::unique_ptr<Accumulator<int>>
PairOfDistributions::createAccumulatorImpl(int init) const {
  return createScalarAccumulator<int>();
}

std::unique_ptr<Accumulator<std::size_t>>
PairOfDistributions::createAccumulatorImpl(std::size_t init) const {
  return createScalarAccumulator<std::size_t>();
}

std::unique_ptr<Accumulator<float>>
PairOfDistributions::createAccumulatorImpl(float init) const {
  return createScalarAccumulator<float>();
}

std::unique_ptr<Accumulator<double>>
PairOfDistributions::createAccumulatorImpl(double init) const {
  return createScalarAccumulator<double>();
}

std::unique_ptr<Accumulator<std::vector<int>>>
PairOfDistributions::createAccumulatorImpl(const std::vector<int> &init) const {
  return createVectorAccumulator<int>(init.size());
}

std::unique_ptr<Accumulator<std::vector<std::size_t>>>
PairOfDistributions::createAccumulatorImpl(const std::vector<std::size_t> &init) const {
  return createVectorAccumulator<std::size_t>(init.size());
}

std::unique_ptr<Accumulator<std::vector<float>>>
PairOfDistributions::createAccumulatorImpl(const std::vector<float> &init) const {
  return createVectorAccumulator<float>(init.size());
}

std::unique_ptr<Accumulator<std::vector<double>>>
PairOfDistributions::createAccumulatorImpl(const std::vector<double> &init) const {
  return createVectorAccumulator<double>(init.size());
}

template <typename T>
std::unique_ptr<Accumulator<T>>
PairOfDistributions::createScalarAccumulator() const {
  return boost::make_unique<PairOfDistributionsAccumulator<T>>(
            first_->createAccumulator<T>(),
            second_->createAccumulator<T>(),
            firstNumLocs_);
}

template <typename T>
std::unique_ptr<Accumulator<std::vector<T>>>
PairOfDistributions::createVectorAccumulator(std::size_t n) const {
  return boost::make_unique<PairOfDistributionsAccumulator<std::vector<T>>>(
            first_->createAccumulator<T>(n),
            second_->createAccumulator<T>(n),
            firstNumLocs_);
}

// -----------------------------------------------------------------------------
void PairOfDistributions::allGatherv(std::vector<size_t> &x) const {
  allGathervImpl(x);
}

void PairOfDistributions::allGatherv(std::vector<int> &x) const {
  allGathervImpl(x);
}

void PairOfDistributions::allGatherv(std::vector<float> &x) const {
  allGathervImpl(x);
}

void PairOfDistributions::allGatherv(std::vector<double> &x) const {
  allGathervImpl(x);
}

void PairOfDistributions::allGatherv(std::vector<util::DateTime> &x) const {
  allGathervImpl(x);
}

void PairOfDistributions::allGatherv(std::vector<std::string> &x) const {
  allGathervImpl(x);
}

template<typename T>
void PairOfDistributions::allGathervImpl(std::vector<T> &x) const {
  std::vector<T> firstX(x.begin(), x.begin() + firstNumLocs_);
  std::vector<T> secondX(x.begin() + firstNumLocs_, x.end());
  first_->allGatherv(firstX);
  second_->allGatherv(secondX);
  x = firstX;
  x.insert(x.end(), secondX.begin(), secondX.end());
}

// -----------------------------------------------------------------------------

size_t PairOfDistributions::globalUniqueConsecutiveLocationIndex(size_t loc) const {
  if (loc < firstNumLocs_)
    return first_->globalUniqueConsecutiveLocationIndex(loc);
  else
    return secondGlobalUniqueConsecutiveLocationIndexOffset_ +
           second_->globalUniqueConsecutiveLocationIndex(loc - firstNumLocs_);
}

// -----------------------------------------------------------------------------

}  // namespace ioda
