/*
 * (C) Copyright 2026 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/writer/collect/collectObs.hpp"

#include "eckit/mpi/Comm.h"
#include "eckit/config/LocalConfiguration.h"

#include "ioda/containers/IFrame.h"
#include "ioda/distribution/Distribution.h"
#include "ioda/distribution/DistributionFactory.h"
#include "ioda/distribution/SelectedRanks.h"
#include "ioda/obsIoPool/ObsIoPool.hpp"
#include "ioda/reader/distribute/distributeObs.hpp"

#include "oops/mpi/mpi.h"
#include "oops/util/Logger.h"

namespace ioda {
namespace writer {

//---------------------------------------------------------------------
void collectObs(const std::unique_ptr<ObsIoPool::ObsIoPool> & obsIoPool,
                std::shared_ptr<Distribution> & ospaceDist,
                ObsSourceStats & obsSourceStats,
                std::unique_ptr<osdf::IFrame> & inOutOsdf) {
  oops::Log::trace() << "writer::collectObs start" << std::endl;
  // Create a SelctedRanks distribution and use that to collect the obs on
  // the io pool ranks using distributeObs.
  //
  // For the SelectedRanks distribution we need to create an eckit
  // LocalConfiguration object corresponding to this YAML:
  //    distribution:
  //      name: SelectedRanks
  //      ranks: [ allComm rank numbers that belong to the io pool ]

  // Gather onto all ranks a vector whose index i is
  //   1 if rank<i> is in the io pool
  //   0 if rank<i> is not in the pool
  std::vector<int> ranksInIoPool(1, (obsIoPool->inIoPool() ? 1 : 0));
  oops::mpi::allGatherv(obsIoPool->commAll(), ranksInIoPool);

  // Transform the gathered vector into a vector containing just the io pool ranks
  // Assume that the number of io pool ranks will be small so using push_back
  // is okay.
  std::vector<int> ioPoolRanks;
  for (std::size_t i = 0; i < ranksInIoPool.size(); ++i) {
    if (ranksInIoPool[i] == 1) {
      ioPoolRanks.push_back(i);
    }
  }

  // Form the SelectedRanks distribution
  eckit::LocalConfiguration selectedRanksConfig;
  selectedRanksConfig.set("name", "SelectedRanks");
  selectedRanksConfig.set("ranks", ioPoolRanks);

  // Create the distribution.
  SelectedRanksParameters selectedRanksParams;
  selectedRanksParams.validateAndDeserialize(selectedRanksConfig);

  std::shared_ptr<Distribution> selectDist =
    DistributionFactory::create(obsIoPool->commAll(), selectedRanksParams);
  selectDist->setNumberLocations(inOutOsdf->numRows());

  // Apply the distribution
  ioda::reader::distributeObs(selectedRanksParams, obsIoPool->commAll(), {},
                              obsSourceStats, ospaceDist, inOutOsdf);
  oops::Log::trace() << "writer::collectObs end" << std::endl;
}

}  // namespace writer
}  // namespace ioda
