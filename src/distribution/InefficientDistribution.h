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

     bool isIdentity() const override {return true;}

     bool isMyRecord(std::size_t RecNum) const override {return true;};

     void patchObs(std::vector<bool> &) const override;

     // The min and max reductions do nothing for the inefficient distribution. Each processor has
     // all observations, so the local reduction is equal to the global reduction.

     void min(int & x) const override {}
     void min(std::size_t & x) const override {}
     void min(float & x) const override {}
     void min(double & x) const override {}
     void min(std::vector<int> & x) const override {}
     void min(std::vector<std::size_t> & x) const override {}
     void min(std::vector<float> & x) const override {}
     void min(std::vector<double> & x) const override {}

     void max(int & x) const override {}
     void max(std::size_t & x) const override {}
     void max(float & x) const override {}
     void max(double & x) const override {}
     void max(std::vector<int> & x) const override {}
     void max(std::vector<std::size_t> & x) const override {}
     void max(std::vector<float> & x) const override {}
     void max(std::vector<double> & x) const override {}

     // Similarly, allGatherv does nothing, since each processor has all observations.
     void allGatherv(std::vector<size_t> &x) const override {}
     void allGatherv(std::vector<int> &x) const override {}
     void allGatherv(std::vector<float> &x) const override {}
     void allGatherv(std::vector<double> &x) const override {}
     void allGatherv(std::vector<util::DateTime> &x) const override {}
     void allGatherv(std::vector<std::string> &x) const override {}

     size_t globalUniqueConsecutiveLocationIndex(size_t loc) const override;

     std::string name() const override {return distName_;}

 private:
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

 private:
     template <typename T>
     std::unique_ptr<Accumulator<T>> createAccumulatorImplT(const T &init) const;

     // dist name
     const std::string distName_ = "InefficientDistribution";
};

}  // namespace ioda

#endif  // DISTRIBUTION_INEFFICIENTDISTRIBUTION_H_
