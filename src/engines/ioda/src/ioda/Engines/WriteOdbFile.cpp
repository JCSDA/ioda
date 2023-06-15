/*
 * (C) Copyright 2022 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#include "ioda/Engines/WriteOdbFile.h"

#include "ioda/Engines/EngineUtils.h"

#include "oops/util/Logger.h"

namespace ioda {
namespace Engines {

//---------------------------------------------------------------------
// WriteOdbFile
//---------------------------------------------------------------------

static WriterMaker<WriteOdbFile> makerOdbFile("ODB");

// Parameters

// Classes

WriteOdbFile::WriteOdbFile(const Parameters_ & params,
                           const WriterCreationParameters & createParams)
                               : WriterBase(createParams), params_(params) {
    oops::Log::trace() << "ioda::Engines::WriteOdbFile start constructor" << std::endl;
    // TODO(srh) Placeholder for now. This gets the engine factory test to pass, but we may
    // actually want it organized like this as the writer gets developed.

    // Create an in-memory backend
    Engines::BackendNames backendName = Engines::BackendNames::ObsStore;

    // Figure out the output file name.
    const std::size_t mpiRank = createParams_.comm.rank();
    int mpiTimeRank = -1;  // a value of -1 tells uniquifyFileName to skip this value
    if (createParams_.timeComm.size() > 1) {
        mpiTimeRank = createParams_.timeComm.rank();
    }
    // Tag on the rank number to the output file name to avoid collisions.
    // Don't use the time communicator rank number in the suffix if the size of
    // the time communicator is 1.
    const std::string outFileName = uniquifyFileName(params_.fileName,
        true, mpiRank, mpiTimeRank);

    Engines::BackendCreationParameters backendParams;
    backendParams.fileName = outFileName;
    Group backend = constructBackend(backendName, backendParams);

    obs_group_ = ObsGroup(backend);
    oops::Log::trace() << "ioda::Engines::WriteOdbFile end constructor" << std::endl;
}

void WriteOdbFile::print(std::ostream & os) const {
  os << params_.fileName.value();
}

//---------------------------------------------------------------------
// WriteOdbProc
//---------------------------------------------------------------------

static WriterProcMaker<WriteOdbProc> makerOdbProc("ODB");

// Parameters

// Classes

//--------------------------------------------------------------------------------------
WriteOdbProc::WriteOdbProc(const Parameters_ & params,
                           const WriterCreationParameters & createParams)
                               : WriterProcBase(createParams), params_(params) {
}

//--------------------------------------------------------------------------------------
void WriteOdbProc::post() {
}

//--------------------------------------------------------------------------------------
void WriteOdbProc::print(std::ostream & os) const {
  os << params_.fileName.value();
}

}  // namespace Engines
}  // namespace ioda
