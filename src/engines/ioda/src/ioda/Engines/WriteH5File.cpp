/*
 * (C) Copyright 2022 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#include "ioda/Engines/WriteH5File.h"

#include "eckit/mpi/Parallel.h"

#include "ioda/Io/IoPoolUtils.h"

#include "oops/util/Logger.h"

namespace ioda {
namespace Engines {

//---------------------------------------------------------------------
// WriteH5File
//---------------------------------------------------------------------

static WriterMaker<WriteH5File> maker("H5File");

// Parameters

// Classes

WriteH5File::WriteH5File(const Parameters_ & params,
                         const WriterCreationParameters & createParams)
                             : WriterBase(createParams), params_(params) {
    oops::Log::trace() << "ioda::Engines::WriteH5File start constructor" << std::endl;
    // Create a backend to write to a new hdf5 file. If the allowOverwrite parameter is
    // true, then it is okay to clobber an existing file.
    Engines::BackendNames backendName = Engines::BackendNames::Hdf5File;

    // Figure out the output file name.
    std::string outFileName;
    std::size_t mpiRank = createParams_.comm.rank();
    int mpiTimeRank = -1; // a value of -1 tells uniquifyFileName to skip this value
    if (createParams_.timeComm.size() > 1) {
        mpiTimeRank = createParams_.timeComm.rank();
    }
    if (createParams_.createMultipleFiles) {
        // Tag on the rank number to the output file name to avoid collisions.
        // Don't use the time communicator rank number in the suffix if the size of
        // the time communicator is 1.
        outFileName = uniquifyFileName(params_.fileName, mpiRank, mpiTimeRank);
    } else {
        // TODO(srh) With the upcoming release (Sep 2022) we need to keep the uniquified
        // file name in order to prevent thrasing downstream tools. We can get rid of
        // the file suffix after the release.
        // If we got to here, we either have just one process in io pool, or we
        // are going to write out the file in parallel mode. In either case, we want
        // the suffix part related to mpiRank to always be zero.
        outFileName = uniquifyFileName(params_.fileName, 0, mpiTimeRank);
    }

    Engines::BackendCreationParameters backendParams;
    backendParams.fileName = outFileName;
    if (createParams_.isParallelIo) {
        backendParams.action = Engines::BackendFileActions::CreateParallel;
        backendParams.comm = 
            dynamic_cast<const eckit::mpi::Parallel &>(createParams_.comm).MPIComm();
    } else {
        backendParams.action = Engines::BackendFileActions::Create;
    }
    if (params.allowOverwrite) {
        backendParams.createMode = Engines::BackendCreateModes::Truncate_If_Exists;
    } else {
        backendParams.createMode = Engines::BackendCreateModes::Fail_If_Exists;
    }

    Group backend = constructBackend(backendName, backendParams);
    obs_group_ = ObsGroup(backend);
    oops::Log::trace() << "ioda::Engines::WriteH5File end constructor" << std::endl;
}

void WriteH5File::print(std::ostream & os) const {
  os << params_.fileName.value();
}

}  // namespace Engines
}  // namespace ioda
