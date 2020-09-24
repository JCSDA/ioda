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
     bool isDistributed() const override { return false; }
     void sum(double &x) override {};
};

}  // namespace ioda

#endif  // DISTRIBUTION_INEFFICIENTDISTRIBUTION_H_
