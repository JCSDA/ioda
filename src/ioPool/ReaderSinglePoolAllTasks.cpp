/*
 * (C) Copyright 2022 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/ioPool/ReaderSinglePoolAllTasks.h"

#include <mpi.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <memory>
#include <numeric>
#include <sstream>
#include <utility>

#include "eckit/config/LocalConfiguration.h"
#include "eckit/exception/Exceptions.h"
#include "eckit/mpi/Comm.h"
#include "eckit/mpi/DataType.h"

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
namespace IoPool {

// Io pool factory maker
static ReaderPoolMaker<ReaderSinglePoolAllTasks>
    makerReaderSinglePoolAllTasks("SinglePoolAllTasks");

//--------------------------------------------------------------------------------------
ReaderSinglePoolAllTasks::ReaderSinglePoolAllTasks(
                            const IoPoolParameters & configParams,
                            const ReaderPoolCreationParameters & createParams)
                   : ReaderPoolBase(configParams, createParams) {
    // Save a persistent copy of the JEDI missing value for a string variable that can
    // be used to properly replace a string fill value from the obs source with this
    // JEDI missing value. The replaceFillWithMissing function needs a char * pointing
    // to this copy of the JEDI missing value to transfer that value to the obs space
    // container.
    stringMissingValue_ = std::make_shared<std::string>(util::missingValue<std::string>());
}

//--------------------------------------------------------------------------------------
void ReaderSinglePoolAllTasks::initialize() {
    oops::Log::trace() << "ReaderSinglePoolAllTasks::initialize, start" << std::endl;
    // TODO(srh) Until the actual reader pool is implemented we need to copy the
    // commAll_ communicator to the commPool_ communicator. The following
    // calls will fall into place for the io pool so use them now to accomplish the
    // copy.

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
    assignRanksToIoPool(nlocs(), rankGrouping);

    // Create the io pool communicator group using the split communicator command.
    createIoPool(rankGrouping);
    oops::Log::trace() << "ReaderSinglePoolAllTasks::initialize, end" << std::endl;
}

//--------------------------------------------------------------------------------------
void ReaderSinglePoolAllTasks::load(Group & destGroup) {
    oops::Log::trace() << "ReaderSinglePoolAllTasks::load, start" << std::endl;
    Group fileGroup;
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
                          windowStart, windowEnd,
                          readerEngine->applyLocationsCheck(), obsGroupVarList_,
                          lonValues, latValues, sourceNlocs_,
                          sourceNlocsInsideTimeWindow_, sourceNlocsOutsideTimeWindow_,
                          sourceNlocsRejectQC_, locIndices_, recNums_,
                          globalNlocs_, nlocs_, nrecs_);

    // Check for consistency of the set of nlocs counts.
    ASSERT(sourceNlocs_ == sourceNlocsInsideTimeWindow_ + sourceNlocsOutsideTimeWindow_);
    ASSERT(sourceNlocs_ == globalNlocs_ + sourceNlocsOutsideTimeWindow_ + sourceNlocsRejectQC_);

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

    // Copy the ObsSpace ObsGroup to the output file Group.
    ioReadGroup(*this, fileGroup, destGroup, dtimeFormat, dtimeValues, dtimeEpoch,
                lonValues, latValues, isParallelIo_, emptyFile);

    // Engine finalization
    readerEngine->finalize();
    oops::Log::trace() << "ReaderSinglePoolAllTasks::load, end" << std::endl;
}

//--------------------------------------------------------------------------------------
void ReaderSinglePoolAllTasks::finalize() {
    oops::Log::trace() << "ReaderSinglePoolAllTasks::finalize, start" << std::endl;

    // At this point there are two split communicator groups: one for the io pool and the
    // other for the processes not included in the io pool.
    if (eckit::mpi::hasComm(poolCommName_)) {
        eckit::mpi::deleteComm(poolCommName_);
    }
    if (eckit::mpi::hasComm(nonPoolCommName_)) {
        eckit::mpi::deleteComm(nonPoolCommName_);
    }
    oops::Log::trace() << "ReaderSinglePoolAllTasks::finalize, end" << std::endl;
}

//--------------------------------------------------------------------------------------
void ReaderSinglePoolAllTasks::print(std::ostream & os) const {
  int poolSize = 0;
  if (this->commPool() != nullptr) {
    poolSize = this->commPool()->size();
  }
  os << readerSrc_ << " (io pool size: " << poolSize << ")";
}

//--------------------------------------------------------------------------------------
void ReaderSinglePoolAllTasks::groupRanks(IoPoolGroupMap & rankGrouping) {
    // TODO(srh) Until the actual reader pool is implemented we need to copy the
    // commAll_ communicator to the commPool_ communicator. This can be accomplished
    // by constructing the rankGrouping map with each commAll_ rank assigned only
    // to itself.
    rankGrouping.clear();
    for (std::size_t i = 0; i < commAll_.size(); ++i) {
        rankGrouping[i] = std::vector<int>(1, i);
    }
}

}  // namespace IoPool
}  // namespace ioda
