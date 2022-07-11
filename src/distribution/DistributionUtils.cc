/*
 * (C) Crown copyright 2021, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/distribution/Accumulator.h"
#include "ioda/distribution/Distribution.h"
#include "ioda/distribution/DistributionParametersBase.h"
#include "ioda/distribution/DistributionUtils.h"
#include "ioda/distribution/InefficientDistribution.h"
#include "ioda/distribution/ReplicaOfNonoverlappingDistribution.h"
#include "ioda/distribution/ReplicaOfGeneralDistribution.h"

#include "oops/util/DateTime.h"
#include "oops/util/missingValues.h"

namespace ioda {

namespace {

template <typename T>
std::size_t globalNumNonMissingObsImpl(const Distribution &dist,
                                       std::size_t numVariables, const std::vector<T> &v) {
  const T missingValue = util::missingValue(missingValue);
  const std::size_t numLocations = v.size() / numVariables;

  // Local reduction
  std::unique_ptr<Accumulator<std::size_t>> accumulator = dist.createAccumulator<std::size_t>();
  for (size_t loc = 0, element = 0; loc < numLocations; ++loc) {
    std::size_t term = 0;
    for (size_t var = 0; var < numVariables; ++var, ++element)
      if (v[element] != missingValue)
        ++term;
    accumulator->addTerm(loc, term);
  }
  // Global reduction
  return accumulator->computeResult();
}

template <typename T>
double dotProductImpl(const Distribution &dist,
                      std::size_t numVariables,
                      const std::vector<T> &v1,
                      const std::vector<T> &v2) {
  ASSERT(v1.size() == v2.size());
  const T missingValue = util::missingValue(missingValue);
  const std::size_t numLocations = v1.size() / numVariables;

  // Local reduction
  std::unique_ptr<Accumulator<double>> accumulator = dist.createAccumulator<double>();
  for (size_t loc = 0, element = 0; loc < numLocations; ++loc) {
    double term = 0;
    for (size_t var = 0; var < numVariables; ++var, ++element)
      if (v1[element] != missingValue && v2[element] != missingValue)
        term += v1[element] * v2[element];
    accumulator->addTerm(loc, term);
  }
  // Global reduction
  return accumulator->computeResult();
}

}  // namespace

// -----------------------------------------------------------------------------
double dotProduct(const Distribution &dist,
                  std::size_t numVariables,
                  const std::vector<double> &v1,
                  const std::vector<double> &v2) {
  return dotProductImpl(dist, numVariables, v1, v2);
}

double dotProduct(const Distribution &dist,
                  std::size_t numVariables,
                  const std::vector<float> &v1,
                  const std::vector<float> &v2) {
  return dotProductImpl(dist, numVariables, v1, v2);
}

double dotProduct(const Distribution &dist,
                  std::size_t numVariables,
                  const std::vector<int> &v1,
                  const std::vector<int> &v2) {
  return dotProductImpl(dist, numVariables, v1, v2);
}

double dotProduct(const Distribution &dist,
                  std::size_t numVariables,
                  const std::vector<int64_t> &v1,
                  const std::vector<int64_t> &v2) {
  return dotProductImpl(dist, numVariables, v1, v2);
}

// -----------------------------------------------------------------------------
std::size_t globalNumNonMissingObs(const Distribution &dist,
                                   std::size_t numVariables,
                                   const std::vector<double> &v) {
  return globalNumNonMissingObsImpl(dist, numVariables, v);
}

std::size_t globalNumNonMissingObs(const Distribution &dist,
                                   std::size_t numVariables,
                                   const std::vector<float> &v) {
  return globalNumNonMissingObsImpl(dist, numVariables, v);
}

std::size_t globalNumNonMissingObs(const Distribution &dist,
                                   std::size_t numVariables,
                                   const std::vector<int> &v) {
  return globalNumNonMissingObsImpl(dist, numVariables, v);
}

std::size_t globalNumNonMissingObs(const Distribution &dist,
                                   std::size_t numVariables,
                                   const std::vector<std::string> &v) {
  return globalNumNonMissingObsImpl(dist, numVariables, v);
}

std::size_t globalNumNonMissingObs(const Distribution &dist,
                                   std::size_t numVariables,
                                   const std::vector<util::DateTime> &v) {
  return globalNumNonMissingObsImpl(dist, numVariables, v);
}

std::size_t globalNumNonMissingObs(const Distribution &dist,
                                   std::size_t numVariables,
                                   const std::vector<bool> &v) {
  const std::size_t numLocations = v.size() / numVariables;

  // Local reduction
  std::unique_ptr<Accumulator<std::size_t>> accumulator = dist.createAccumulator<std::size_t>();
  for (size_t loc = 0; loc < numLocations; ++loc)
    accumulator->addTerm(loc, numVariables);

  // Global reduction
  return accumulator->computeResult();
}

// -----------------------------------------------------------------------------
std::shared_ptr<Distribution> createReplicaDistribution(
    const eckit::mpi::Comm & comm,
    std::shared_ptr<const Distribution> master,
    const std::vector<std::size_t> &masterRecordNums) {
  if (master->isNonoverlapping())
    return std::make_shared<ReplicaOfNonoverlappingDistribution>(comm, std::move(master));
  else if (master->isIdentity())
    return std::make_shared<InefficientDistribution>(comm, EmptyDistributionParameters());
  else
    return std::make_shared<ReplicaOfGeneralDistribution>(comm, std::move(master),
                                                          masterRecordNums);
}

}  // namespace ioda
