/*
 * (C) Crown copyright 2021, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef DISTRIBUTION_MASTERANDREPLICADISTRIBUTION_H_
#define DISTRIBUTION_MASTERANDREPLICADISTRIBUTION_H_

#include "ioda/distribution/Distribution.h"

namespace ioda {

/// \brief Represents a concatenation of locations and records from two distributions.
class PairOfDistributions : public Distribution {
 public:
  /// \brief Create a PairOfDistributions object.
  ///
  /// \param comm
  ///   Communicator used by both `first` and `second`.
  /// \param first
  ///   The first distribution.
  /// \param second
  ///   The second distribution.
  /// \param firstNumLocs
  ///   Number of locations in the first distribution held on the calling process. The
  ///   PairOfDistributions will map locations 0, 1, ..., (`firstNumLocs` - 1) to the
  ///   same locations from the first distribution, and locations `firstNumLocs`,
  ///   (`firstNumLocs` + 1), ... to locations 0, 1, ... from the second distribution.
  /// \param secondRecordNumOffset
  ///   Offset to apply to record indices from the second distribution. The PairOfDistributions
  ///   will map each record `r` < `secondRecordNumOffset` to record `r` from the
  ///   first distribution, and each record `r` >= `secondRecordNumOffset` to record
  ///   `r` - `secondRecordNumOffset` from the second distribution.
  explicit PairOfDistributions(const eckit::mpi::Comm & comm,
                                        std::shared_ptr<const Distribution> first,
                                        std::shared_ptr<const Distribution> second,
                                        std::size_t firstNumLocs,
                                        std::size_t secondRecordNumOffset);
  ~PairOfDistributions() override;

  /// This function should not be called. Records are meant to be assigned to the two distributions
  /// before wrapping them in a PairOfDistributions.
  void assignRecord(const std::size_t RecNum, const std::size_t LocNum,
                    const eckit::geometry::Point2 & point) override;
  bool isMyRecord(std::size_t RecNum) const override;
  void patchObs(std::vector<bool> &) const override;

  void min(int & x) const override;
  void min(std::size_t & x) const override;
  void min(float & x) const override;
  void min(double & x) const override;
  void min(std::vector<int> & x) const override;
  void min(std::vector<std::size_t> & x) const override;
  void min(std::vector<float> & x) const override;
  void min(std::vector<double> & x) const override;

  void max(int & x) const override;
  void max(std::size_t & x) const override;
  void max(float & x) const override;
  void max(double & x) const override;
  void max(std::vector<int> & x) const override;
  void max(std::vector<std::size_t> & x) const override;
  void max(std::vector<float> & x) const override;
  void max(std::vector<double> & x) const override;

  void allGatherv(std::vector<size_t> &x) const override;
  void allGatherv(std::vector<int> &x) const override;
  void allGatherv(std::vector<float> &x) const override;
  void allGatherv(std::vector<double> &x) const override;
  void allGatherv(std::vector<util::DateTime> &x) const override;
  void allGatherv(std::vector<std::string> &x) const override;

  size_t globalUniqueConsecutiveLocationIndex(size_t loc) const override;

  std::string name() const override { return "PairOfDistributions"; }

 private:
  template <typename T>
  void minImpl(T & x) const;

  template <typename T>
  void maxImpl(T & x) const;

  std::unique_ptr<Accumulator<int>>
      createAccumulatorImpl(int init) const override;
  std::unique_ptr<Accumulator<std::size_t>>
      createAccumulatorImpl(std::size_t init) const override;
  std::unique_ptr<Accumulator<float>>
      createAccumulatorImpl(float init) const override;
  std::unique_ptr<Accumulator<double>>
      createAccumulatorImpl(double init) const override;
  std::unique_ptr<Accumulator<std::vector<int>>>
      createAccumulatorImpl(const std::vector<int> &init) const override;
  std::unique_ptr<Accumulator<std::vector<std::size_t>>>
      createAccumulatorImpl(const std::vector<std::size_t> &init) const override;
  std::unique_ptr<Accumulator<std::vector<float>>>
      createAccumulatorImpl(const std::vector<float> &init) const override;
  std::unique_ptr<Accumulator<std::vector<double>>>
      createAccumulatorImpl(const std::vector<double> &init) const override;

  template <typename T>
  std::unique_ptr<Accumulator<T>> createScalarAccumulator() const;

  template <typename T>
  std::unique_ptr<Accumulator<std::vector<T>>> createVectorAccumulator(std::size_t n) const;

  template <typename T>
  void allGathervImpl(std::vector<T> &x) const;

  std::shared_ptr<const Distribution> first_;
  std::shared_ptr<const Distribution> second_;
  std::size_t firstNumLocs_;
  std::size_t secondRecordNumOffset_;
  std::size_t secondGlobalUniqueConsecutiveLocationIndexOffset_;
};

}  // namespace ioda

#endif  // DISTRIBUTION_MASTERANDREPLICADISTRIBUTION_H_
