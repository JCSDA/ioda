/*
 * (C) Copyright 2017-2019 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/distribution/InefficientDistribution.h"

#include <algorithm>
#include <iostream>
#include <numeric>

#include <boost/make_unique.hpp>

#include "ioda/distribution/DistributionFactory.h"
#include "ioda/distribution/InefficientDistributionAccumulator.h"
#include "oops/util/DateTime.h"
#include "oops/util/Logger.h"

// -----------------------------------------------------------------------------
/*!
 * \brief Inefficient distribution
 *
 * \details This method distributes all observations to all processes.
 */

// -----------------------------------------------------------------------------

namespace ioda {

// -----------------------------------------------------------------------------
static DistributionMaker<InefficientDistribution> maker("InefficientDistribution");

// -----------------------------------------------------------------------------
InefficientDistribution::InefficientDistribution(const eckit::mpi::Comm & Comm,
                                                 const eckit::Configuration & config)
  : Distribution(Comm) {
  oops::Log::trace() << "InefficientDistribution constructed" << std::endl;
}

// -----------------------------------------------------------------------------
InefficientDistribution::~InefficientDistribution() {
  oops::Log::trace() << "InefficientDistribution destructed" << std::endl;
}

// -----------------------------------------------------------------------------
void InefficientDistribution::patchObs(std::vector<bool> & patchObsVec) const {
  // a copy of obs is present on all PEs but only rank=0 "owns" this obs. as patch
  size_t MyRank = comm_.rank();
  if (MyRank == 0) {
    std::fill(patchObsVec.begin(), patchObsVec.end(), true);
  } else {
    std::fill(patchObsVec.begin(), patchObsVec.end(), false);
  }
}

// -----------------------------------------------------------------------------
std::unique_ptr<Accumulator<int>>
InefficientDistribution::createAccumulatorImpl(int init) const {
  return createAccumulatorImplT(init);
}

std::unique_ptr<Accumulator<std::size_t>>
InefficientDistribution::createAccumulatorImpl(std::size_t init) const {
  return createAccumulatorImplT(init);
}

std::unique_ptr<Accumulator<float>>
InefficientDistribution::createAccumulatorImpl(float init) const {
  return createAccumulatorImplT(init);
}

std::unique_ptr<Accumulator<double>>
InefficientDistribution::createAccumulatorImpl(double init) const {
  return createAccumulatorImplT(init);
}

std::unique_ptr<Accumulator<std::vector<int>>>
InefficientDistribution::createAccumulatorImpl(const std::vector<int> &init) const {
  return createAccumulatorImplT(init);
}

std::unique_ptr<Accumulator<std::vector<std::size_t>>>
InefficientDistribution::createAccumulatorImpl(const std::vector<std::size_t> &init) const {
  return createAccumulatorImplT(init);
}

std::unique_ptr<Accumulator<std::vector<float>>>
InefficientDistribution::createAccumulatorImpl(const std::vector<float> &init) const {
  return createAccumulatorImplT(init);
}

std::unique_ptr<Accumulator<std::vector<double>>>
InefficientDistribution::createAccumulatorImpl(const std::vector<double> &init) const {
  return createAccumulatorImplT(init);
}

template <typename T>
std::unique_ptr<Accumulator<T>>
InefficientDistribution::createAccumulatorImplT(const T &init) const {
  return boost::make_unique<InefficientDistributionAccumulator<T>>(init);
}

// -----------------------------------------------------------------------------
size_t InefficientDistribution::globalUniqueConsecutiveLocationIndex(size_t loc) const {
  return loc;
}

// -----------------------------------------------------------------------------
}  // namespace ioda
