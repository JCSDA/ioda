/*
 * (C) Copyright 2022 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#include "ioda/Engines/WriteOdbFile.h"

#include "ioda/Misc/IoPoolUtils.h"

#include "oops/util/Logger.h"

namespace ioda {
namespace Engines {

//---------------------------------------------------------------------
// WriteOdbFile
//---------------------------------------------------------------------

static WriterMaker<WriteOdbFile> maker("ODB");

// Parameters

// Classes

WriteOdbFile::WriteOdbFile(const Parameters_ & params,
                           const util::DateTime & winStart,
                           const util::DateTime & winEnd,
                           const eckit::mpi::Comm & comm,
                           const eckit::mpi::Comm & timeComm,
                           const std::vector<std::string> & obsVarNames)
                               : WriterBase(winStart, winEnd, comm, timeComm, obsVarNames),
                                 fileName_(params.fileName) {
    oops::Log::trace() << "ioda::Engines::WriteOdbFile start constructor" << std::endl;
    // TODO(srh) Placeholder for now. This gets the engine factory test to pass, but we may
    // actually want it organized like this as the writer gets developed.

    // Create an in-memory backend
    Engines::BackendNames backendName = Engines::BackendNames::ObsStore;
    Engines::BackendCreationParameters backendParams;
    Group backend = constructBackend(backendName, backendParams);

    obs_group_ = ObsGroup(backend);
    oops::Log::trace() << "ioda::Engines::WriteOdbFile end constructor" << std::endl;
}

void WriteOdbFile::print(std::ostream & os) const {
  os << fileName_;
}

}  // namespace Engines
}  // namespace ioda
