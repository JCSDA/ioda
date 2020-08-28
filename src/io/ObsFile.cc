/*
 * (C) Copyright 2017-2019 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#include "ioda/io/ObsFile.h"

#include "ioda/core/IodaUtils.h"
#include "ioda/Layout.h"

#include "oops/util/abor1_cpp.h"

////////////////////////////////////////////////////////////////////////
// Implementation of ObsIo for a file
////////////////////////////////////////////////////////////////////////

namespace ioda {

//--------------------------------------------------------------------------------
ObsFile::ObsFile(const ObsIoActions action, const ObsIoModes mode,
                 const ObsIoParameters & params) : ObsIo(action, mode, params) {
    std::string fileName;

    Engines::BackendNames backendName;
    Engines::BackendCreationParameters backendParams;
    Group backend;

    if (action == ObsIoActions::OPEN_FILE) {
        fileName = params.in_file_.fileName;
        oops::Log::trace() << "Constructing ObsFile: Opening file for read: "
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

        // record maximum frame size
        max_frame_size_ = params.in_file_.maxFrameSize;

        // fill in the variable list
        var_list_ = listAllVars(obs_group_, "");
    } else if (action == ObsIoActions::CREATE_FILE) {
        fileName = params.out_file_.fileName;
        oops::Log::trace() << "Constructing ObsFile: Creating file for write: "
                           << fileName << std::endl;
        backendName = Engines::BackendNames::Hdf5File;

        // Create an hdf5 file, and allow overwriting an existing file (for now)
        backendParams.fileName = fileName;
        backendParams.action = Engines::BackendFileActions::Create;
        backendParams.createMode = Engines::BackendCreateModes::Truncate_If_Exists;

        // Create the backend and attach it to an ObsGroup
        // Use the None DataLyoutPolicy for now to accommodate the current file format
        backend = constructBackend(backendName, backendParams);
        NewDimensionScales_t newDims;
        obs_group_ = ObsGroup::generate(backend, newDims,
            detail::DataLayoutPolicy::generate(detail::DataLayoutPolicy::Policies::None));

        // record maximum frame size
        max_frame_size_ = params.out_file_.maxFrameSize;
    } else {
        ABORT("ObsFile: Unrecongnized ObsIoActions value");
    }
}

ObsFile::~ObsFile() {}

//-----------------------------------------------------------------------------------
void ObsFile::print(std::ostream & os) const {
    os << "ObsFile: " << std::endl;
}

}  // namespace ioda
