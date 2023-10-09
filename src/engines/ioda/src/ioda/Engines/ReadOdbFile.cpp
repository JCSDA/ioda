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

    // And load the ODB file into it
    Engines::ODC::ODC_Parameters odcparams;
    odcparams.filename    = params.fileName;
    odcparams.mappingFile = params.mappingFileName;
    odcparams.queryFile   = params.queryFileName;
    odcparams.maxNumberChannels = params.maxNumberChannels;
    const util::DateTime missingDate = util::missingValue<util::DateTime>();
    odcparams.timeWindowStart = createParams_.timeWindow.start() + util::Duration(1);
    odcparams.timeWindowExtendedLowerBound =
      params.timeWindowExtendedLowerBound.value() != boost::none ?
      params.timeWindowExtendedLowerBound.value().value() : missingDate;
    obs_group_ = Engines::ODC::openFile(odcparams, backend);
    oops::Log::trace() << "ioda::Engines::ReadOdbFile end constructor" << std::endl;
}

std::string ReadOdbFile::fileName() const {
   return fileName_;
}

void ReadOdbFile::print(std::ostream & os) const {
  os << fileName_;
}

}  // namespace Engines
}  // namespace ioda
