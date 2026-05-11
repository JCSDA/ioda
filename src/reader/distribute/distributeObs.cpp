/*
 * (C) Copyright 2025 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/reader/distribute/distributeObs.hpp"

#include <numeric>
#include <unordered_set>

#include "eckit/exception/Exceptions.h"
#include "eckit/mpi/Comm.h"
#include "ioda/containers/FrameCols.h"
#include "ioda/containers/FrameRows.h"
#include "ioda/containers/FrameUtils.h"
#include "ioda/containers/IFrame.h"
#include "ioda/containers/CreateIFrame.h"
#include "ioda/core/ObsSourceStats.h"
#include "ioda/distribution/DistributionFactory.h"
#include "ioda/ioPool/ReaderPoolUtils.h"
#include "ioda/reader/distribute/osdfDistributeUtils.hpp"
#include "oops/mpi/mpi.h"
#include "oops/util/Logger.h"

namespace ioda {
namespace reader {
namespace {  // anonymous namespace, so this overload can only be called via the public overloads
//---------------------------------------------------------------------
void distributeObs(const DistributionParametersBase & distParams,
                   const eckit::mpi::Comm & commAll,
                   const std::vector<std::string> & obsGroupVarList,
                   ObsSourceStats & inObsSourceStats,
                   std::shared_ptr<Distribution> & inDist,
                   std::unique_ptr<osdf::IFrame> & inOsdf,
                   ObsSourceStats & outObsSourceStats,
                   std::shared_ptr<Distribution> & outDist,
                   std::unique_ptr<osdf::IFrame> & outOsdf,
                   const bool preserveInputs) {
  oops::Log::trace() << "reader::distributeObs start" << std::endl;

  if (!preserveInputs) {
    // In this mode the code is assuming inputs and outputs are the same objects.
    ASSERT_MSG(&inObsSourceStats == &outObsSourceStats,
      "distributeObs: Input and output observation source stats must be the same in this mode");
    ASSERT_MSG(&inDist == &outDist, "distributeObs: Input and output distributions must "
      "be the same in this mode");
    ASSERT_MSG(&inOsdf == &outOsdf, "distributeObs: Input and output OSDF containers must "
      "be the same in this mode");
  }

  // tempDistOut is the user-requested new distribution type (Step 4 of design, (now moved to top))
  std::shared_ptr<Distribution> tempDistOut = DistributionFactory::create(commAll, distParams);

  // Deal with the special case of zero input locations, and we don't need to create a new osdf to
  // return. Nothing to distribute.
  if (!preserveInputs && inObsSourceStats.sourceNlocs == 0) {
    // Just need to set the output obsSourceStats
    outObsSourceStats.nrecs = 0;
    outObsSourceStats.recNums.resize(0);
    outDist = tempDistOut;
    tempDistOut.reset();
    return;
  }

  // Deal with the special case of when user requests the IdentityDistribution.
  if (distParams.name.value() == "Identity") {
    // Note that we do not allow preserveInputs=true in the Identity distribution case,
    // so the in/out references are definitely the same inside this conditional block.
    ASSERT_MSG(&inObsSourceStats == &outObsSourceStats,
      "distributeObs: Input and output observation source stats must be the same for Identity "
      "distribution");
    ASSERT_MSG(&inDist == &outDist, "distributeObs: Input and output distributions must be "
      "the same for Identity distribution");
    ASSERT_MSG(&inOsdf == &outOsdf, "distributeObs: Input and output OSDF containers must be "
      "the same for Identity distribution");
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
    tempDistOut->setNumberLocations(inOsdf->numRows());

    // Group locations stored on this MPI process into records. Assign a (local) record index to
    // each location.
    std::vector<std::size_t> recNums;
    osdfAssignRecordNumbers(*inOsdf, obsGroupVarList, recNums);
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

    outObsSourceStats.nrecs = numRecords;
    outObsSourceStats.recNums = std::move(recNums);
    outDist = tempDistOut;
    tempDistOut.reset();
    return;
  }

  // standard distribution logic starts here
  std::unique_ptr<osdf::IFrame> globalOsdf = std::make_unique<osdf::FrameCols>();

  // Distribution passed in via inOutDist is used to do allGatherv operations. (Step 1.)
  ASSERT_MSG(inDist, "distributeObs: distribution object passed in via inDist is nullptr.");

  // allGather just the variables needed for record grouping and MPI distribution to the globalOsdf
  // (Step 2.)
  // Global lat/lon are always needed in order to call applyMpiDistribution
  const std::string latColName = "MetaData/latitude";
  const std::string lonColName = "MetaData/longitude";
  std::unordered_set<std::string> computeColumns = {latColName, lonColName};
  for (const auto & colName : obsGroupVarList) {
    computeColumns.insert("MetaData/" + colName);
  }

  // allGather the computation required columns. If preserveInputs=false, these columns also get
  // removed from inOsdf.
  for (const auto & colName : computeColumns) {
    ASSERT_MSG(inOsdf->hasColumn(colName),
               "distributeObs: required column " + colName + " is missing from input osdf.");
    osdf::FrameUtils::callWithSupportedType(
      inOsdf->getColumnType(colName),
      [&](auto typeDiscriminator) {
        using T = decltype(typeDiscriminator);
        std::vector<T> values;
        std::string unit = inOsdf->getColumnUnits(colName);
        inOsdf->getColumn(colName, values);
        inDist->allGatherv(values);
        globalOsdf->appendNewColumn(colName, values, unit);
        if (!preserveInputs) {
          inOsdf->removeColumn(colName);
        }
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

  ioda::IoPool::applyMpiDistribution(tempDistOut, false,
                                    lonValues, latValues,
                                    sourceLocIndices,
                                    sourceRecNums,
                                    localLocIndices,
                                    localRecNums,
                                    localNlocs,
                                    localNrecs);

  // Create the rank-specific osdf container to hold only this rank's locations (Step 6.)
  std::unique_ptr<osdf::IFrame> tempRankOsdf = osdf::createIFrame(inOsdf->frameType());

  // For the global columns already in memory, select the rows from the globalOsdf that
  // correspond to this rank's locations, then remove each global column after it has been
  // processed. (Step 7.)
  auto colNames = globalOsdf->columnNames();
  for (const auto & colName : computeColumns) {
    osdfSelectRankData(globalOsdf, tempRankOsdf, colName, localLocIndices);
    globalOsdf->removeColumn(colName);
  }

  // One column at a time, allGatherv the remaining columns from inOutOsdf to globalOsdf,
  // then select the rows for this rank into tempRankOsdf (Step 8.)
  // It will run faster to remove columns from inOutOsdf if we walk backwards through
  // the column list due to the way removeColumn() is implemented.
  const std::vector<std::string> inColNames = inOsdf->columnNames();
  for (auto it = inColNames.rbegin(); it != inColNames.rend(); ++it) {
    const std::string & colName = *it;
    if (preserveInputs && computeColumns.find(colName) != computeColumns.end()) {
      // This column was already processed earlier, but wasn't removed, so skip it
      continue;
    }
    osdf::FrameUtils::callWithSupportedType(
      inOsdf->getColumnType(colName),
      [&](auto typeDiscriminator) {
        using T = decltype(typeDiscriminator);
        std::vector<T> values;
        std::string unit = inOsdf->getColumnUnits(colName);
        inOsdf->getColumn(colName, values);
        inDist->allGatherv(values);
        globalOsdf->appendNewColumn(colName, values, unit);
        if (!preserveInputs) {
          inOsdf->removeColumn(colName);
        }
        osdfSelectRankData(globalOsdf, tempRankOsdf, colName, localLocIndices);
        globalOsdf->removeColumn(colName);
      });
  }

  outDist = tempDistOut;
  tempDistOut.reset();

  // Update the outOsdf to be the rank-specific osdf (Step 9.)
  outOsdf = std::move(tempRankOsdf);

  // Update the ObsSourceStats
  outObsSourceStats.nrecs = localNrecs;
  outObsSourceStats.recNums = std::move(localRecNums);
  outObsSourceStats.nlocs = localNlocs;
  outObsSourceStats.locIndices = std::move(localLocIndices);

  oops::Log::trace() << "reader::distributeObs end" << std::endl;
}
}  // anonymous namespace

void distributeObs(const DistributionParametersBase & distParams,
                   const eckit::mpi::Comm & commAll,
                   const std::vector<std::string> & obsGroupVarList,
                   ObsSourceStats & inOutObsSourceStats,
                   std::shared_ptr<Distribution> & inOutDist,
                   std::unique_ptr<osdf::IFrame> & inOutOsdf) {
  distributeObs(distParams, commAll, obsGroupVarList,
                inOutObsSourceStats, inOutDist, inOutOsdf,
                inOutObsSourceStats, inOutDist, inOutOsdf,
                false);
}

void distributeObs(const DistributionParametersBase & distParams,
                   const eckit::mpi::Comm & commAll,
                   const std::vector<std::string> & obsGroupVarList,
                   ObsSourceStats & inObsSourceStats,
                   std::shared_ptr<Distribution> & inDist,
                   std::unique_ptr<osdf::IFrame> & inOsdf,
                   ObsSourceStats & outObsSourceStats,
                   std::shared_ptr<Distribution> & outDist,
                   std::unique_ptr<osdf::IFrame> & outOsdf) {
  // Disallow this overload for the Identity distribution since it does not make sense and would
  // waste memory.
  ASSERT_MSG(distParams.name.value() != "Identity",
    "The distributeObs overload with separate input and output variables does not allow the "
    "Identity distribution");
  // If caller is passing both input and output variables, then they must be different variables.
  ASSERT_MSG(&inObsSourceStats != &outObsSourceStats,
    "distributeObs: Input and output observation source stats cannot be the same in this mode");
  ASSERT_MSG(&inDist != &outDist, "distributeObs: Input and output distributions cannot "
    "be the same in this mode");
  ASSERT_MSG(&inOsdf != &outOsdf, "distributeObs: Input and output OSDF containers cannot "
    "be the same in this mode");

  // Set some of the output obs source stats to be the same as the input obs source stats
  // (except for those that are always set in distributeObs)
  outObsSourceStats.sourceNlocs = inObsSourceStats.sourceNlocs;
  outObsSourceStats.gNlocs = inObsSourceStats.gNlocs;
  outObsSourceStats.gNlocsOutsideTimewindow = inObsSourceStats.gNlocsOutsideTimewindow;
  outObsSourceStats.gNlocsRejectQc = inObsSourceStats.gNlocsRejectQc;
  distributeObs(distParams, commAll, obsGroupVarList,
                inObsSourceStats, inDist, inOsdf,
                outObsSourceStats, outDist, outOsdf,
                true);
}

}  // namespace reader
}  // namespace ioda
