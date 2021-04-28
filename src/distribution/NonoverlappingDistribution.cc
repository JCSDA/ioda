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

#include "ioda/distribution/DistributionFactory.h"
#include "oops/mpi/mpi.h"
#include "oops/util/DateTime.h"
#include "oops/util/Logger.h"

namespace ioda {

// -----------------------------------------------------------------------------
NonoverlappingDistribution::NonoverlappingDistribution(const eckit::mpi::Comm & Comm,
                                                       const eckit::Configuration & config)
  : Distribution(Comm, config) {
    oops::Log::trace() << "NonoverlappingDistribution constructed" << std::endl;
}

// -----------------------------------------------------------------------------
NonoverlappingDistribution::~NonoverlappingDistribution() {
    oops::Log::trace() << "NonoverlappingDistribution destructed" << std::endl;
}

// -----------------------------------------------------------------------------
void NonoverlappingDistribution::patchObs(std::vector<bool> & patchObsVec) const {
  std::fill(patchObsVec.begin(), patchObsVec.end(), true);
}

// -----------------------------------------------------------------------------
double NonoverlappingDistribution::dot_product(
                const std::vector<double> &v1, const std::vector<double> &v2) const {
  return dot_productImpl(v1, v2);
}

// -----------------------------------------------------------------------------
double NonoverlappingDistribution::dot_product(
                const std::vector<float> &v1, const std::vector<float> &v2) const {
  return dot_productImpl(v1, v2);
}

// -----------------------------------------------------------------------------
double NonoverlappingDistribution::dot_product(
                const std::vector<int> &v1, const std::vector<int> &v2) const {
  return dot_productImpl(v1, v2);
}

// -----------------------------------------------------------------------------
template <typename T>
double NonoverlappingDistribution::dot_productImpl(
                   const std::vector<T> &v1, const std::vector<T> &v2) const {
  ASSERT(v1.size() == v2.size());
  T missingValue = util::missingValue(missingValue);

  double zz = 0.0;
  for (size_t jj = 0; jj < v1.size(); ++jj) {
    if (v1[jj] != missingValue && v2[jj] != missingValue) {
      zz += v1[jj] * v2[jj];
    }
  }

  comm_.allReduceInPlace(zz, eckit::mpi::sum());
  return zz;
}

// -----------------------------------------------------------------------------
size_t NonoverlappingDistribution::globalNumNonMissingObs(const std::vector<double> &v) const {
  return globalNumNonMissingObsImpl(v);
}

size_t NonoverlappingDistribution::globalNumNonMissingObs(const std::vector<float> &v) const {
  return globalNumNonMissingObsImpl(v);
}

size_t NonoverlappingDistribution::globalNumNonMissingObs(const std::vector<int> &v) const {
  return globalNumNonMissingObsImpl(v);
}

size_t NonoverlappingDistribution::globalNumNonMissingObs(
    const std::vector<std::string> &v) const {
  return globalNumNonMissingObsImpl(v);
}

size_t NonoverlappingDistribution::globalNumNonMissingObs(
    const std::vector<util::DateTime> &v) const {
  return globalNumNonMissingObsImpl(v);
}

template <typename T>
size_t NonoverlappingDistribution::globalNumNonMissingObsImpl(const std::vector<T> &v) const {
  T missingValue = util::missingValue(missingValue);

  size_t nobs = 0;
  for (size_t jj = 0; jj < v.size() ; ++jj) {
    if (v[jj] != missingValue) ++nobs;
  }

  comm_.allReduceInPlace(nobs, eckit::mpi::sum());
  return nobs;
}

// -----------------------------------------------------------------------------
void NonoverlappingDistribution::allReduceInPlace(double &x, eckit::mpi::Operation::Code op) const {
  comm_.allReduceInPlace(x, op);
}

void NonoverlappingDistribution::allReduceInPlace(float &x, eckit::mpi::Operation::Code op) const {
  comm_.allReduceInPlace(x, op);
}

void NonoverlappingDistribution::allReduceInPlace(int &x, eckit::mpi::Operation::Code op) const {
  comm_.allReduceInPlace(x, op);
}

void NonoverlappingDistribution::allReduceInPlace(size_t &x, eckit::mpi::Operation::Code op) const {
  comm_.allReduceInPlace(x, op);
}

void NonoverlappingDistribution::allReduceInPlace(std::vector<double> &x,
                                                  eckit::mpi::Operation::Code op) const {
  comm_.allReduceInPlace(x.begin(), x.end(), op);
}

void NonoverlappingDistribution::allReduceInPlace(std::vector<float> &x,
                                                  eckit::mpi::Operation::Code op) const {
  comm_.allReduceInPlace(x.begin(), x.end(), op);
}

void NonoverlappingDistribution::allReduceInPlace(std::vector<size_t> &x,
                                                  eckit::mpi::Operation::Code op) const {
  comm_.allReduceInPlace(x.begin(), x.end(), op);
}

// -----------------------------------------------------------------------------
void NonoverlappingDistribution::allGatherv(std::vector<size_t> &x) const {
    oops::mpi::allGatherv(comm_, x);
}

void NonoverlappingDistribution::allGatherv(std::vector<int> &x) const {
    oops::mpi::allGatherv(comm_, x);
}

void NonoverlappingDistribution::allGatherv(std::vector<float> &x) const {
    oops::mpi::allGatherv(comm_, x);
}

void NonoverlappingDistribution::allGatherv(std::vector<double> &x) const {
    oops::mpi::allGatherv(comm_, x);
}

void NonoverlappingDistribution::allGatherv(std::vector<util::DateTime> &x) const {
    oops::mpi::allGatherv(comm_, x);
}

void NonoverlappingDistribution::allGatherv(std::vector<std::string> &x) const {
    oops::mpi::allGatherv(comm_, x);
}

void NonoverlappingDistribution::exclusiveScan(size_t &x) const {
    // Could be done with MPI_Exscan, but there's no wrapper for it in eckit::mpi.

    std::vector<size_t> xs(comm_.size());
    comm_.allGather(x, xs.begin(), xs.end());
    x = std::accumulate(xs.begin(), xs.begin() + comm_.rank(), 0);
}

// -----------------------------------------------------------------------------

}  // namespace ioda
