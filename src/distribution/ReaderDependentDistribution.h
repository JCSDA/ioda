/*
 * (C) Crown Copyright 2025 Met Office UK
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef DISTRIBUTION_READERDEPENDENTDISTRIBUTION_H_
#define DISTRIBUTION_READERDEPENDENTDISTRIBUTION_H_

#include "ioda/distribution/NonoverlappingDistribution.h"
#include "ioda/distribution/DistributionParametersBase.h"

namespace ioda {

// ---------------------------------------------------------------------

/// The observation distribution produced by a parallel input file reader and depending on its
/// implementation.
class ReaderDependentDistribution: public NonoverlappingDistribution {
 public:
    typedef EmptyDistributionParameters Parameters_;

    ReaderDependentDistribution(const eckit::mpi::Comm & Comm,
               const Parameters_ &);
    ~ReaderDependentDistribution() override;

    /// \brief Always returns true.
    ///
    /// It is expected that the reader pool will not call this function during the construction of
    /// an ObsSpace, and that it will call setNumberLocations() instead.
    bool isMyRecord(std::size_t RecNum) const override;

    std::string name() const override;
};

}  // namespace ioda

#endif  // DISTRIBUTION_READERDEPENDENTDISTRIBUTION_H_
