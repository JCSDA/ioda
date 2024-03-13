/*
 * (C) Copyright 2022 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#include <fstream>
#include <boost/optional.hpp>

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

    obs_group_ = ObsStore::createRootGroup();
    oops::Log::trace() << "ioda::Engines::WriteOdbFile end constructor" << std::endl;
}

void WriteOdbFile::print(std::ostream & os) const {
    os << params_.fileName.value();
}

void WriteOdbFile::finalize() {
    oops::Log::trace() << "WriteOdbProc::finalize = " << std::endl;
    Engines::ODC::ODC_Parameters odcparams;
    const std::size_t mpiRank = createParams_.comm.rank();
    int mpiTimeRank = -1;  // a value of -1 tells uniquifyFileName to skip this value
    if (createParams_.timeComm.size() > 1) {
        mpiTimeRank = createParams_.timeComm.rank();
    }
    // Tag on the rank number to the output file name to avoid collisions.
    // Don't use the time communicator rank number in the suffix if the size of
    // the time communicator is 1.
    const std::string outFileName = uniquifyFileName(params_.fileName.value(),
        true, mpiRank, mpiTimeRank);
    odcparams.queryFile = params_.queryFileName;
    odcparams.mappingFile = params_.mappingFileName;
    odcparams.outputFile = outFileName;
    odcparams.odbType = params_.odbType;
    odcparams.missingObsSpaceVariableAbort = params_.missingObsSpaceVariableAbort;
    Group writerGroup = ioda::Engines::ODC::createFile(odcparams, obs_group_);
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
    // If a single output file was requested (write multiple files: false)
    // then the first processor reads in the files, concatenates them and
    // removes the file produced by each ioPool member.
    oops::Log::trace() << "WriteOdbProc::post" << std::endl;
    if (!createParams_.createMultipleFiles &&
        createParams_.comm.rank() == 0 &&
        createParams_.timeComm.rank() == 0) {
        const std::size_t mpiRankSize = createParams_.comm.size();
        const std::size_t mpiTimeRankSize = createParams_.timeComm.size();
        const std::string outFileName = params_.fileName.value();
        std::ofstream outFile(outFileName, std::ios_base::binary |
                              std::ios_base::out | std::ios_base::trunc);
        for (std::size_t irank = 0; irank < mpiRankSize; irank++) {
            for (std::size_t itime = 0; itime < mpiTimeRankSize; itime++) {
                int mpiTimeRank = -1;
                if (mpiTimeRankSize > 1) {
                    mpiTimeRank = itime;
                }
                const std::string inFileName =
                        uniquifyFileName(params_.fileName.value(), true, irank, mpiTimeRank);
                std::ifstream inFile(inFileName, std::ios_base::binary | std::ios_base::in);
                outFile << inFile.rdbuf();
                inFile.close();
                std::remove(inFileName.c_str());
            }
        }
        outFile.close();
    }
}

//--------------------------------------------------------------------------------------
bool WriteOdbProc::backendCanUseParallelIO() const {
    return false;
}

//--------------------------------------------------------------------------------------
void WriteOdbProc::print(std::ostream & os) const {
    os << params_.fileName.value();
}

}  // namespace Engines
}  // namespace ioda
