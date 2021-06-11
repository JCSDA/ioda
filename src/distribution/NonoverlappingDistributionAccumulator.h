/*
 * (C) Crown copyright 2021, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef DISTRIBUTION_NONOVERLAPPINGDISTRIBUTIONACCUMULATOR_H_
#define DISTRIBUTION_NONOVERLAPPINGDISTRIBUTIONACCUMULATOR_H_

#include <cassert>
#include <vector>

#include "eckit/mpi/Comm.h"
#include "ioda/distribution/Accumulator.h"

namespace ioda {

/// \brief Implementation of the Accumulator interface suitable for the NonoverlappingDistribution.
template <typename T>
class NonoverlappingDistributionAccumulator : public Accumulator<T> {
 public:
  NonoverlappingDistributionAccumulator(const T &, const eckit::mpi::Comm &comm)
    : localResult_(0), comm_(comm)
  {}

  void addTerm(std::size_t /*loc*/, const T &term) override {
    localResult_ += term;
  }

  T computeResult() const override {
    T result = localResult_;
    comm_.allReduceInPlace(result, eckit::mpi::sum());
    return result;
  }

 private:
  T localResult_;
  const eckit::mpi::Comm &comm_;
};

template <typename T>
class NonoverlappingDistributionAccumulator<std::vector<T>> : public Accumulator<std::vector<T>> {
 public:
  /// Note: only the length of the `init` vector matters -- the values of its elements are ignored.
  NonoverlappingDistributionAccumulator(const std::vector<T> &init, const eckit::mpi::Comm &comm)
    : localResult_(init.size(), 0), comm_(comm)
  {}

  void addTerm(std::size_t /*loc*/, const std::vector<T> &term) override {
    // Using assert() rather than ASSERT() since this can be called from a tight loop
    // and I want this extra check to disappear in optimised builds.
    assert(term.size() == localResult_.size());
    for (std::size_t i = 0, n = localResult_.size(); i < n; ++i)
      localResult_[i] += term[i];
  }

  void addTerm(std::size_t /*loc*/, std::size_t item, const T &term) override {
    localResult_[item] += term;
  }

  std::vector<T> computeResult() const override {
    std::vector<T> result = localResult_;
    comm_.allReduceInPlace(result.begin(), result.end(), eckit::mpi::sum());
    return result;
  }

 private:
  std::vector<T> localResult_;
  const eckit::mpi::Comm &comm_;
};

}  // namespace ioda

#endif  // DISTRIBUTION_NONOVERLAPPINGDISTRIBUTIONACCUMULATOR_H_
