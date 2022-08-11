/*
 * (C) Copyright 2017-2019 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#include "eckit/config/YAMLConfiguration.h"

#include "ioda/core/FileFormat.h"
#include "ioda/Engines/Factory.h"
#include "ioda/Engines/HH.h"  // for genUniqueName()
#include "ioda/Engines/ODC.h"
#include "ioda/Misc/IoPoolUtils.h"
#include "ioda/io/ObsIoFileRead.h"

namespace ioda {

//--------------------------------------------------------------------------------
static ObsIoMaker<ObsIoFileRead> maker("FileRead");

//------------------------------ public functions --------------------------------
//--------------------------------------------------------------------------------
ObsIoFileRead::ObsIoFileRead(const Parameters_ & ioParams,
                             const ObsSpaceParameters & obsSpaceParams) : ObsIo() {
    std::string fileName = ioParams.fileName;

    oops::Log::trace() << "Constructing ObsIoFileRead: Opening file for read: "
                       << fileName << std::endl;

    const bool odb = (determineFileFormat(fileName, ioParams.format) == FileFormat::ODB);

    if (odb)
      createObsGroupFromOdbFile(fileName, ioParams);
    else
      createObsGroupFromHdf5File(fileName);

    // Collect variable and dimension infomation for downstream use
    VarUtils::collectVarDimInfo(obs_group_, var_list_, dim_var_list_, dims_attached_to_vars_,
                                max_var_size_);

    // record number of locations
    nlocs_ = obs_group_.vars.open("nlocs").getDimensions().dimsCur[0];
    if (nlocs_ == 0) {
      oops::Log::info() << "WARNING: Input file " << fileName << " contains zero observations"
          << std::endl;
    }

    // record variables by which observations should be grouped into records
    obs_grouping_vars_ = ioParams.obsGrouping.value().obsGroupVars;
}

ObsIoFileRead::~ObsIoFileRead() {}

//------------------------------ private functions ----------------------------------
//-----------------------------------------------------------------------------------
void ObsIoFileRead::print(std::ostream & os) const {
    os << "ObsIoFileRead: " << std::endl;
}

void ObsIoFileRead::createObsGroupFromHdf5File(const std::string & fileName) {
    // Prepare to create a backend backed by an existing read-only hdf5 file
    Engines::BackendNames backendName = Engines::BackendNames::Hdf5File;
    Engines::BackendCreationParameters backendParams;
    backendParams.fileName = fileName;
    backendParams.action = Engines::BackendFileActions::Open;
    backendParams.openMode = Engines::BackendOpenModes::Read_Only;

    // Create the backend and attach it to an ObsGroup
    Group backend = constructBackend(backendName, backendParams);
    obs_group_ = ObsGroup(backend);
}

void ObsIoFileRead::createObsGroupFromOdbFile(const std::string & fileName,
                                              const Parameters_ & ioParams) {
    if (ioParams.mappingFile.value().empty())
        throw ioda::Exception("The 'obsdatain.mapping file' option "
                              "must be set for obs files in the ODB format.", ioda_Here());
    if (ioParams.queryFile.value().empty())
        throw ioda::Exception("The 'obsdatain.query file' option "
                              "must be set for obs files in the ODB format.", ioda_Here());

    // Create an in-memory backend
    Engines::BackendNames backendName = Engines::BackendNames::ObsStore;
    Engines::BackendCreationParameters backendParams;
    backendParams.fileName = ioda::Engines::HH::genUniqueName();
    backendParams.action = Engines::BackendFileActions::Create;
    backendParams.createMode = Engines::BackendCreateModes::Truncate_If_Exists;
    backendParams.allocBytes = 1024*1024*50;
    backendParams.flush = false;
    Group backend = constructBackend(backendName, backendParams);

    // And load the ODB file into it
    Engines::ODC::ODC_Parameters odcparams;
    odcparams.filename    = fileName;
    odcparams.mappingFile = ioParams.mappingFile;
    odcparams.queryFile   = ioParams.queryFile;
    odcparams.maxNumberChannels = ioParams.maxNumberChannels;

    obs_group_ = Engines::ODC::openFile(odcparams, backend);
}

}  // namespace ioda
