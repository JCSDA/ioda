/*
 * (C) Copyright 2017-2019 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef DISTRIBUTION_ROUNDROBIN_H_
#define DISTRIBUTION_ROUNDROBIN_H_

#include <vector>

#include "distribution/Distribution.h"
#include "eckit/mpi/Comm.h"
#include "oops/util/Logger.h"

namespace ioda {

// ---------------------------------------------------------------------
/*!
 * \brief Round robin distribution
 *
 * \details This class implements a round-robin style of distribution which
 *          optimzes load balancing.
 *
 * \author Xin Zhang (JCSDA)
 */
class RoundRobin: public Distribution {
 public:
     explicit RoundRobin(const eckit::mpi::Comm & Comm);
     ~RoundRobin();
     bool isMyRecord(std::size_t RecNum) const;
     bool isDistributed() const { return true; }
};

}  // namespace ioda

#endif  // DISTRIBUTION_ROUNDROBIN_H_
