/*
 * (C) Copyright 2025 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include "ioda/reader/ObsReader.hpp"

#include "eckit/mpi/Comm.h"

#include "ioda/containers/CreateIFrame.h"
#include "ioda/containers/IFrame.h"
#include "ioda/containers/FrameMetadata.h"
#include "ioda/core/ObsSourceStats.h"
#include "ioda/distribution/Distribution.h"
#include "ioda/distribution/DistributionFactory.h"
#include "ioda/distribution/DistributionParametersBase.h"
#include "ioda/distribution/IdentityDistribution.h"
#include "ioda/ObsDataIoParameters.h"
#include "ioda/obsIoPool/ObsIoPool.hpp"
#include "ioda/reader/distribute/distributeObs.hpp"
#include "ioda/reader/filter/filterObs.hpp"
#include "ioda/reader/load/loadObs.hpp"
#include "ioda/containers/FrameCols.h"
#include "ioda/containers/FrameRows.h"

#include "oops/util/Logger.h"
#include "oops/util/TimeWindow.h"
#include "oops/util/Timer.h"

namespace ioda {
namespace reader {

// -----------------------------------------------------------------------------
void obsRead(const std::vector<eckit::LocalConfiguration>& dataInParams,
             const ioda::IoPool::IoPoolParameters &ioPoolParams,
             const ioda::DistributionParametersBase &distParams, const eckit::mpi::Comm &commAll,
             const std::vector<std::string> &obsVarNames,
             const util::TimeWindow &timeWindow, std::shared_ptr<Distribution> &ospaceDist,
             std::unique_ptr<osdf::IFrame> &destOsdf, ioda::ObsSourceStats &obsSourceStats,
             osdf::FrameMetadata &osdfMetadata) {
  oops::Log::trace() << "reader::obsRead start" << std::endl;
  util::Timer timer("ioda::reader", "obsRead");
  // The read is done in three independent steps:
  //   1. Load: collectively load data from the input file into an OSDF
  //   2. Filter: apply any filters to the OSDF to remove unwanted rows
  //   3. Distribute: distribute the OSDF rows to all ranks in comm

  //----- Load step -----
  // Populate all MPI tasks with an appropriate OSDF container.
  // Members of the io pool get obs data from the input files, and
  // non-io pool members get an "empty" OSDF container (which contains
  // a column metadata row matching the io pool member OSDFs, but has
  // zero data rows).

  // Loop through input files, load into a temporary osdf, then append to destOsdf
  // obsSourceStats are updated in the filter and distribute steps.
  for (size_t index = 0; index < dataInParams.size(); ++index) {
    ObsDataInParameters dataInParamsSingleFile;
    dataInParamsSingleFile.deserialize(dataInParams[index]);

    // Handle first osdf separately so that destOsdf has correct metadata for append.
    if (index == 0) {
      loadObs(dataInParamsSingleFile, ioPoolParams, commAll, obsVarNames, timeWindow, destOsdf,
              osdfMetadata);
    } else {
      std::unique_ptr<osdf::IFrame> tempOsdf = osdf::createIFrame(destOsdf->frameType());
      loadObs(dataInParamsSingleFile, ioPoolParams, commAll, obsVarNames, timeWindow, tempOsdf,
              osdfMetadata);
      destOsdf->append(tempOsdf);
    }
  }

  //----- Filter step -----
  // Apply filters to the OSDF to remove unwanted rows
  filterObs(timeWindow, commAll, obsSourceStats, destOsdf, osdfMetadata);

  //----- Distribute step -----
  // Move obs to their intended MPI ranks in the main communicator group
  // (LN) currently only use first file to get variable list, since append only
  // allows files of same metadata (column names and types),
  // so all files must share same list of variables.

  ObsDataInParameters dataInParamsSingleFile;
  dataInParamsSingleFile.deserialize(dataInParams[0]);
  const auto obsGroupVarList = dataInParamsSingleFile.obsGrouping.value().obsGroupVars.value();

  if (ospaceDist) {
    oops::Log::info() << "WARNING: the reader::obsRead function received a non-null pointer " <<
      "in its 'ospaceDist' parameter. 'ospaceDist' is an output-only parameter, so the incoming " <<
      "value will be ignored and overwritten." << std::endl;
  }
  // Temporarily create an IdentityDistribution to represent the current distribution.
  // 'ospaceDist' will get updated to the requested distribution (defined in distParams) by the call
  // to 'distributeObs' below.
  //
  // Note there is an assumption here that the data is currently distributed
  // in a non-overlapping way across the ranks, because IdentityDistribution
  // is a subclass of NonOverlappingDistribution.
  std::unique_ptr<DistributionParametersBase> identityDistParams =
    createBaseDistributionParams("Identity");
  ospaceDist = DistributionFactory::create(commAll, *identityDistParams);
  ospaceDist->setNumberLocations(destOsdf->numRows());
  distributeObs(distParams, commAll, obsGroupVarList, obsSourceStats, ospaceDist, destOsdf);
  oops::Log::trace() << "reader::obsRead end" << std::endl;
}

}  // namespace reader
}  // namespace ioda
