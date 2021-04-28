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
    explicit NonoverlappingDistribution(const eckit::mpi::Comm & Comm,
                                        const eckit::Configuration & config);
    ~NonoverlappingDistribution() override;

    void patchObs(std::vector<bool> & patchObsVec) const override;

    double dot_product(const std::vector<double> &v1, const std::vector<double> &v2) const override;
    double dot_product(const std::vector<float> &v1, const std::vector<float> &v2) const override;
    double dot_product(const std::vector<int> &v1, const std::vector<int> &v2) const override;

    size_t globalNumNonMissingObs(const std::vector<double> &v1) const override;
    size_t globalNumNonMissingObs(const std::vector<float> &v) const override;
    size_t globalNumNonMissingObs(const std::vector<int> &v) const override;
    size_t globalNumNonMissingObs(const std::vector<std::string> &v) const override;
    size_t globalNumNonMissingObs(const std::vector<util::DateTime> &v) const override;

    void allReduceInPlace(double &x, eckit::mpi::Operation::Code op) const override;
    void allReduceInPlace(float &x, eckit::mpi::Operation::Code op) const override;
    void allReduceInPlace(int &x, eckit::mpi::Operation::Code op) const override;
    void allReduceInPlace(size_t &x, eckit::mpi::Operation::Code op) const override;
    void allReduceInPlace(std::vector<double> &x, eckit::mpi::Operation::Code op) const override;
    void allReduceInPlace(std::vector<float> &x, eckit::mpi::Operation::Code op) const override;
    void allReduceInPlace(std::vector<size_t> &x, eckit::mpi::Operation::Code op) const override;

    void allGatherv(std::vector<size_t> &x) const override;
    void allGatherv(std::vector<int> &x) const override;
    void allGatherv(std::vector<float> &x) const override;
    void allGatherv(std::vector<double> &x) const override;
    void allGatherv(std::vector<util::DateTime> &x) const override;
    void allGatherv(std::vector<std::string> &x) const override;

    void exclusiveScan(size_t &x) const override;

 private:
    template <typename T>
    size_t globalNumNonMissingObsImpl(const std::vector<T> &v) const;
    template <typename T>
    double dot_productImpl(const std::vector<T> &v1, const std::vector<T> &v2) const;
};

}  // namespace ioda

#endif  // DISTRIBUTION_NONOVERLAPPINGDISTRIBUTION_H_
