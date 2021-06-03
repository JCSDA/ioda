/*
 * (C) Copyright 2017-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef DISTRIBUTION_NONOVERLAPPINGDISTRIBUTION_H_
#define DISTRIBUTION_NONOVERLAPPINGDISTRIBUTION_H_

#include <vector>

#include "eckit/mpi/Comm.h"
#include "oops/util/Logger.h"

#include "ioda/distribution/Distribution.h"

namespace ioda {

// ---------------------------------------------------------------------
/*!
 * \brief Implements some methods of Distribution in a manner suitable for distributions
 * storing each observation on one and only one process.
 */
class NonoverlappingDistribution : public Distribution {
 public:
    explicit NonoverlappingDistribution(const eckit::mpi::Comm & Comm);
    ~NonoverlappingDistribution() override;

    bool isNonoverlapping() const override { return true; }

    void assignRecord(const std::size_t RecNum, const std::size_t LocNum,
                      const eckit::geometry::Point2 & point) override;
    void patchObs(std::vector<bool> & patchObsVec) const override;
    void computePatchLocs() override;

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

 private:
    template <typename T>
    void minImpl(T & x) const;

    template <typename T>
    void maxImpl(T & x) const;

    template <typename T>
    void reductionImpl(T & x, eckit::mpi::Operation::Code op) const;

    template <typename T>
    void reductionImpl(std::vector<T> & x, eckit::mpi::Operation::Code op) const;

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
    std::unique_ptr<Accumulator<T>> createAccumulatorImplT(const T &init) const;

 private:
    size_t numLocationsOnThisRank_ = 0;
    size_t numLocationsOnLowerRanks_ = 0;
};

}  // namespace ioda

#endif  // DISTRIBUTION_NONOVERLAPPINGDISTRIBUTION_H_
