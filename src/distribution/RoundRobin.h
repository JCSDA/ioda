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
     RoundRobin(const eckit::mpi::Comm & Comm, const std::size_t Gnlocs);
     RoundRobin(const eckit::mpi::Comm & Comm, const std::size_t Gnlocs,
                const std::vector<std::size_t> & Records);
     ~RoundRobin();
     void distribution();

 private:
     /*! \brief Records numbers which indicate observation location grouping */
     std::vector<std::size_t> record_numbers_;
};

}  // namespace ioda

#endif  // DISTRIBUTION_ROUNDROBIN_H_
