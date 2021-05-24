/*
 * (C) Copyright 2017-2019 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#include "ioda/Engines/Factory.h"

#include "ioda/io/ObsIoFileRead.h"

namespace ioda {

static ObsIoMaker<ObsIoFileRead> maker("FileRead");

//------------------------------ public functions --------------------------------
//--------------------------------------------------------------------------------
ObsIoFileRead::ObsIoFileRead(const Parameters_ & ioParams,
                             const ObsSpaceParameters & obsSpaceParams) : ObsIo() {
    Engines::BackendNames backendName;
    Engines::BackendCreationParameters backendParams;
    Group backend;

    std::string fileName = ioParams.fileName;
    oops::Log::trace() << "Constructing ObsIoFileRead: Opening file for read: "
                       << fileName << std::endl;

    // Open an hdf5 file, read only
    backendName = Engines::BackendNames::Hdf5File;
    if (ioParams.readFromSeparateFiles) {
        // We are initializing from a prior run and therefore reading in the
        // separate ioda files produced from that prior run.
        backendParams.fileName = uniquifyFileName(fileName, obsSpaceParams.getMpiRank(),
                                                  obsSpaceParams.getMpiTimeRank());
    } else {
        backendParams.fileName = fileName;
    }
    backendParams.action = Engines::BackendFileActions::Open;
    backendParams.openMode = Engines::BackendOpenModes::Read_Only;

    // Create the backend and attach it to an ObsGroup
    backend = constructBackend(backendName, backendParams);
    ObsGroup og(backend);
    obs_group_ = og;

    // Collect variable and dimension infomation for downstream use
    collectVarDimInfo(obs_group_, var_list_, dim_var_list_, dims_attached_to_vars_,
                      max_var_size_);

    // record number of locations
    nlocs_ = obs_group_.vars.open("nlocs").getDimensions().dimsCur[0];

    // record variables by which observations should be grouped into records
    obs_grouping_vars_ = ioParams.obsGrouping.value().obsGroupVars;
}

ObsIoFileRead::~ObsIoFileRead() {}

//------------------------------ private functions ----------------------------------
//-----------------------------------------------------------------------------------
void ObsIoFileRead::print(std::ostream & os) const {
    os << "ObsIoFileRead: " << std::endl;
}

}  // namespace ioda
