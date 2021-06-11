/*
 * (C) Crown copyright 2021, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef DISTRIBUTION_REPLICAOFNONOVERLAPPINGDISTRIBUTION_H_
#define DISTRIBUTION_REPLICAOFNONOVERLAPPINGDISTRIBUTION_H_

#include <memory>

#include "ioda/distribution/NonoverlappingDistribution.h"

namespace ioda {

// ---------------------------------------------------------------------
/*!
 * \brief Distribution assigning each record to a process if and only if a non-overlapping
 * _master distribution_ has done the same.
 */
class ReplicaOfNonoverlappingDistribution : public NonoverlappingDistribution {
 public:
    /// \brief Constructor.
    ///
    /// \param comm
    ///   The communicator used by \p master.
    /// \param master
    ///   Master distribution. The replica will assign each record to a process if and only if
    ///   the master has done the same.
    explicit ReplicaOfNonoverlappingDistribution(const eckit::mpi::Comm &comm,
                                                 std::shared_ptr<const Distribution> master);

    ~ReplicaOfNonoverlappingDistribution() override;

    bool isMyRecord(std::size_t RecNum) const override;

    std::string name() const override { return "ReplicaOfNonoverlappingDistribution"; }

 private:
    std::shared_ptr<const Distribution> master_;
};

}  // namespace ioda

#endif  // DISTRIBUTION_REPLICAOFNONOVERLAPPINGDISTRIBUTION_H_
