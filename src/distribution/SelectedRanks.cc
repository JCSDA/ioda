/*
 * (C) Copyright 2026 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/distribution/SelectedRanks.h"

#include <iostream>

#include "eckit/mpi/Comm.h"
#include "ioda/distribution/DistributionFactory.h"
#include "oops/util/Logger.h"

namespace ioda {

// -----------------------------------------------------------------------------
static DistributionMaker<SelectedRanks> maker("SelectedRanks");

// -----------------------------------------------------------------------------
SelectedRanks::SelectedRanks(const eckit::mpi::Comm & Comm,
                             const Parameters_ & params)
                            : NonoverlappingDistribution(Comm) {
  selectedRanks_ = params.ranks.value();
  for (auto & rank : selectedRanks_) {
    if (rank < 0 || rank >= comm_.size()) {
      throw eckit::BadParameter("SelectedRanks: rank " + std::to_string(rank) +
                                " is out of bounds", Here());
    }
  }
  oops::Log::trace() << "SelectedRanks constructed" << std::endl;
}

// -----------------------------------------------------------------------------
SelectedRanks::~SelectedRanks() {
  oops::Log::trace() << "SelectedRanks destructed" << std::endl;
}

// -----------------------------------------------------------------------------
std::string SelectedRanks::name() const {
  return "SelectedRanks";
}

// -----------------------------------------------------------------------------
/*!
 * \brief Selected Ranks selector
 *
 * \details This distribution puts all data onto a list of selected ranks
 * that are specified by a parameter. The other ranks will not have any
 * data. The records will be distributed evenly across the selected ranks, via
 * a round-robin algorithm for just those ranks. This is useful for getting all the data
 * onto just the IO Pool ranks for output.
 *
 * \param[in] RecNum Record number, checked if belongs on this process element
 */
bool SelectedRanks::isMyRecord(std::size_t RecNum) const {
  bool returnValue = false;
  auto it = std::find(selectedRanks_.begin(), selectedRanks_.end(), comm_.rank());
  if (it != selectedRanks_.end()) {
      auto mySelectedRanksIndex =
              static_cast<std::size_t>(std::distance(selectedRanks_.begin(), it));
      returnValue = (RecNum % selectedRanks_.size() == mySelectedRanksIndex);
  }
  return returnValue;
}

// -----------------------------------------------------------------------------

}  // namespace ioda
