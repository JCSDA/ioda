/*
 * (C) Crown copyright 2021, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef DISTRIBUTION_INEFFICIENTDISTRIBUTIONACCUMULATOR_H_
#define DISTRIBUTION_INEFFICIENTDISTRIBUTIONACCUMULATOR_H_

#include <cassert>
#include <vector>

#include "ioda/distribution/Accumulator.h"

namespace ioda {

/// \brief Implementation of the Accumulator interface suitable for the InefficientDistribution.
template <typename T>
class InefficientDistributionAccumulator : public Accumulator<T> {
 public:
  explicit InefficientDistributionAccumulator(const T &)
    : localResult_(0)
  {}

  void addTerm(std::size_t /*loc*/, const T &term) override {
    localResult_ += term;
  }

  T computeResult() const override {
    return localResult_;
  }

 private:
  T localResult_;
};

template <typename T>
class InefficientDistributionAccumulator<std::vector<T>> : public Accumulator<std::vector<T>> {
 public:
  /// Note: only the length of the `init` vector matters -- the values of its elements are ignored.
  explicit InefficientDistributionAccumulator(const std::vector<T> &init)
    : localResult_(init.size(), 0)
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
    return localResult_;
  }

 private:
  std::vector<T> localResult_;
};

}  // namespace ioda

#endif  // DISTRIBUTION_INEFFICIENTDISTRIBUTIONACCUMULATOR_H_
