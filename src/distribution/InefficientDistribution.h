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
     explicit InefficientDistribution(const eckit::mpi::Comm & Comm);
     ~InefficientDistribution();

     bool isMyRecord(std::size_t RecNum) const override;

     void sum(double &x) const override {};
     void sum(int &x) const override {};
     void sum(size_t &x) const override {};
     void sum(std::vector<double> &x) const override {};
     void sum(std::vector<size_t> &x) const override {};

     void min(double &x) const override {};
     void min(float &x) const override {};
     void min(int &x) const override {};

     void max(double &x) const override {};
     void max(float &x) const override {};
     void max(int &x) const override {};
};

}  // namespace ioda

#endif  // DISTRIBUTION_INEFFICIENTDISTRIBUTION_H_
