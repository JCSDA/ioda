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
 *          The RoundRobin constructor that has two arguments (Comm, Gnlocs)
 *          is for treating each observation location as a separate record,
 *          i.e. no observation grouping.
 *
 *          The constructor with three arguments (Comm, Gnlocs, Records) is
 *          for the caller to specify observation grouping. The Records
 *          vector needs to be filled with the vaules 0..(number_of_records-1)
 *          denoting the grouping. For example, if you have ten locations and
 *          want to form them into 5 records (adjacent pairs), then Records
 *          needs to be set to:
 *
 *              [ 0, 0, 1, 1, 2, 2, 3, 3, 4, 4 ]
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
     bool isDistributed() const { return true; }

 private:
     /*! \brief Records numbers which indicate observation location grouping */
     std::vector<std::size_t> record_numbers_;
};

}  // namespace ioda

#endif  // DISTRIBUTION_ROUNDROBIN_H_
