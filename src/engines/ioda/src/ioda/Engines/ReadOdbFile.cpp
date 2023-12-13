/*
 * (C) Copyright 2022 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#include "oops/util/Logger.h"
#include "oops/util/missingValues.h"

#include "ioda/Engines/ReadOdbFile.h"

namespace ioda {
namespace Engines {

//---------------------------------------------------------------------
// ReadOdbFile
//---------------------------------------------------------------------

static ReaderMaker<ReadOdbFile> maker("ODB");

// Parameters

// Classes

ReadOdbFile::ReadOdbFile(const Parameters_ & params,
                         const ReaderCreationParameters & createParams)
                             : ReaderBase(createParams), fileName_(params.fileName) {
    oops::Log::trace() << "ioda::Engines::ReadOdbFile start constructor" << std::endl;
    // Create an in-memory backend
    Engines::BackendNames backendName = Engines::BackendNames::ObsStore;
    Engines::BackendCreationParameters backendParams;
    Group backend = constructBackend(backendName, backendParams);

    if (openInputFileCheck(fileName_)) {
        // Have a file, load the ODB file contents into the memory backend
        Engines::ODC::ODC_Parameters odcparams;
        odcparams.filename    = params.fileName;
        odcparams.mappingFile = params.mappingFileName;
        odcparams.queryFile   = params.queryFileName;
        odcparams.maxNumberChannels = params.maxNumberChannels;
        const util::DateTime missingDate = util::missingValue<util::DateTime>();
        odcparams.timeWindowStart = createParams_.timeWindow.start();
        odcparams.timeWindowExtendedLowerBound =
          params.timeWindowExtendedLowerBound.value() != boost::none ?
          params.timeWindowExtendedLowerBound.value().value() : missingDate;
        obs_group_ = Engines::ODC::openFile(odcparams, backend);
        oops::Log::trace() << "ioda::Engines::ReadOdbFile end constructor" << std::endl;
    } else {
        // Input file does not exist (is not readable actually)
        if (params.missingFileAction.value() == "warn") {
            oops::Log::info() << "WARNING: input file is not readable, "
               << "will continue with empty file representation" << std::endl
               << "WARNING:     file: " << fileName_ << std::endl;

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

std::string ReadOdbFile::fileName() const {
   return fileName_;
}

void ReadOdbFile::print(std::ostream & os) const {
  os << fileName_;
}

}  // namespace Engines
}  // namespace ioda
