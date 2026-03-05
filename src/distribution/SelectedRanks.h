/*
 * (C) Copyright 2026 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#pragma once

#include "ioda/distribution/NonoverlappingDistribution.h"
#include "ioda/distribution/DistributionParametersBase.h"
#include "oops/util/parameters/RequiredParameter.h"

namespace ioda {

class SelectedRanksParameters : public DistributionParametersBase {
  OOPS_CONCRETE_PARAMETERS(SelectedRanksParameters, DistributionParametersBase)

 public:
  oops::RequiredParameter<std::vector<size_t>> ranks{"ranks", "ranks receiving data", this};
};

// ---------------------------------------------------------------------
/*!
 * \brief Selected Ranks distribution
 *
 * \details This class puts all data onto a list of selected ranks
 * that are specified by a parameter. The other ranks will not have any
 * data. The data will be distributed evenly across the selected ranks, via
 * a round-robin algorithm for just those ranks. This is useful for getting all the data
 * onto just the IO Pool ranks for output.
 *
 */
class SelectedRanks: public NonoverlappingDistribution {
 public:
    typedef SelectedRanksParameters Parameters_;

    SelectedRanks(const eckit::mpi::Comm & Comm,
               const Parameters_ &);
    ~SelectedRanks() override;

    bool isMyRecord(std::size_t) const override;

    std::string name() const override;

 private:
    std::vector<size_t> selectedRanks_;
};

}  // namespace ioda

