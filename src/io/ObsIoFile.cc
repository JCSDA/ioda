/*
 * (C) Copyright 2017-2019 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#include "ioda/io/ObsIoFile.h"

#include <typeindex>
#include <typeinfo>

#include "ioda/core/IodaUtils.h"
#include "ioda/Layout.h"

#include "oops/util/abor1_cpp.h"

////////////////////////////////////////////////////////////////////////
// Implementation of ObsIo for a file
////////////////////////////////////////////////////////////////////////

namespace ioda {

//------------------------------ public functions --------------------------------
//--------------------------------------------------------------------------------
ObsIoFile::ObsIoFile(const ObsIoActions action, const ObsIoModes mode,
                     const ObsSpaceParameters & params) : ObsIo(action, mode, params) {
    std::string fileName;

    Engines::BackendNames backendName;
    Engines::BackendCreationParameters backendParams;
    Group backend;

    if (action == ObsIoActions::OPEN_FILE) {
        fileName = params.top_level_.obsInFile.value()->fileName;
        oops::Log::trace() << "Constructing ObsIoFile: Opening file for read: "
                           << fileName << std::endl;

        // Open an hdf5 file, read only
        backendName = Engines::BackendNames::Hdf5File;
        backendParams.fileName = fileName;
        backendParams.action = Engines::BackendFileActions::Open;
        backendParams.openMode = Engines::BackendOpenModes::Read_Only;

        // Create the backend and attach it to an ObsGroup
        // Use the None DataLyoutPolicy for now to accommodate the current file format
        backend = constructBackend(backendName, backendParams);
        ObsGroup og(backend,
            detail::DataLayoutPolicy::generate(detail::DataLayoutPolicy::Policies::None));
        obs_group_ = og;

        // record maximum variable size
        max_var_size_ = maxVarSize0(obs_group_);

        // record lists of regular variables and dimension scale variables
        this->resetVarList();
        this->resetDimVarList();
        this->resetVarDimMap();
    } else if (action == ObsIoActions::CREATE_FILE) {
        fileName = params.top_level_.obsOutFile.value()->fileName;
        oops::Log::trace() << "Constructing ObsIoFile: Creating file for write: "
                           << fileName << std::endl;
        backendName = Engines::BackendNames::Hdf5File;

        // Create an hdf5 file, and allow overwriting an existing file (for now)
        // Tag on the rank number to the output file name to avoid collisions if running
        // with multiple MPI tasks.
        backendParams.fileName = uniquifyFileName(fileName, params.getMpiRank());
        backendParams.action = Engines::BackendFileActions::Create;
        backendParams.createMode = Engines::BackendCreateModes::Truncate_If_Exists;

        // Create the backend and attach it to an ObsGroup
        // Use the None DataLyoutPolicy for now to accommodate the current file format
        backend = constructBackend(backendName, backendParams);
        obs_group_ = ObsGroup::generate(backend, params.getDimScales(),
            detail::DataLayoutPolicy::generate(detail::DataLayoutPolicy::Policies::None));

        // record maximum variable size
        max_var_size_ = params.getMaxVarSize();
    } else {
        ABORT("ObsIoFile: Unrecongnized ObsIoActions value");
    }

    // record number of locations
    nlocs_ = obs_group_.vars.open("nlocs").getDimensions().dimsCur[0];
}

ObsIoFile::~ObsIoFile() {}

//------------------------------ private functions ----------------------------------
//-----------------------------------------------------------------------------------
void ObsIoFile::print(std::ostream & os) const {
    os << "ObsIoFile: " << std::endl;
}

}  // namespace ioda
