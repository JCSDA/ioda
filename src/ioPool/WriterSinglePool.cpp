/*
 * (C) Copyright 2022 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/ioPool/WriterSinglePool.h"

#include <mpi.h>

#include <algorithm>
#include <cstdio>
#include <memory>
#include <numeric>
#include <sstream>
#include <utility>

#include "eckit/config/LocalConfiguration.h"

#include "ioda/Copying.h"
#include "ioda/Engines/EngineUtils.h"
#include "ioda/Exception.h"
#include "ioda/ioPool/WriterPoolFactory.h"
#include "ioda/ioPool/WriterPoolUtils.h"

#include "oops/util/Logger.h"

namespace ioda {
namespace IoPool {

// Io pool factory maker
static WriterPoolMaker<WriterSinglePool> makerWriterSinglePool("SinglePool");

//--------------------------------------------------------------------------------------
WriterSinglePool::WriterSinglePool(const IoPoolParameters & configParams,
                                   const WriterPoolCreationParameters & createParams)
                                       : WriterPoolBase(configParams, createParams) {
}

//--------------------------------------------------------------------------------------
void WriterSinglePool::initialize() {
    oops::Log::trace() << "WriterSinglePool::initialize, start" << std::endl;
    // Create and initialize the io pool.
    nlocs_ = patchObsVec().size();
    patchNlocs_ = std::count(patchObsVec().begin(), patchObsVec().end(), true);

    // Build the io pool. This call will set the pool size (how many MPI tasks belong to
    // the pool), assign the other MPI tasks to each of the pool tasks, and then split
    // the MPI commucator group to form the pool.
    buildIoPool(patchNlocs());

    // Calculate the total nlocs for each rank in the io pool.
    // This sets the total_nlocs_ data member and that holds the sum of
    // the nlocs from each rank (from comm_all_) that is assigned to this rank.
    // Use patch nlocs to get proper count after duplicate obs are removed.
    setTotalNlocs(patchNlocs());

    // Calculate the "global nlocs" which is the sum of total_nlocs_ from each rank
    // in the io pool. This is used to set the sizes of the variables (dimensioned
    // by nlocs) for the single file output. Also calculate the nlocs starting point
    // (offset) into the single file output for this rank.
    collectSingleFileInfo();

    // Set the isParallelIo_ flag. If a rank is not in the io pool, this gets set to
    // false, which is okay since the non io pool ranks do not use it.
    if (this->commPool() != nullptr) {
        isParallelIo_ = ((!configParams_.writeMultipleFiles) && (this->commPool()->size() > 1));
    } else {
        isParallelIo_ = false;
    }

    // Set the createMultipleFiles_ flag. If rank is not in the io pool, this gets
    // set to false which is okay since the non io pool ranks do not use it.
    if (this->commPool() != nullptr) {
        createMultipleFiles_ =
            ((configParams_.writeMultipleFiles) && (this->commPool()->size() > 1));
    } else {
        createMultipleFiles_ = false;
    }

    // Create an object of the writer pre-/post-processor here so that it can be
    // accessed throught the lifetime of the io pool object. The lifetime of the
    // writer engine is only during the save function. The writer pre-/post-processor
    // and writer engine classes are separated so that the pre-/post-processor steps
    // can manipulate files that the save command uses.
    if (this->commPool() != nullptr) {
        Engines::WriterCreationParameters createParams(*(this->commPool()), this->commTime(),
                                                       createMultipleFiles_, isParallelIo_);
        writer_proc_ = Engines::WriterProcFactory::create(writerParams_, createParams);
        if (!writer_proc_->backendCanUseParallelIO()) isParallelIo_ = false;
    }
    oops::Log::trace() << "WriterSinglePool::initialize, end" << std::endl;
}

//--------------------------------------------------------------------------------------
void WriterSinglePool::save(const Group & srcGroup) {
    oops::Log::trace() << "WriterSinglePool::save, start" << std::endl;
    Group fileGroup;
    std::unique_ptr<Engines::WriterBase> writerEngine = nullptr;
    if (this->commPool() != nullptr) {
        Engines::WriterCreationParameters createParams(*(this->commPool()), this->commTime(),
                                                       createMultipleFiles_, isParallelIo_);
        writerEngine = Engines::WriterFactory::create(writerParams_, createParams);

        fileGroup = writerEngine->getObsGroup();

        // Engine initialization
        writerEngine->initialize();

        // collect the destination from the writer engine instance
        std::ostringstream ss;
        ss << *writerEngine;
        writerDest_ = ss.str();
    }

    // Copy the ObsSpace ObsGroup to the output file Group.
    ioWriteGroup(*this, srcGroup, fileGroup, isParallelIo_);

    // Engine finalization
    if (this->commPool() != nullptr) {
        writerEngine->finalize();
    }
    oops::Log::trace() << "WriterSinglePool::save, end" << std::endl;
}

//--------------------------------------------------------------------------------------
void WriterSinglePool::finalize() {
    oops::Log::trace() << "WriterSinglePool::finalize, start" << std::endl;
    // Call the post processor associated with the backend engine being
    // used in the save function.
    if (this->commPool() != nullptr) {
        writer_proc_->post();
    }

    // At this point there are two split communicator groups: one for the io pool and the
    // other for the processes not included in the io pool.
    if (eckit::mpi::hasComm(poolCommName_)) {
        eckit::mpi::deleteComm(poolCommName_);
    }
    if (eckit::mpi::hasComm(nonPoolCommName_)) {
        eckit::mpi::deleteComm(nonPoolCommName_);
    }
    oops::Log::trace() << "WriterSinglePool::finalize, end" << std::endl;
}

//--------------------------------------------------------------------------------------
void WriterSinglePool::print(std::ostream & os) const {
  int poolSize = 0;
  if (this->commPool() != nullptr) {
    poolSize = this->commPool()->size();
  }
  os << writerDest_ << " (io pool size: " << poolSize << ")";
}

}  // namespace IoPool
}  // namespace ioda
