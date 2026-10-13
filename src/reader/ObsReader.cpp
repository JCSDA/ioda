/*
 * (C) Copyright 2025 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include "ioda/reader/ObsReader.hpp"

#include <cstddef>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "eckit/exception/Exceptions.h"
#include "eckit/mpi/Comm.h"

#include "ioda/containers/CreateIFrame.h"
#include "ioda/containers/IFrame.h"
#include "ioda/containers/FrameMetadata.h"
#include "ioda/core/ObsSourceStats.h"
#include "ioda/distribution/Distribution.h"
#include "ioda/distribution/DistributionFactory.h"
#include "ioda/distribution/DistributionParametersBase.h"
#include "ioda/ObsDataIoParameters.h"
#include "ioda/reader/distribute/distributeObs.hpp"
#include "ioda/reader/filter/filterObs.hpp"
#include "ioda/reader/load/loadObs.hpp"

#include "oops/util/Logger.h"
#include "oops/util/TimeWindow.h"
#include "oops/util/Timer.h"

namespace ioda {
namespace reader {

// -----------------------------------------------------------------------------
std::size_t offsetSourceLocationIndices(const eckit::mpi::Comm & commAll,
                                        const std::unique_ptr<osdf::IFrame> & osdf,
                                        const std::size_t offset) {
  std::size_t globalNlocs = 0;
  commAll.allReduce(osdf->numRows(), globalNlocs, eckit::mpi::sum());

  // If we have columns, then we should have a sourceLocationIndices column.
  if (osdf->numCols() > 0) {
    ASSERT(osdf->hasColumn("sourceLocationIndices"));
  }

  // If the offset is zero or the osdf has no rows, there is nothing to renumber.
  if ((offset > 0) && (osdf->numRows() > 0)) {
    std::vector<int> locationIndices;
    osdf->getColumn("sourceLocationIndices", locationIndices);
    for (int & locationIndex : locationIndices) {
      locationIndex += static_cast<int>(offset);
    }
    osdf->setColumn("sourceLocationIndices", locationIndices);
  }
  return globalNlocs;
}

// -----------------------------------------------------------------------------
std::vector<std::size_t> getSourceLocationIndices(const osdf::IFrame & osdf) {
  // The proper result when there are zero rows is an empty vector since
  // there are zero locations.
  if (osdf.numRows() == 0) {
    return {};
  }
  std::vector<int> sourceLocationIndices;
  osdf.getColumn("sourceLocationIndices", sourceLocationIndices);
  return std::vector<std::size_t>(sourceLocationIndices.begin(), sourceLocationIndices.end());
}

// -----------------------------------------------------------------------------
void setLocalSourceStats(const osdf::IFrame & osdf, std::vector<std::size_t> recNums,
                         ObsSourceStats & obsSourceStats) {
  ASSERT(recNums.size() == osdf.numRows());
  obsSourceStats.nlocs = osdf.numRows();
  obsSourceStats.locIndices = getSourceLocationIndices(osdf);
  obsSourceStats.nrecs = std::set<std::size_t>(recNums.begin(), recNums.end()).size();
  obsSourceStats.recNums = std::move(recNums);
}

// -----------------------------------------------------------------------------
void obsRead(const std::vector<eckit::LocalConfiguration>& dataInParams,
             const ioda::IoPool::IoPoolParameters &ioPoolParams,
             const ioda::DistributionParametersBase &distParams, const eckit::mpi::Comm &commAll,
             const std::vector<std::string> &obsVarNames,
             const util::TimeWindow &timeWindow, const std::string &obsName,
             std::shared_ptr<Distribution> &ospaceDist,
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
  //
  // A generated obs source is handed the locations to produce, so its output is the obs
  // set that was asked for rather than a sample to be screened, and the location checks
  // do not apply to it. Mixing generated and read sources in one obs space would make
  // that ambiguous, so the checks are only dropped only when every source is generated.
  bool applyLocationChecks = false;
  std::size_t sourceLocationOffset = 0;
  for (size_t index = 0; index < dataInParams.size(); ++index) {
    ObsDataInParameters dataInParamsSingleFile;
    dataInParamsSingleFile.deserialize(dataInParams[index]);
    if (!sourceIsGenerated(dataInParamsSingleFile)) {
      applyLocationChecks = true;
    }

    // Handle first osdf separately so that destOsdf has correct metadata for append.
    if (index == 0) {
      loadObs(dataInParamsSingleFile, ioPoolParams, commAll, obsVarNames, timeWindow, obsName,
              destOsdf, osdfMetadata);
      sourceLocationOffset +=
        offsetSourceLocationIndices(commAll, destOsdf, sourceLocationOffset);
    } else {
      std::unique_ptr<osdf::IFrame> tempOsdf = osdf::createIFrame(destOsdf->frameType());
      loadObs(dataInParamsSingleFile, ioPoolParams, commAll, obsVarNames, timeWindow, obsName,
              tempOsdf, osdfMetadata);
      // Renumber before the skip checks below so that every rank takes part in the collective
      // inside this call. A file that was warned about and skipped holds no locations anywhere,
      // so it contributes nothing to the running offset.
      sourceLocationOffset +=
        offsetSourceLocationIndices(commAll, tempOsdf, sourceLocationOffset);

      // A warn-and-skipped missing file comes back with no column metadata, which
      // FrameCols::append can't handle on either side, so treat it as a no-op instead.
      if (tempOsdf->numCols() == 0) {
        continue;
      } else if (destOsdf->numCols() == 0) {
        destOsdf = std::move(tempOsdf);
      } else {
        // The location indices were renumbered above; append copies them unchanged.
        destOsdf->append(tempOsdf);
      }
    }
  }

  //----- Filter step -----
  // Apply filters to the OSDF to remove unwanted rows
  const FilterCounts filterCounts =
    filterObs(timeWindow, commAll, destOsdf, osdfMetadata, applyLocationChecks);

  // Record the stats that describe the obs source as a whole. These are counted before the
  // distribute step, which can place a location on more than one rank.
  obsSourceStats.sourceNlocs = sourceLocationOffset;
  commAll.allReduce(destOsdf->numRows(), obsSourceStats.gNlocs, eckit::mpi::sum());
  obsSourceStats.gNlocsOutsideTimewindow = filterCounts.gNlocsOutsideTimewindow;
  obsSourceStats.gNlocsRejectQc = filterCounts.gNlocsRejectQc;

  //----- Distribute step -----
  // Move obs to their intended MPI ranks in the main communicator group
  // (LN) currently only use first file to get variable list, since append only
  // allows files of same metadata (column names and types),
  // so all files must share same list of variables.

  ObsDataInParameters dataInParamsSingleFile;
  dataInParamsSingleFile.deserialize(dataInParams[0]);
  // If the requested distribution does not want obs grouping applied (currently only
  // possible with the Halo distribution), use an empty list of group variables instead, so
  // that every location is assigned its own record.
  const auto obsGroupVarList = distParams.applyObsGrouping() ?
      dataInParamsSingleFile.obsGrouping.value().obsGroupVars.value() :
      std::vector<std::string>();

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
  std::vector<std::size_t> recNums;
  distributeObs(distParams, commAll, obsGroupVarList, ospaceDist, destOsdf, recNums);

  // Record the stats that describe the locations on this rank.
  setLocalSourceStats(*destOsdf, std::move(recNums), obsSourceStats);
  oops::Log::trace() << "reader::obsRead end" << std::endl;
}

}  // namespace reader
}  // namespace ioda
