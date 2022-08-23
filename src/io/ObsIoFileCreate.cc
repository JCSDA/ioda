/*
 * (C) Copyright 2017-2019 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#include "ioda/Engines/EngineUtils.h"
#include "ioda/Misc/IoPoolUtils.h"
#include "ioda/io/ObsIoFileCreate.h"

namespace ioda {

static ObsIoMaker<ObsIoFileCreate> maker("FileCreate");

//------------------------------ public functions --------------------------------
//--------------------------------------------------------------------------------
ObsIoFileCreate::ObsIoFileCreate(const Parameters_ & ioParams,
                                 const ObsSpaceParameters & obsSpaceParams)
  : ObsIo() {
    Engines::BackendNames backendName;
    Engines::BackendCreationParameters backendParams;
    Group backend;

    std::string fileName = ioParams.fileName;
    oops::Log::trace() << "Constructing ObsIoFileCreate: Creating file for write: "
                       << fileName << std::endl;
    backendName = Engines::BackendNames::Hdf5File;

    // Create an hdf5 file, and allow overwriting an existing file (for now)
    // Tag on the rank number to the output file name to avoid collisions if running
    // with multiple MPI tasks.
    backendParams.fileName =
      uniquifyFileName(fileName, obsSpaceParams.getMpiRank(), obsSpaceParams.getMpiTimeRank());
    backendParams.action = Engines::BackendFileActions::Create;
    backendParams.createMode = Engines::BackendCreateModes::Truncate_If_Exists;

    // Create the backend and attach it to an ObsGroup
    // Use the None DataLyoutPolicy for now to accommodate the current file format
    backend = constructBackend(backendName, backendParams);
    obs_group_ = ObsGroup::generate(backend, obsSpaceParams.getDimScales());

    // record maximum variable size
    max_var_size_ = obsSpaceParams.getMaxVarSize();

    // record number of locations
    nlocs_ = obs_group_.vars.open("nlocs").getDimensions().dimsCur[0];
}

ObsIoFileCreate::~ObsIoFileCreate() {}

//------------------------------ private functions ----------------------------------
//-----------------------------------------------------------------------------------
void ObsIoFileCreate::print(std::ostream & os) const {
    os << "ObsIoFileCreate: " << std::endl;
}

}  // namespace ioda
