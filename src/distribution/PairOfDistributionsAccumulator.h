/*
 * (C) Crown copyright 2021, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef DISTRIBUTION_PAIROFDISTRIBUTIONSACCUMULATOR_H_
#define DISTRIBUTION_PAIROFDISTRIBUTIONSACCUMULATOR_H_

#include <memory>
#include <vector>

#include "ioda/distribution/Accumulator.h"

namespace ioda {

/// \brief Implementation of the Accumulator interface suitable for the
/// PairOfDistributions.
template <typename T>
class PairOfDistributionsAccumulator : public Accumulator<T> {
 public:
  PairOfDistributionsAccumulator(std::unique_ptr<Accumulator<T>> firstAccumulator,
                                 std::unique_ptr<Accumulator<T>> secondAccumulator,
                                 std::size_t firstNumLocs)
    : firstAccumulator_(std::move(firstAccumulator)),
      secondAccumulator_(std::move(secondAccumulator)),
      firstNumLocs_(firstNumLocs)
  {}

  void addTerm(std::size_t loc, const T &term) override {
    if (loc < firstNumLocs_)
      firstAccumulator_->addTerm(loc, term);
    else
      secondAccumulator_->addTerm(loc - firstNumLocs_, term);
  }

  T computeResult() const override {
    return firstAccumulator_->computeResult() + secondAccumulator_->computeResult();
  }

 private:
  std::unique_ptr<Accumulator<T>> firstAccumulator_;
  std::unique_ptr<Accumulator<T>> secondAccumulator_;
  std::size_t firstNumLocs_;
};

template <typename T>
class PairOfDistributionsAccumulator<std::vector<T>> : public Accumulator<std::vector<T>> {
 public:
  PairOfDistributionsAccumulator(
      std::unique_ptr<Accumulator<std::vector<T>>> firstAccumulator,
      std::unique_ptr<Accumulator<std::vector<T>>> secondAccumulator,
      std::size_t firstNumLocs)
    : firstAccumulator_(std::move(firstAccumulator)),
      secondAccumulator_(std::move(secondAccumulator)),
      firstNumLocs_(firstNumLocs)
  {}

  void addTerm(std::size_t loc, const std::vector<T> &term) override {
    if (loc < firstNumLocs_)
      firstAccumulator_->addTerm(loc, term);
    else
      secondAccumulator_->addTerm(loc - firstNumLocs_, term);
  }

  void addTerm(std::size_t loc, std::size_t item, const T &term) override {
    if (loc < firstNumLocs_)
      firstAccumulator_->addTerm(loc, item, term);
    else
      secondAccumulator_->addTerm(loc - firstNumLocs_, item, term);
  }

  std::vector<T> computeResult() const override {
    std::vector<T> result = firstAccumulator_->computeResult();
    std::vector<T> secondResult = secondAccumulator_->computeResult();
    for (std::size_t i = 0, n = result.size(); i < n; ++i)
      result[i] += secondResult[i];
    return result;
  }

 private:
  std::unique_ptr<Accumulator<std::vector<T>>> firstAccumulator_;
  std::unique_ptr<Accumulator<std::vector<T>>> secondAccumulator_;
  std::size_t firstNumLocs_;
};

}  // namespace ioda

#endif  // DISTRIBUTION_PAIROFDISTRIBUTIONSACCUMULATOR_H_
