/*
 * (C) Copyright 2022 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#include "ioda/Engines/WriteH5File.h"

#include "ioda/Misc/IoPoolUtils.h"

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
                         const util::DateTime & winStart,
                         const util::DateTime & winEnd,
                         const eckit::mpi::Comm & comm,
                         const eckit::mpi::Comm & timeComm,
                         const std::vector<std::string> & obsVarNames)
                             : WriterBase(winStart, winEnd, comm, timeComm, obsVarNames),
                               fileName_(params.fileName) {
    oops::Log::trace() << "ioda::Engines::WriteH5File start constructor" << std::endl;
    // Create a backend to write to a new hdf5 file. If the allowOverwrite parameter is
    // true, then it is okay to clobber an existing file.
    Engines::BackendNames backendName = Engines::BackendNames::Hdf5File;

    // Tag on the rank number to the output file name to avoid collisions if running
    // with multiple MPI tasks. Don't use the time communicator rank number in the
    // suffix if the size of the time communicator is 1.
    std::size_t mpiRank = comm_.rank();
    int mpiTimeRank = -1; // a value of -1 tells uniquifyFileName to skip this value
    if (timeComm_.size() > 1) {
        mpiTimeRank = timeComm_.rank();
    }
    Engines::BackendCreationParameters backendParams;
    std::string fileName = params.fileName;
    backendParams.fileName = uniquifyFileName(fileName, mpiRank, mpiTimeRank);
    backendParams.action = Engines::BackendFileActions::Create;
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
  os << fileName_;
}

}  // namespace Engines
}  // namespace ioda
