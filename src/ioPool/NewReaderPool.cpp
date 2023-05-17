/*
 * (C) Copyright 2022 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/ioPool/NewReaderPool.h"

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
#include "ioda/ioPool/ReaderUtils.h"

#include "oops/util/Logger.h"
#include "oops/util/missingValues.h"

namespace ioda {

// For the MPI communicator splitting
constexpr int readerPoolColor = 3;
constexpr int readerNonPoolColor = 4;
const char readerPoolCommName[] = "readerIoPool";
const char readerNonPoolCommName[] = "readerNonIoPool";

//--------------------------------------------------------------------------------------
void NewReaderPool::groupRanks(IoPoolGroupMap & rankGrouping) {
    // TODO(srh) Until the actual reader pool is implemented we need to copy the
    // comm_all_ communicator to the comm_pool_ communicator. This can be accomplished
    // by constructing the rankGrouping map with each comm_all_ rank assigned only
    // to itself.
    rankGrouping.clear();
    for (std::size_t i = 0; i < comm_all_.size(); ++i) {
        rankGrouping[i] = std::move(std::vector<int>(1, i));
    }
}

//--------------------------------------------------------------------------------------
NewReaderPool::NewReaderPool(const oops::Parameter<IoPoolParameters> & ioPoolParams,
               const oops::RequiredPolymorphicParameter
                   <Engines::ReaderParametersBase, Engines::ReaderFactory> & readerParams,
               const eckit::mpi::Comm & commAll, const eckit::mpi::Comm & commTime,
               const util::DateTime & winStart, const util::DateTime & winEnd,
               const std::vector<std::string> & obsVarNames,
               const std::shared_ptr<Distribution> & distribution,
               const std::vector<std::string> & obsGroupVarList)
                   : IoPoolBase(ioPoolParams, commAll, commTime, winStart, winEnd,
                     readerPoolColor, readerNonPoolColor,
                     readerPoolCommName, readerNonPoolCommName),
                     reader_params_(readerParams) {
    readerSrc_ = "New Reader (under development)";
}

NewReaderPool::~NewReaderPool() = default;

//--------------------------------------------------------------------------------------
void NewReaderPool::initialize() {
}

//--------------------------------------------------------------------------------------
void NewReaderPool::load(Group & destGroup) {
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
    source_nlocs_ = 0;
}

//--------------------------------------------------------------------------------------
void NewReaderPool::finalize() {
    oops::Log::trace() << "NewReaderPool::finalize, start" << std::endl;

    // At this point there are two split communicator groups: one for the io pool and the
    // other for the processes not included in the io pool.
    if (eckit::mpi::hasComm(poolCommName_)) {
        eckit::mpi::deleteComm(poolCommName_);
    }
    if (eckit::mpi::hasComm(nonPoolCommName_)) {
        eckit::mpi::deleteComm(nonPoolCommName_);
    }
    oops::Log::trace() << "NewReaderPool::finalize, end" << std::endl;
}

//--------------------------------------------------------------------------------------
void NewReaderPool::print(std::ostream & os) const {
  os << readerSrc_ << " (io pool size: " << size_pool_ << ")";
}

}  // namespace ioda
