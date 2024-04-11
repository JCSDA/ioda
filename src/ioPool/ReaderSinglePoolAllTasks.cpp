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
    const IoPoolParameters & configParams, const ReaderPoolCreationParameters & createParams)
        : ReaderPoolBase(configParams, createParams) {
}

//--------------------------------------------------------------------------------------
void ReaderSinglePoolAllTasks::initialize() {
    oops::Log::trace() << "ReaderSinglePoolAllTasks::initialize, start" << std::endl;
    // TODO(srh) Until the actual reader pool is implemented we need to copy the
    // commAll_ communicator to the commPool_ communicator. Two of the IoPoolBase
    // virtual functions are being overridden here (setTargetPoolSize and groupRanks)
    // to accomplish this.
    buildIoPool(this->nlocs());
    oops::Log::trace() << "ReaderSinglePoolAllTasks::initialize, end" << std::endl;
}

//--------------------------------------------------------------------------------------
void ReaderSinglePoolAllTasks::load(Group & destGroup) {
    oops::Log::trace() << "ReaderSinglePoolAllTasks::load, start" << std::endl;
    Group fileGroup;
    Engines::ReaderCreationParameters
        createParams(timeWindow_, *commPool_, commTime_,
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

    // Extract and record global data from the input file. Call the extract utility from
    // only rank 0, and then broadcast the data to the other ranks. This saves a bit on
    // file IO.
    bool emptyFile;
    DateTimeFormat dtimeFormat;
    std::string dtimeEpoch("");
    std::vector<int64_t> dtimeValues;
    std::vector<float> lonValues;
    std::vector<float> latValues;
    std::vector<std::size_t> sourceLocIndices;
    std::vector<std::size_t> sourceRecNums;
    // Rank 0 does the preliminary checking and formation of the source location
    // indices and source record numbers. These are identical operations on each
    // MPI task so file io can be reduced by having rank 0 only do the io, generate
    // the indices and record numbers and broadcast that information to the other
    // ranks.
    extractGlobalInfoFromSource(this->commAll(), fileGroup, readerSrc_, timeWindow_,
       readerEngine->applyLocationsCheck(), obsGroupVarList_, dtimeValues,
       lonValues, latValues, sourceLocIndices, sourceRecNums, emptyFile, dtimeFormat,
       dtimeEpoch, globalNlocs_, sourceNlocs_, sourceNlocsInsideTimeWindow_,
       sourceNlocsOutsideTimeWindow_, sourceNlocsRejectQC_);

    // Calculate the MPI routing in a collective fashion using the
    // entire commAll communicator group.
    applyMpiDistribution(distribution_, emptyFile, lonValues, latValues, sourceLocIndices,
                         sourceRecNums, locIndices_, recNums_, nlocs_, nrecs_);

    // Check for consistency of the set of nlocs counts.
    ASSERT(sourceNlocs_ == sourceNlocsInsideTimeWindow_ + sourceNlocsOutsideTimeWindow_);
    ASSERT(sourceNlocs_ == globalNlocs_ + sourceNlocsOutsideTimeWindow_ + sourceNlocsRejectQC_);

    // Create the memory backend for the destGroup
    // TODO(srh) There needs to be a memory Engine structure created with ObsStore and
    // Hdf5Mem subclasses. Then call the corresponding factory function from here.
    Engines::BackendNames backendName = Engines::BackendNames::ObsStore;
    Engines::BackendCreationParameters backendParams;
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
void ReaderSinglePoolAllTasks::setTargetPoolSize(const int numMpiTasks) {
    // TODO(srh) For now this reader is placing all tasks into the pool. This means
    // that the desired number of pool member tasks is simply the size of the
    // commAll communicator group.
    targetPoolSize_ = numMpiTasks;
}

//--------------------------------------------------------------------------------------
void ReaderSinglePoolAllTasks::groupRanks(const int numMpiTasks,
                                          IoPoolGroupMap & rankGrouping) {
    // TODO(srh) Until the actual reader pool is implemented we need to copy the
    // commAll_ communicator to the commPool_ communicator. This can be accomplished
    // by constructing the rankGrouping map with each commAll_ rank assigned only
    // to itself. Note need to assign empty vector to each element of rankGrouping
    // since these vectors represent the other ranks associated with each pool member.
    // Ie, all ranks are in the pool, and each rank has no associated ranks.
    rankGrouping.clear();
    for (std::size_t i = 0; i < numMpiTasks; ++i) {
        rankGrouping[i] = std::vector<int>(0);
    }
}

}  // namespace IoPool
}  // namespace ioda
