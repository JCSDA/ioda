/*
 * (C) Copyright 2017-2019 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef DISTRIBUTION_ROUNDROBIN_H_
#define DISTRIBUTION_ROUNDROBIN_H_

#include "ioda/distribution/NonoverlappingDistribution.h"
#include "ioda/distribution/DistributionParametersBase.h"

namespace ioda {

// ---------------------------------------------------------------------
/*!
 * \brief Round robin distribution
 *
 * \details This class implements a round-robin style of distribution which
 *          optimizes load balancing.
 *
 * \author Xin Zhang (JCSDA)
 */
class RoundRobin: public NonoverlappingDistribution {
 public:
    typedef EmptyDistributionParameters Parameters_;

    RoundRobin(const eckit::mpi::Comm & Comm,
               const Parameters_ &);
    ~RoundRobin() override;

    bool isMyRecord(std::size_t RecNum) const override;

    std::string name() const override;
};

}  // namespace ioda

#endif  // DISTRIBUTION_ROUNDROBIN_H_
