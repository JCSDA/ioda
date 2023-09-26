/*
 * (C) Copyright 2022 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#include "ioda/Engines/WriteH5File.h"

#include "eckit/mpi/Parallel.h"

#include "ioda/Copying.h"   // for the post-processor workaround
#include "ioda/Engines/EngineUtils.h"

#include "oops/util/DateTime.h"  // for the post-processor workaround
#include "oops/util/Logger.h"

namespace ioda {
namespace Engines {

//---------------------------------------------------------------------
// WriteH5File
//---------------------------------------------------------------------

static WriterMaker<WriteH5File> makerH5File("H5File");

// Parameters

// Classes

WriteH5File::WriteH5File(const Parameters_ & params,
                         const WriterCreationParameters & createParams)
                             : WriterBase(createParams), params_(params) {
    oops::Log::trace() << "ioda::Engines::WriteH5File start constructor" << std::endl;
    // Create a backend to write to a new hdf5 file. If the allowOverwrite parameter is
    // true, then it is okay to clobber an existing file.
    Engines::BackendNames backendName = Engines::BackendNames::Hdf5File;

    // Figure out the output file name.
    std::size_t mpiRank = createParams_.comm.rank();
    int mpiTimeRank = -1; // a value of -1 tells uniquifyFileName to skip this value
    if (createParams_.timeComm.size() > 1) {
        mpiTimeRank = createParams_.timeComm.rank();
    }
    // Tag on the rank number to the output file name to avoid collisions.
    // Don't use the time communicator rank number in the suffix if the size of
    // the time communicator is 1.
    std::string outFileName = uniquifyFileName(params_.fileName,
        createParams_.createMultipleFiles, mpiRank, mpiTimeRank);

    Engines::BackendCreationParameters backendParams;
    backendParams.fileName = outFileName;
    if (createParams_.isParallelIo) {
        backendParams.action = Engines::BackendFileActions::CreateParallel;
        backendParams.comm = 
            dynamic_cast<const eckit::mpi::Parallel &>(createParams_.comm).MPIComm();
    } else {
        backendParams.action = Engines::BackendFileActions::Create;
    }
    if (params.allowOverwrite) {
        backendParams.createMode = Engines::BackendCreateModes::Truncate_If_Exists;
    } else {
        backendParams.createMode = Engines::BackendCreateModes::Fail_If_Exists;
    }

    Group backend = constructBackend(backendName, backendParams);
    obs_group_ = ObsGroup(backend);
    oops::Log::trace() << "ioda::Engines::WriteH5File end constructor" << std::endl;
}

void WriteH5File::print(std::ostream & os) const {
  os << params_.fileName.value();
}

//---------------------------------------------------------------------
// WriteH5Proc
//---------------------------------------------------------------------

static WriterProcMaker<WriteH5Proc> makerH5Proc("H5File");

// Parameters

// Classes

//--------------------------------------------------------------------------------------
WriteH5Proc::WriteH5Proc(const Parameters_ & params,
                         const WriterCreationParameters & createParams)
                             : WriterProcBase(createParams), params_(params) {
}

//--------------------------------------------------------------------------------------
void WriteH5Proc::post() {
    // TODO(srh) Workaround until we get fixed length string support in the netcdf-c
    // library. This is expected to be available in the 4.9.1 release of netcdf-c.
    // For now move the file with fixed length strings to a temporary
    // file (obsdataout.obsfile spec with "_flenstr" appended to the filename) and
    // then copy that file to the intended output file while changing the fixed
    // length strings to variable length strings.
    //
    // This function is only called by the io pool processes.
    //
    // Create the temp file name, move the output file to the temp file name,
    // then copy the file to the intended file name.
    std::string tempFileName;
    std::string finalFileName;
    workaroundGenFileNames(finalFileName, tempFileName);

    // If the output file was created using parallel io, then we only need rank 0
    // to do the rename, copy workaround.
    if (createParams_.isParallelIo) {
        if (createParams_.comm.rank() == 0) {
            workaroundFixToVarLenStrings(finalFileName, tempFileName);
        }
    } else {
        workaroundFixToVarLenStrings(finalFileName, tempFileName);
    }
}

//--------------------------------------------------------------------------------------
void WriteH5Proc::print(std::ostream & os) const {
  os << params_.fileName.value();
}

//--------------------------------------------------------------------------------------
void WriteH5Proc::workaroundGenFileNames(std::string & finalFileName,
                                         std::string & tempFileName) {
    tempFileName = params_.fileName;
    finalFileName = tempFileName;

    // Append "_flenstr" (fixed length strings) to the temp file name, then uniquify
    // in the same manner as the writer backend.
    std::size_t found = tempFileName.find_last_of(".");
    if (found == std::string::npos)
        found = tempFileName.length();
    tempFileName.insert(found, "_flenstr");

    std::size_t mpiRank = createParams_.comm.rank();
    int mpiTimeRank = -1; // a value of -1 tells uniquifyFileName to skip this value
    if (createParams_.timeComm.size() > 1) {
        mpiTimeRank = createParams_.timeComm.rank();
    }
    // Tag on the rank number to the output file name to avoid collisions.
    // Don't use the time communicator rank number in the suffix if the size of
    // the time communicator is 1.
    tempFileName = uniquifyFileName(tempFileName, createParams_.createMultipleFiles,
                                    mpiRank, mpiTimeRank);
    finalFileName = uniquifyFileName(finalFileName, createParams_.createMultipleFiles,
                                     mpiRank, mpiTimeRank);
}

void WriteH5Proc::workaroundFixToVarLenStrings(const std::string & finalFileName,
                                               const std::string & tempFileName) {
    oops::Log::trace() << "WriterPool::finalize: applying flen to vlen strings workaround: "
                       << tempFileName << " -> "
                       << finalFileName << std::endl;

    // Rename the output file, then copy back to the original name while changing the
    // strings back to variable length strings.
    if (std::rename(finalFileName.c_str(), tempFileName.c_str()) != 0) {
        throw Exception("Unable to rename output file.", ioda_Here());
    }

    // Create backends for reading the temp file and writing the final file.
    // Reader backend
    eckit::LocalConfiguration readerConfig;
    eckit::LocalConfiguration readerSubConfig;
    readerSubConfig.set("type", "H5File");
    readerSubConfig.set("obsfile", tempFileName);
    readerConfig.set("engine", readerSubConfig);

    WorkaroundReaderParameters readerParams;
    readerParams.validateAndDeserialize(readerConfig);
    const bool isParallel = false;
    const util::DateTime windowStart("1000-01-01T00:00:00Z");   // use a window that will
    const util::DateTime windowEnd("3000-01-01T00:00:00Z");     // accept all of the obs
    Engines::ReaderCreationParameters readerCreateParams(windowStart, windowEnd,
        createParams_.comm, createParams_.timeComm, {}, isParallel);
    std::unique_ptr<Engines::ReaderBase> readerEngine = Engines::ReaderFactory::create(
          readerParams.engine.value().engineParameters, readerCreateParams);

    // Writer backend
    WorkaroundWriterParameters writerParams;
    eckit::LocalConfiguration writerConfig;
    eckit::LocalConfiguration writerSubConfig;
    writerSubConfig.set("type", "H5File");
    writerSubConfig.set("obsfile", finalFileName);
    writerConfig.set("engine", writerSubConfig);
    std::unique_ptr<Engines::WriterBase> workaroundWriter;

    // We want each rank to write its own corresponding file, and that can be accomplished
    // in telling the writer that we are to create multiple files and not use the parallel
    // io mode.
    writerParams.validateAndDeserialize(writerConfig);
    const bool createMultipleFiles = false;
    Engines::WriterCreationParameters writerCreateParams(
        createParams_.comm, createParams_.timeComm, createMultipleFiles, isParallel);
    std::unique_ptr<Engines::WriterBase> writerEngine = Engines::WriterFactory::create(
          writerParams.engine.value().engineParameters, writerCreateParams);

    // Copy the contents from the temp file to the final file.
    Group readerTopGroup = readerEngine->getObsGroup();
    Group writerTopGroup = writerEngine->getObsGroup();
    copyGroup(readerTopGroup, writerTopGroup);

    // If we made it to here, we successfully copied the file, so we can remove
    // the temporary file.
    if (std::remove(tempFileName.c_str()) != 0) {
        oops::Log::info() << "WARNING: Unable to remove temporary output file: "
                          << tempFileName << std::endl;
    }
}

}  // namespace Engines
}  // namespace ioda
