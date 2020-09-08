/*
 * (C) Copyright 2017-2019 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#include "ioda/io/ObsFile.h"

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
ObsFile::ObsFile(const ObsIoActions action, const ObsIoModes mode,
                 const ObsSpaceParameters & params) : ObsIo(action, mode, params) {
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

        // record maximum variable and frame size
        max_var_size_ = maxVarSize0(obs_group_);
        max_frame_size_ = params.in_file_.maxFrameSize;
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
        obs_group_ = ObsGroup::generate(backend, params.getDimScales(),
            detail::DataLayoutPolicy::generate(detail::DataLayoutPolicy::Policies::None));

        // record maximum variable and frame size
        max_var_size_ = params.getMaxVarSize();
        max_frame_size_ = params.out_file_.maxFrameSize;
    } else {
        ABORT("ObsFile: Unrecongnized ObsIoActions value");
    }
}

ObsFile::~ObsFile() {}

// -----------------------------------------------------------------------------
void ObsFile::genFrameIndexRecNums(std::shared_ptr<Distribution> & dist) {
    // Generate location indices relative to the obs source (locIndex) and relative
    // to the current frame (frameIndex).
    //
    // Apply the timing window. Need to filter out locations that are outside the
    // timing window before generating record numbers. This is because
    // we are generating record numbers on the fly since we want to get to the point where
    // we can do the MPI distribution without knowing how many obs (and records) we are going
    // to encounter.
    std::vector<Dimensions_t> locIndex;
    std::vector<Dimensions_t> frameIndex;
    genFrameLocationsTimeWindow(locIndex, frameIndex);

    // Generate record numbers for this frame. Consider obs grouping.
    std::vector<Dimensions_t> records;
    std::string obsGroupVarName = params_.in_file_.obsGroupVar;
    if (obsGroupVarName.empty()) {
         genRecordNumbersAll(locIndex, records);
    } else {
        genRecordNumbersGrouping(obsGroupVarName, frameIndex, records);
    }

    // Apply the MPI distribution to the records
    applyMpiDistribution(dist, locIndex, records);

    // New frame count is the number of entries in the frame_loc_index_ vector
    // This will be handed to callers through the frameCount function for all
    // variables with nlocs as their first dimension.
    adjusted_nlocs_frame_count_ = frame_loc_index_.size();
}

//------------------------------ private functions ----------------------------------
//-----------------------------------------------------------------------------------
void ObsFile::print(std::ostream & os) const {
    os << "ObsFile: " << std::endl;
}

}  // namespace ioda
