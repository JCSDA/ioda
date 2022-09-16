/*
 * (C) Copyright 2022 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#include "oops/util/Logger.h"

#include "ioda/Engines/ReadH5File.h"

namespace ioda {
namespace Engines {

//---------------------------------------------------------------------
// ReadH5File
//---------------------------------------------------------------------

static ReaderMaker<ReadH5File> maker("H5File");

// Parameters

// Classes

ReadH5File::ReadH5File(const Parameters_ & params, const util::DateTime & winStart,
                       const util::DateTime & winEnd, const eckit::mpi::Comm & comm,
                       const eckit::mpi::Comm & timeComm,
                       const std::vector<std::string> & obsVarNames)
                           : ReaderBase(winStart, winEnd, comm, timeComm, obsVarNames),
                             fileName_(params.fileName) {
    oops::Log::trace() << "ioda::Engines::ReadH5File start constructor" << std::endl;
    // Record the file name for reporting
    fileName_ = params.fileName;

    // Create a backend backed by an existing read-only hdf5 file
    Engines::BackendNames backendName = BackendNames::Hdf5File;
    Engines::BackendCreationParameters backendParams;
    backendParams.fileName = fileName_;
    backendParams.action = BackendFileActions::Open;
    backendParams.openMode = BackendOpenModes::Read_Only;

    Group backend = constructBackend(backendName, backendParams);
    obs_group_ = ObsGroup(backend);
    oops::Log::trace() << "ioda::Engines::ReadH5File end constructor" << std::endl;
}

void ReadH5File::print(std::ostream & os) const {
  os << fileName_;
}

}  // namespace Engines
}  // namespace ioda
