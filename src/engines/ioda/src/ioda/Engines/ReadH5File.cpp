/*
 * (C) Copyright 2022 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#include "oops/util/Logger.h"

#include "ioda/Engines/ReadH5File.h"
#include "ioda/Exception.h"

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
    if (haveFileReadAccess(fileName_)) {
        // Input file exists and is readable
        // Create a backend backed by an existing read-only hdf5 file
        Engines::BackendNames backendName = BackendNames::Hdf5File;
        Engines::BackendCreationParameters backendParams;
        backendParams.fileName = fileName_;
        backendParams.action = BackendFileActions::Open;
        backendParams.openMode = BackendOpenModes::Read_Only;

        Group backend = constructBackend(backendName, backendParams);
        obs_group_ = ObsGroup(backend);
        oops::Log::trace() << "ioda::Engines::ReadH5File end constructor" << std::endl;
    } else {
        // Input file does not exist (is not readable actually)
        if (params.missingFileAction.value() == "warn") {
            oops::Log::info() << "WARNING: input file is not readable, "
               << "will continue with empty file representation" << std::endl
               << "WARNING:     file: " << fileName_ << std::endl;
            // Create a memory backend as a placehold for the missing file. Make the
            // memory backend look like an empty file (Location == 0).
            Engines::BackendNames backendName = Engines::BackendNames::ObsStore;
            Engines::BackendCreationParameters backendParams;
            Group backend = constructBackend(backendName, backendParams);

            // Create the ObsGroup and attach the backend.
            obs_group_ = ObsGroup::generate(backend, {});

            // Create the Location dimension and set its size to zero.
            obs_group_.vars.create<int64_t>("Location", { 0 })
                .setIsDimensionScale("Location");
        } else if (params.missingFileAction.value() == "error") {
            std::string ErrMsg = std::string("Input file is not readable, ") +
                std::string("will stop execution. File: ") + fileName_ + std::string("\n");
            throw Exception(ErrMsg, ioda_Here());
        } else {
            std::string ErrMsg = std::string("Unrecognized input file missing action: ") +
                params.missingFileAction.value();
            throw Exception(ErrMsg, ioda_Here());
        }
    }
}

std::string ReadH5File::fileName() const {
   return fileName_;
}

void ReadH5File::print(std::ostream & os) const {
  os << fileName_;
}

}  // namespace Engines
}  // namespace ioda
