/*
 * (C) Copyright 2019 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef DISTRIBUTION_INEFFICIENTDISTRIBUTION_H_
#define DISTRIBUTION_INEFFICIENTDISTRIBUTION_H_

#include <vector>

#include "eckit/mpi/Comm.h"

#include "oops/util/Logger.h"

#include "ioda/distribution/Distribution.h"

namespace ioda {

// ---------------------------------------------------------------------
/*!
 * \brief Inefficient distribution
 *
 * \details This class implements distribution that has copies of all
 *          observations on each processor (to be used for testing)
 *
 */
class InefficientDistribution: public Distribution {
 public:
     explicit InefficientDistribution(const eckit::mpi::Comm & Comm,
                           const eckit::Configuration & config);
     ~InefficientDistribution();

     bool isMyRecord(std::size_t RecNum) const override {return true;};

     void patchObs(std::vector<bool> &) const override;

     double dot_product(const std::vector<double> &v1, const std::vector<double> &v2)
                      const override;
     double dot_product(const std::vector<float> &v1, const std::vector<float> &v2)
                      const override;
     double dot_product(const std::vector<int> &v1, const std::vector<int> &v2)
                      const override;

     size_t globalNumNonMissingObs(const std::vector<double> &v) const override;
     size_t globalNumNonMissingObs(const std::vector<float> &v) const override;
     size_t globalNumNonMissingObs(const std::vector<int> &v) const override;
     size_t globalNumNonMissingObs(const std::vector<std::string> &v) const override;
     size_t globalNumNonMissingObs(const std::vector<util::DateTime> &v) const override;

     // allReduce do nothing for the inefficient distribution. Each processor has each
     // observation so the local reduce is equal to the global reduce
     void allReduceInPlace(double &x, eckit::mpi::Operation::Code op) const override {}
     void allReduceInPlace(float &x, eckit::mpi::Operation::Code op) const override {}
     void allReduceInPlace(int &x, eckit::mpi::Operation::Code op) const override {}
     void allReduceInPlace(size_t &x, eckit::mpi::Operation::Code op) const override {}
     void allReduceInPlace(std::vector<double> &x, eckit::mpi::Operation::Code op) const override {}
     void allReduceInPlace(std::vector<float> &x, eckit::mpi::Operation::Code op) const override {}
     void allReduceInPlace(std::vector<size_t> &x, eckit::mpi::Operation::Code op) const override {}

     // Similarly, allGatherv does nothing, since each processor has all observations.
     void allGatherv(std::vector<size_t> &x) const override {}
     void allGatherv(std::vector<int> &x) const override {}
     void allGatherv(std::vector<float> &x) const override {}
     void allGatherv(std::vector<double> &x) const override {}
     void allGatherv(std::vector<util::DateTime> &x) const override {}
     void allGatherv(std::vector<std::string> &x) const override {}

     void exclusiveScan(size_t &x) const override;
     std::string name() const override {return distName_;}

 private:
     template <typename T>
     double dot_productImpl(const std::vector<T> &v1, const std::vector<T> &v2) const;
     template <typename T>
     size_t globalNumNonMissingObsImpl(const std::vector<T> &v) const;

     // dist name
     const std::string distName_ = "InefficientDistribution";
};

}  // namespace ioda

#endif  // DISTRIBUTION_INEFFICIENTDISTRIBUTION_H_
