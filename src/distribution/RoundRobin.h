/*
 * (C) Copyright 2017-2019 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef DISTRIBUTION_ROUNDROBIN_H_
#define DISTRIBUTION_ROUNDROBIN_H_

#include <vector>

#include "eckit/mpi/Comm.h"
#include "oops/util/Logger.h"

#include "ioda/distribution/Distribution.h"

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
     bool isMyRecord(std::size_t RecNum) const override;

     void sum(double &x) const override;
     void sum(int &x) const override;
     void sum(size_t &x) const override;
     void sum(std::vector<double> &x) const override;
     void sum(std::vector<size_t> &x) const override;

     void min(double &x) const override;
     void min(float &x) const override;
     void min(int &x) const override;

     void max(double &x) const override;
     void max(float &x) const override;
     void max(int &x) const override;

     void allGatherv(std::vector<size_t> &x) const override;
     void allGatherv(std::vector<int> &x) const override;
     void allGatherv(std::vector<float> &x) const override;
     void allGatherv(std::vector<double> &x) const override;
     void allGatherv(std::vector<util::DateTime> &x) const override;
     void allGatherv(std::vector<std::string> &x) const override;

     void exclusiveScan(size_t &x) const override;
};

}  // namespace ioda

#endif  // DISTRIBUTION_ROUNDROBIN_H_
