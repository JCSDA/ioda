/*
 * (C) Copyright 2022 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/ioPool/ReaderSinglePool.h"

#include <mpi.h>

#include <algorithm>
#include <cstdio>
#include <memory>
#include <numeric>
#include <sstream>
#include <utility>

#include "eckit/config/LocalConfiguration.h"
#include "eckit/exception/Exceptions.h"

#include "ioda/Copying.h"
#include "ioda/distribution/Distribution.h"
#include "ioda/Engines/EngineUtils.h"
#include "ioda/Engines/HH.h"
#include "ioda/Exception.h"
#include "ioda/ioPool/ReaderPoolFactory.h"
#include "ioda/ioPool/ReaderPoolUtils.h"

#include "oops/util/Logger.h"
#include "oops/util/missingValues.h"

namespace ioda {

// Io pool factory maker
static ReaderPoolMaker<ReaderSinglePool> makerReaderSinglePool("SinglePool");

//--------------------------------------------------------------------------------------
ReaderSinglePool::ReaderSinglePool(const IoPoolParameters & configParams,
                                   const ReaderPoolCreationParameters & createParams)
                                       : ReaderPoolBase(configParams, createParams) {
    readerSrc_ = "New Reader (under development)";
}

//--------------------------------------------------------------------------------------
void ReaderSinglePool::initialize() {
    // First establish the reader pool which consists of assigning ranks in the "All"
    // communicator to the "Pool" communicator and then splitting the "All" communicator
    // to form the "Pool" communicator.

    // For now, the target pool size is simply the minumum of the specified (or default) max
    // pool size and the size of the comm_all_ communicator group.
    setTargetPoolSize();

    // This call will return a data structure that shows how to assign the ranks
    // to the io pools, plus which non io pool ranks get associated with the io pool
    // ranks. Only rank 0 needs to have this data since it will be used to form and
    // send the assignments to the other ranks.
    std::map<int, std::vector<int>> rankGrouping;
    groupRanks(rankGrouping);

    // This call will fill in the vector data member rank_assignment_, which holds all of
    // the ranks each member of the io pool needs to communicate with to collect the
    // variable data. Use the patch nlocs (ie, the number of locations "owned" by this
    // rank) to represent the number of locations after any duplicated locations are
    // removed.
    oops::Log::debug() << "nlocs: " << this->nlocs() << std::endl;
    assignRanksToIoPool(this->nlocs(), rankGrouping);

    // Create the io pool communicator group using the split communicator command.
    createIoPool(rankGrouping);

    // Second, run the pre-processing steps that establish which locations go to
    // which ranks. These steps include the timing window filtering, quality checks,
    // obs grouping and applying the mpi distribution scheme.

    // Rank 0 is the only rank that opens the input file. The time window filter, quality
    // checks, obs grouping and applicatin of the MPI distribution are performed with
    // all ranks (in commAll_) and rank 0 writes out results into a temp file. Eventually,
    // rank 0 will rearrange the locations and split up into files for each rank in the
    // io pool.
    Group fileGroup;
    bool applyLocationsCheck;
    if (this->commAll().rank() == 0) {
        Engines::ReaderCreationParameters
            createParams(winStart_, winEnd_, *commPool_, commTime_,
                         obsVarNames_, isParallelIo_);
        std::unique_ptr<Engines::ReaderBase> readerEngine =
            Engines::ReaderFactory::create(readerParams_, createParams);

        fileGroup = readerEngine->getObsGroup();

        // Engine initialization
        readerEngine->initialize();

        // Collect the destination from the reader engine instance
        std::ostringstream ss;
        ss << *readerEngine;
        readerSrc_ = ss.str();

        // Send the engine applyLocationsCheck() value to the other ranks
        applyLocationsCheck = readerEngine->applyLocationsCheck();
        oops::mpi::broadcastBool(this->commAll(), applyLocationsCheck, 0);
    } else {
        oops::mpi::broadcastBool(this->commAll(), applyLocationsCheck, 0);
    }

    // Rank 0 does the preliminary checking and formation of the source location
    // indices and source record numbers. These are identical operations on each
    // MPI task so file io can be reduced by having rank 0 only do the io, generate
    // the indices and record numbers and broadcast that information to the other
    // ranks.

    // Check for required variables
    DateTimeFormat dtimeFormat;  // which format the source date time variable is using
    bool emptyFile;              // true if the source is empty
    checkForRequiredVars(fileGroup, this->commAll(), readerSrc_, dtimeFormat, emptyFile);

    // Read and convert the dtimeValues to the current epoch format if older formats are
    // being used in the source.
    std::vector<int64_t> dtimeValues;
    std::string dtimeEpoch("");
    readSourceDtimeVar(fileGroup, this->commAll(), emptyFile, dtimeFormat,
                       dtimeValues, dtimeEpoch);

    // Convert the window start and end times to int64_t offsets from the dtimeEpoch
    // value. This will provide for a very fast "inside the timing window check".
    util::DateTime epochDt;
    convertEpochStringToDtime(dtimeEpoch, epochDt);
    const int64_t windowStart = (winStart_ - epochDt).toSeconds();
    const int64_t windowEnd = (winEnd_ - epochDt).toSeconds();

    // Determine which locations will be retained by this process for its obs space
    // source_loc_indices_ holds the original source location index (position in
    // the 1D Location variable) and recNums_ holds the assigned record number.
    std::vector<float> lonValues;
    std::vector<float> latValues;
    setIndexAndRecordNums(fileGroup, this->commAll(), emptyFile, distribution_, dtimeValues,
                          windowStart, windowEnd, applyLocationsCheck, obsGroupVarList_,
                          lonValues, latValues, sourceNlocs_,
                          sourceNlocsInsideTimeWindow_, sourceNlocsOutsideTimeWindow_,
                          sourceNlocsRejectQC_, locIndices_, recNums_,
                          globalNlocs_, nlocs_, nrecs_);

    // Check for consistency of the set of nlocs counts.
    ASSERT(sourceNlocs_ == sourceNlocsInsideTimeWindow_ + sourceNlocsOutsideTimeWindow_);
    ASSERT(sourceNlocs_ == globalNlocs_ + sourceNlocsOutsideTimeWindow_ + sourceNlocsRejectQC_);

    // For each pool member record the source location indices that each associated non-pool
    // member requires. Note that the rankGrouping structure contains the information about
    // which ranks are in the pool, and the non-pool ranks those pool ranks are associated with.
    setDistributionMap(*this, locIndices_, rankAssignment_, distributionMap_);

    oops::Log::debug() << "rankAssignment_ size: " << rankAssignment_.size() << std::endl;
    for (auto & rankAssign : rankAssignment_) {
        oops::Log::debug() << "rankAssignment_:     assigned rank (nlocs): "
                           << rankAssign.first << " (" << rankAssign.second << ")" << std::endl;
    }

    oops::Log::debug() << "distributionMap_ size: " << distributionMap_.size() << std::endl;
    for (auto & distMap : distributionMap_) {
        if (distMap.second.size() > 0) {
            oops::Log::debug() << "    rank: loc indices: " << distMap.first << ": "
                               << distMap.second[0] << "..."
                               << distMap.second[distMap.second.size()-1] << std::endl;
        } else {
            oops::Log::debug() << "    rank: loc indices empty: " << distMap.first << std::endl;
        }
    }
}

//--------------------------------------------------------------------------------------
void ReaderSinglePool::load(Group & destGroup) {
    std::unique_ptr<Engines::ReaderBase> readerEngine = nullptr;

    // Engine initialization
    if (this->commPool() != nullptr) {
        if (readerEngine != nullptr) {
            readerEngine->initialize();
        }
    }

    // Create the memory backend for the destGroup
    // TODO(srh) There needs to be a memory Engine structure created with ObsStore and
    // Hdf5Mem subclasses. Then call the corresponding factory function from here.
    Engines::BackendNames backendName = Engines::BackendNames::ObsStore;  // Hdf5Mem; ObsStore;
    Engines::BackendCreationParameters backendParams;
    // These parameters only matter if Hdf5Mem is the engine selected. ObsStore ignores.
    backendParams.action = Engines::BackendFileActions::Create;
    backendParams.createMode = Engines::BackendCreateModes::Truncate_If_Exists;
    backendParams.fileName = ioda::Engines::HH::genUniqueName();
    backendParams.allocBytes = 1024*1024*50;
    backendParams.flush = false;
    Group backend = constructBackend(backendName, backendParams);

    // Create the ObsGroup and attach the backend.
    destGroup = ObsGroup::generate(backend, {});

    // Mark the destGroup as empty for now, until we get the reader to actually
    // load date into the destGroup
    sourceNlocs_ = 0;

    // Engine finalization
    if (this->commPool() != nullptr) {
        if (readerEngine != nullptr) {
            readerEngine->finalize();
        }
    }
}

//--------------------------------------------------------------------------------------
void ReaderSinglePool::finalize() {
    oops::Log::trace() << "ReaderSinglePool::finalize, start" << std::endl;

    // At this point there are two split communicator groups: one for the io pool and the
    // other for the processes not included in the io pool.
    if (eckit::mpi::hasComm(poolCommName_)) {
        eckit::mpi::deleteComm(poolCommName_);
    }
    if (eckit::mpi::hasComm(nonPoolCommName_)) {
        eckit::mpi::deleteComm(nonPoolCommName_);
    }
    oops::Log::trace() << "ReaderSinglePool::finalize, end" << std::endl;
}

//--------------------------------------------------------------------------------------
void ReaderSinglePool::print(std::ostream & os) const {
  int poolSize = 0;
  if (this->commPool() != nullptr) {
    poolSize = this->commPool()->size();
  }
  os << readerSrc_ << " (io pool size: " << poolSize << ")";
}

}  // namespace ioda
