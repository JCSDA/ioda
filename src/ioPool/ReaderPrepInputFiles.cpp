/*
 * (C) Copyright 2022 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/ioPool/ReaderPrepInputFiles.h"

#include "ioda/distribution/Distribution.h"
#include "ioda/Exception.h"
#include "ioda/ioPool/ReaderPoolFactory.h"
#include "ioda/ioPool/ReaderPoolUtils.h"

#include "oops/util/Logger.h"

namespace ioda {
namespace IoPool {

// Io pool factory maker
static ReaderPoolMaker<ReaderPrepInputFiles> makerReaderPrepInputFiles("PrepInputFiles");

//--------------------------------------------------------------------------------------
ReaderPrepInputFiles::ReaderPrepInputFiles(
    const IoPoolParameters & configParams, const ReaderPoolCreationParameters & createParams)
        : ReaderPoolBase(configParams, createParams) {
    // Check that the optional file preparation parameters have been specified.
    if (configParams.prepFileParameters.value() == boost::none) {
        const std::string errMsg = std::string("ReaderPrepInputFiles: Must specify the ") +
            std::string("'obs space.io pool.file preparation' section ") +
            std::string("in the YAML configuration.");
        throw Exception(errMsg, ioda_Here());
    }
}

//--------------------------------------------------------------------------------------
void ReaderPrepInputFiles::initialize() {
    oops::Log::trace() << "ReaderPrepInputFiles::initialize, start" << std::endl;
    // Run the pre-processing steps that establish which locations go to
    // which ranks. These steps include the timing window filtering, quality checks,
    // obs grouping and applying the mpi distribution scheme.

    // Rank 0 is the only rank that opens the input file. The time window filter, quality
    // checks, obs grouping and applicatin of the MPI distribution are performed with
    // all ranks (in commAll_) and rank 0 writes out results into a temp file. Eventually,
    // rank 0 will rearrange the locations and split up into files for each rank in the
    // io pool.
    Group fileGroup;
    bool applyLocationsCheck;
    if (this->commAll().rank() == 0) {
        Engines::ReaderCreationParameters
            createParams(timeWindow_, commAll_, commTime_,
                         obsVarNames_, isParallelIo_);
        std::unique_ptr<Engines::ReaderBase> readerEngine =
            Engines::ReaderFactory::create(readerParams_, createParams);

        fileGroup = readerEngine->getObsGroup();

        // Engine initialization
        readerEngine->initialize();

        // Collect the destination from the reader engine instance
        std::ostringstream ss;
        ss << *readerEngine;
        readerSrc_ = ss.str();

        // Store the file name associated with the reader engine
        fileName_ = readerEngine->fileName();

        // Store the engine applyLocationsCheck() from the reader engine
        applyLocationsCheck = readerEngine->applyLocationsCheck();

        // Rank 0 does the preliminary checking and formation of the source location
        // indices and source record numbers. These are identical operations on each
        // MPI task so file io can be reduced by having rank 0 only do the io, generate
        // the indices and record numbers and broadcast that information to the other
        // ranks.
        bool emptyFile;
        DateTimeFormat dtimeFormat;
        std::vector<int64_t> dtimeValues;
        std::vector<float> lonValues;
        std::vector<float> latValues;
        std::vector<std::size_t> sourceLocIndices;
        std::vector<std::size_t> sourceRecNums;
        extractGlobalInfoFromSource(this->commAll(), fileGroup, readerSrc_, timeWindow_,
            applyLocationsCheck, obsGroupVarList_, dtimeValues, lonValues, latValues,
            sourceLocIndices, sourceRecNums, emptyFile, dtimeFormat, dtimeEpoch_, globalNlocs_,
            sourceNlocs_, sourceNlocsInsideTimeWindow_, sourceNlocsOutsideTimeWindow_,
            sourceNlocsRejectQC_);

        // Check for consistency of the set of nlocs counts.
        ASSERT(sourceNlocs_ == sourceNlocsInsideTimeWindow_ + sourceNlocsOutsideTimeWindow_);
        ASSERT(sourceNlocs_ ==
                   globalNlocs_ + sourceNlocsOutsideTimeWindow_ + sourceNlocsRejectQC_);

        // Emulate the formation of the rankGrouping given the target mpi communicator size
        int mpiCommSize = configParams_.prepFileParameters.value()->mpiCommSize;
        setTargetPoolSize(mpiCommSize);
        IoPoolGroupMap rankGrouping;
        groupRanks(mpiCommSize, rankGrouping);

        // Emulate the mpi distribution given rankGrouping
        std::vector<int> assocAllRanks;
        std::vector<int> ioPoolRanks;
        std::vector<std::size_t> locIndicesAllRanks;
        std::vector<int> locIndicesStarts;
        std::vector<int> locIndicesCounts;  // Note this matches the nlocs_ value for each rank
        std::vector<std::size_t> recNumsAllRanks;
        emulateMpiDistribution(this->distribution()->name(), emptyFile, mpiCommSize,
                               this->targetPoolSize(), rankGrouping, sourceLocIndices,
                               sourceRecNums, assocAllRanks, ioPoolRanks,
                               locIndicesAllRanks, locIndicesStarts,
                               locIndicesCounts, recNumsAllRanks);

        // Set the output file names
        // Both the first arguement to setPrepInfoFileName, and the second arguement to
        // setNewInputFileName is set to true to tells these functions to use the work
        // directory parameter when forming the new file names.
        const std::string prepOutputFile = configParams_.prepFileParameters.value()->outputFile;
        prepInfoFileName_ = setPrepInfoFileName(prepOutputFile);
        std::vector<std::string> assocFileNames(ioPoolRanks.size());
        for (std::size_t i = 0; i < assocFileNames.size(); ++i) {
            if (ioPoolRanks[i] >= 0) {
                assocFileNames[i] = this->setNewInputFileName(prepOutputFile, ioPoolRanks[i]);
            } else {
                assocFileNames[i] = std::string("");
            }
        }

        // Build the input files
        readerBuildInputFiles(*this, mpiCommSize, this->targetPoolSize(),
            fileGroup, assocAllRanks, ioPoolRanks, assocFileNames,
            locIndicesAllRanks, locIndicesStarts, locIndicesCounts, recNumsAllRanks,
            dtimeValues, dtimeEpoch_, lonValues, latValues);
    }
    this->commAll().barrier();
    oops::Log::trace() << "ReaderPrepInputFiles::initialize, end" << std::endl;
}

//--------------------------------------------------------------------------------------
void ReaderPrepInputFiles::load(Group & destGroup) {
    oops::Log::trace() << "ReaderPrepInputFiles::load, start" << std::endl;
    oops::Log::trace() << "ReaderPrepInputFiles::load, end" << std::endl;
}

//--------------------------------------------------------------------------------------
void ReaderPrepInputFiles::finalize() {
    oops::Log::trace() << "ReaderPrepInputFiles::finalize, start" << std::endl;
    oops::Log::trace() << "ReaderPrepInputFiles::finalize, end" << std::endl;
}

//--------------------------------------------------------------------------------------
void ReaderPrepInputFiles::print(std::ostream & os) const {
  os << readerSrc_ << " (target io pool size: " << targetPoolSize_ << ")";
}

}  // namespace IoPool
}  // namespace ioda
