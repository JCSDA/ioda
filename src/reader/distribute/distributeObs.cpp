/*
 * (C) Copyright 2025 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/reader/distribute/distributeObs.hpp"

#include <numeric>
#include <unordered_set>

#include "eckit/mpi/Comm.h"
#include "ioda/containers/FrameCols.h"
#include "ioda/containers/FrameMetadata.h"
#include "ioda/containers/FrameRows.h"
#include "ioda/containers/FrameUtils.h"
#include "ioda/containers/IFrame.h"
#include "ioda/containers/CreateIFrame.h"
#include "ioda/core/ObsSourceStats.h"
#include "ioda/distribution/DistributionFactory.h"
#include "ioda/distribution/IdentityDistribution.h"
#include "ioda/ioPool/ReaderPoolUtils.h"
#include "ioda/reader/distribute/osdfDistributeUtils.hpp"
#include "oops/mpi/mpi.h"
#include "oops/util/Logger.h"

namespace ioda {
namespace reader {

//---------------------------------------------------------------------
void distributeObs(const DistributionParametersBase & distParams,
                   const eckit::mpi::Comm & commAll,
                   const std::vector<std::string> & obsGroupVarList,
                   ObsSourceStats & obsSourceStats,
                   std::shared_ptr<Distribution> & ospaceDist,
                   std::unique_ptr<osdf::IFrame> & inOutOsdf) {
  oops::Log::trace() << "reader::distributeObs start" << std::endl;
  // ospaceDist is the user-requested distribution type (Step 4 of design, (now moved to top))
  ospaceDist = DistributionFactory::create(commAll, distParams);

  // Deal with the special case of zero input locations. Nothing to distribute.
  if (obsSourceStats.sourceNlocs == 0) {
    // Just need to set the output obsSourceStats
    obsSourceStats.nrecs = 0;
    obsSourceStats.recNums.resize(0);
    return;
  }

  // Deal with the special case of when user requests the IdentityDistribution.
  if (distParams.name.value() == "Identity") {
    if (!obsGroupVarList.empty()) {
      oops::Log::warning() <<
        "WARNING: an ObsSpace configured to group locations into records is using the "
        "Identity distribution, which keeps each location on the MPI process on which it has "
        "been placed by the reader. This will produce incorrect results (with records split across "
        "multiple MPI processes) unless the reader is configured to ensure locations belonging to "
        "the same record are kept together. This can be done only for the ODB reader. If you are "
        "using any other reader, switch to a different distribution (e.g. RoundRobin); if you're "
        "using the ODB reader, make sure the 'record grouping columns' option in the query file is "
        "set correctly (see "
        "https://jointcenterforsatellitedataassimilation-jedi-docs.readthedocs-hosted.com/en/"
        "latest/inside/jedi-components/ioda/file-formats.html#reading-odb-files-in-parallel "
        "for more information)." << std::endl;
    }
    ospaceDist->setNumberLocations(inOutOsdf->numRows());

    // Group locations stored on this MPI process into records. Assign a (local) record index to
    // each location.
    std::vector<std::size_t> recNums;
    osdfAssignRecordNumbers(*inOutOsdf, obsGroupVarList, recNums);
    // Records are numbered consecutively, so the number of records on this process is simply the
    // highest record index plus one.
    const std::size_t numRecords =
      recNums.empty() ? 0 : (*std::max_element(recNums.begin(), recNums.end()) + 1);
    // Make record indices on different MPI processes unique by offsetting them by the total number
    // of records stored on MPI processes with lower ranks.
    std::size_t recNumOffset = numRecords;
    oops::mpi::exclusiveScan(commAll, recNumOffset);
    std::transform(recNums.begin(), recNums.end(), recNums.begin(),
                   [recNumOffset] (std::size_t recNum) { return recNum + recNumOffset; });

    obsSourceStats.nrecs = numRecords;
    obsSourceStats.recNums = std::move(recNums);
    return;
  }

  // standard distribution logic starts here
  std::unique_ptr<osdf::IFrame> globalOsdf = std::make_unique<osdf::FrameCols>();
  // IdentityDistribution is used to do initial allGatherv operations, regardless of the
  // user-requested distribution type (Step 1.)
  IdentityDistribution tempDistribution(commAll, IdentityDistribution::Parameters_());
  tempDistribution.setNumberLocations(inOutOsdf->numRows());

  // allGather just the variables needed for record grouping and MPI distribution to the globalOsdf
  // (Step 2.)
  // Global lat/lon are always needed in order to call applyMpiDistribution
  const std::string latColName = "MetaData/latitude";
  const std::string lonColName = "MetaData/longitude";
  std::unordered_set<std::string> computeColumns = {latColName, lonColName};
  for (const auto & colName : obsGroupVarList) {
    computeColumns.insert("MetaData/" + colName);
  }

  // allGather the computation required columns, removing them from inOutOsdf as they are processed
  for (const auto & colName : computeColumns) {
    ASSERT_MSG(inOutOsdf->hasColumn(colName),
               "distributeObs: required column " + colName + " is missing from input osdf.");
    osdf::FrameUtils::callWithSupportedType(
      inOutOsdf->getColumnType(colName),
      [&](auto typeDiscriminator) {
        using T = decltype(typeDiscriminator);
        std::vector<T> values;
        inOutOsdf->getColumn(colName, values);
        tempDistribution.allGatherv(values);
        globalOsdf->appendNewColumn(colName, values);
        inOutOsdf->removeColumn(colName);
        });
  }

  // Assign the record number for each location in the globalOsdf (Step 3.)
  std::vector<std::size_t> sourceRecNums;
  osdfAssignRecordNumbers(*globalOsdf, obsGroupVarList, sourceRecNums);

  // (Step 4 of the design doc has been moved to the top of this function)

  // Apply the MPI distribution to determine which locations/records
  // belong to this MPI process (Step 5.)
  std::vector<float> lonValues, latValues;
  globalOsdf->getColumn(lonColName, lonValues);
  globalOsdf->getColumn(latColName, latValues);
  size_t globalNlocs = latValues.size();
  ASSERT(globalNlocs == sourceRecNums.size());
  std::vector<std::size_t> sourceLocIndices(globalNlocs);
  // applyMpiDistribution was designed for use outside the Load/Filter/Distribute paradigm. The
  // sourceLocIndices we pass it here are just 0 to N-1 since we have received pre-filtered data.
  std::iota(sourceLocIndices.begin(), sourceLocIndices.end(), 0);
  std::vector<std::size_t> localLocIndices, localRecNums;
  std::size_t localNlocs, localNrecs;

  ioda::IoPool::applyMpiDistribution(ospaceDist, false,
                                    lonValues, latValues,
                                    sourceLocIndices,
                                    sourceRecNums,
                                    localLocIndices,
                                    localRecNums,
                                    localNlocs,
                                    localNrecs);

  // Create the rank-specific osdf container to hold only this rank's locations (Step 6.)
  std::unique_ptr<osdf::IFrame> rankOsdf =
    osdf::createIFrame(inOutOsdf->frameType());

  // For the global columns already in memory, select the rows from the globalOsdf that
  // correspond to this rank's locations, then remove each global column after it has been
  // processed. (Step 7.)
  auto colNames = globalOsdf->columnNames();
  for (const auto & colName : computeColumns) {
    osdfSelectRankData(globalOsdf, rankOsdf, colName, localLocIndices);
    globalOsdf->removeColumn(colName);
  }

  // One column at a time, allGatherv the remaining columns from inOutOsdf to globalOsdf,
  // then select the rows for this rank into rankOsdf (Step 8.)
  for (const auto & colName : inOutOsdf->columnNames()) {
    osdf::FrameUtils::callWithSupportedType(
      inOutOsdf->getColumnType(colName),
      [&](auto typeDiscriminator) {
        using T = decltype(typeDiscriminator);
        std::vector<T> values;
        inOutOsdf->getColumn(colName, values);
        tempDistribution.allGatherv(values);
        globalOsdf->appendNewColumn(colName, values);
        inOutOsdf->removeColumn(colName);
        osdfSelectRankData(globalOsdf, rankOsdf, colName, localLocIndices);
        globalOsdf->removeColumn(colName);
      });
  }

  // Update the inOutOsdf to be the rank-specific osdf (Step 9.)
  inOutOsdf = std::move(rankOsdf);

  // Update the ObsSourceStats
  obsSourceStats.nrecs = localNrecs;
  obsSourceStats.recNums = localRecNums;
  obsSourceStats.nlocs = localNlocs;
  obsSourceStats.locIndices = localLocIndices;

  oops::Log::trace() << "reader::distributeObs end" << std::endl;
}

}  // namespace reader
}  // namespace ioda
