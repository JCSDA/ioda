/*
 * (C) Copyright 2025 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include "ioda/reader/ObsReader.hpp"

#include "eckit/mpi/Comm.h"

#include "ioda/containers/IFrame.h"
#include "ioda/containers/FrameMetadata.h"
#include "ioda/core/ObsSourceStats.h"
#include "ioda/distribution/Distribution.h"
#include "ioda/distribution/DistributionParametersBase.h"
#include "ioda/ObsDataIoParameters.h"
#include "ioda/obsIoPool/ObsIoPool.hpp"
#include "ioda/reader/distribute/distributeObs.hpp"
#include "ioda/reader/filter/filterObs.hpp"
#include "ioda/reader/load/loadObs.hpp"

#include "oops/util/Logger.h"
#include "oops/util/TimeWindow.h"
#include "oops/util/Timer.h"

namespace ioda {
namespace reader {

// -----------------------------------------------------------------------------
void obsRead(const ioda::ObsDataInParameters & dataInParams,
             const ioda::IoPool::IoPoolParameters & ioPoolParams,
             const ioda::DistributionParametersBase & distParams,
             const eckit::mpi::Comm & commAll,
             const util::TimeWindow & timeWindow,
             std::shared_ptr<Distribution> & ospaceDist,
             std::unique_ptr<osdf::IFrame> & destOsdf,
             ioda::ObsSourceStats & obsSourceStats,
             osdf::FrameMetadata & osdfMetadata) {
  oops::Log::trace() << "reader::obsRead start" << std::endl;
  util::Timer timer("ioda::reader", "obsRead");
  // The read is done in three indepndent steps:
  //   1. Load: collectively load data from the input file into an OSDF
  //   2. Filter: apply any filters to the OSDF to remove unwanted rows
  //   3. Distribute: distribute the OSDF rows to all ranks in comm

  //----- Load step -----
  // Populate all MPI tasks with an appropriate OSDF container.
  // Members of the io pool get obs data from the input file, and
  // non-io pool members get an "empty" OSDF container (which contains
  // a column metadata row matching the io pool member OSDFs, but has
  // zero data rows).
  loadObs(dataInParams, ioPoolParams, commAll, destOsdf, osdfMetadata);

  //----- Filter step -----
  // Apply filters to the OSDF to remove unwanted rows
  filterObs(timeWindow, commAll, obsSourceStats, destOsdf, osdfMetadata);

  //----- Distribute step -----
  // Move obs to their intended MPI ranks in the main communicator group
  const auto obsGroupVarList = dataInParams.obsGrouping.value().obsGroupVars.value();
  distributeObs(distParams, commAll, obsGroupVarList, osdfMetadata, obsSourceStats, ospaceDist,
                destOsdf);
  oops::Log::trace() << "reader::obsRead end" << std::endl;
}

}  // namespace reader
}  // namespace ioda
