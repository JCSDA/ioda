/*
 * (C) Copyright 2017 UCAR
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

class RoundRobin: public Distribution {
 public:
     ~RoundRobin();
     void distribution(const eckit::mpi::Comm &, const std::size_t gnlocs);
};

}  // namespace ioda

#endif  // DISTRIBUTION_ROUNDROBIN_H_
