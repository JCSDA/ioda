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

ReadH5File::ReadH5File(const Parameters_ & params,
                       const ReaderCreationParameters & createParams)
                           : ReaderBase(createParams), fileName_(params.fileName) {
    oops::Log::trace() << "ioda::Engines::ReadH5File start constructor" << std::endl;
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
