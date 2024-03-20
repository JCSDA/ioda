/*
 * (C) Copyright 2022 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/ioPool/ReaderSinglePool.h"

#include <mpi.h>

#include <algorithm>
#include <cstdio>
#include <memory>
#include <numeric>
#include <sstream>
#include <utility>

#include "eckit/config/LocalConfiguration.h"
#include "eckit/exception/Exceptions.h"

#include "ioda/Copying.h"
#include "ioda/distribution/Distribution.h"
#include "ioda/Engines/EngineUtils.h"
#include "ioda/Exception.h"
#include "ioda/ioPool/ReaderPoolFactory.h"
#include "ioda/ioPool/ReaderPoolUtils.h"

#include "oops/util/Logger.h"
#include "oops/util/missingValues.h"

namespace ioda {
namespace IoPool {

// Io pool factory maker
static ReaderPoolMaker<ReaderSinglePool> makerReaderSinglePool("SinglePool");

static const int msgIsVarSize = 0;
static const int msgIsVarData = 0;

//--------------------------------------------------------------------------------------
ReaderSinglePool::ReaderSinglePool(const IoPoolParameters & configParams,
                                   const ReaderPoolCreationParameters & createParams)
                                       : ReaderPoolBase(configParams, createParams) {
    // Save a persistent copy of the JEDI missing value for a string variable that can
    // be used to properly replace a string fill value from the obs source with this
    // JEDI missing value. The replaceFillWithMissing function needs a char * pointing
    // to this copy of the JEDI missing value to transfer that value to the obs space
    // container.
    stringMissingValue_ = std::make_shared<std::string>(util::missingValue<std::string>());

    // Check that we have a valid entry for the file preparation type
    if ((createParams.inputFilePrepType != "external") &&
        (createParams.inputFilePrepType != "internal")) {
        const std::string errMsg = std::string("Unrecognized file preparation type: '") +
            createParams.inputFilePrepType +
            std::string("', must be one of: 'internal' or 'external'");
        throw Exception(errMsg, ioda_Here());
    }

    // Check to make sure the work directory parameter has been specified. Only the
    // ReaderSinglePool requires this parameter (ie, WriterSingelPool and
    // ReaderSinglePoolAllTasks do not). Also, the work directory is only required
    // for the internal file prep mode (external mode has the correct path to the
    // prepared file in the obsfile spec).
    if ((createParams.inputFilePrepType == "internal") &&
        (configParams.workDir.value() == "")) {
        const std::string errMsg =
            std::string("ReaderSinglePool: Must specify a work directory in the ") +
            std::string(" YAML configuration ('obs space.io pool.work directory' spec");
        throw Exception(errMsg, ioda_Here());
    }
}

//--------------------------------------------------------------------------------------
void ReaderSinglePool::setNewInputFileName() {
    if (this->commPool() != nullptr) {
        // Determine the time communicator rank number. A value of -1 tells the
        // uniquifyFileName routing to not add a suffix for the time communicator rank.
        int timeRankNum = -1;
        if (this->commTime().size() > 1) {
            timeRankNum = this->commTime().rank();
        }

        if (inputFilePrepType_ == "internal") {
            // Construct the path, name to the new input file. Strip off the trailing suffix
            // (.nc4, .odb, .nc, etc) and replace with "_<ioPoolRank>.nc4". For now, we will
            // always use the hdf5 backend for these files.

            // Get "basename" of the path in fileName
            std::size_t pos = this->fileName().find_last_of('/');
            if (pos == std::string::npos) {
                newInputFileName_ = this->fileName();
            } else {
                // fileName contains path information, strip off every character leading
                // up to the last '/'.
                newInputFileName_ = this->fileName().substr(pos + 1);
            }

            // Replace the file extension with .nc4
            pos = newInputFileName_.find_last_of('.');
            if (pos == std::string::npos) {
                // No file extension, simply add the ".nc4"
                newInputFileName_ += std::string(".nc4");
            } else {
                // Replace the extension with ".nc4"
                newInputFileName_ = newInputFileName_.substr(0, pos) + std::string(".nc4");
            }

            // Generate a file name using the poolRank number, and the time communicator
            // rank number. The second argument to uniquifyFileName is "write multiple files"
            // which needs to be true to allow uniquifyFileName to tag on the poolRank number.
            newInputFileName_ = this->workDir() + std::string("/") + newInputFileName_;
            newInputFileName_ = Engines::uniquifyFileName(
                newInputFileName_, true, this->commPool()->rank(), timeRankNum);
        } else {
            // fileName_ already has the correct path, only need to uniquify that name.

            // Replace the file extension with .nc4
            const std::size_t pos = fileName_.find_last_of('.');
            if (pos == std::string::npos) {
                // No file extension, simply add the ".nc4"
                newInputFileName_ = fileName_ + std::string(".nc4");
            } else {
                // Replace the extension with ".nc4"
                newInputFileName_ = fileName_.substr(0, pos) + std::string(".nc4");
            }

            // add on the rank number suffixes
            newInputFileName_ = Engines::uniquifyFileName(
                fileName_, true, this->commPool()->rank(), timeRankNum);
        }
    } else {
        newInputFileName_ = std::string("");
    }
}

//--------------------------------------------------------------------------------------
void ReaderSinglePool::setPrepInfoFileName() {
    if (this->commAll().rank() == 0) {
        std::string prepInfoFileSuffix("_prep_file_info");
        if (inputFilePrepType_ == "internal") {
            // Construct the path, name to the new input file. Strip off the trailing suffix
            // (.nc4, .odb, .nc, etc) and replace with "_<ioPoolRank>.nc4". For now, we will
            // always use the hdf5 backend for these files.

            // Get "basename" of the path in fileName
            std::size_t pos = this->fileName().find_last_of('/');
            if (pos == std::string::npos) {
                prepInfoFileName_ = this->fileName();
            } else {
                // fileName contains path information, strip off every character leading
                // up to the last '/'.
                prepInfoFileName_ = this->fileName().substr(pos + 1);
            }

            // Replace the file extension with .nc4
            pos = prepInfoFileName_.find_last_of('.');
            if (pos == std::string::npos) {
                // No file extension, simply add the prep info file suffix and ".nc4"
                prepInfoFileName_ = prepInfoFileName_ +
                                    prepInfoFileSuffix + std::string(".nc4");
            } else {
                // Replace the extension with the prep info file suffix and ".nc4"
                prepInfoFileName_ = prepInfoFileName_.substr(0, pos) +
                                   prepInfoFileSuffix + std::string(".nc4");
            }

            // Add the path to the work directory to the base file name and to
            // the prep info file name.
            prepInfoFileName_ = this->workDir() + std::string("/") + prepInfoFileName_;
        } else {
            // fileName_ already has the correct path, only need to uniquify that name.

            // Replace the file extension with .nc4
            const std::size_t pos = fileName_.find_last_of('.');
            if (pos == std::string::npos) {
                // No file extension, simply add the prep info file suffix and ".nc4"
                prepInfoFileName_ = fileName_ + prepInfoFileSuffix + std::string(".nc4");
            } else {
                // Replace the extension with the prep info file suffix and ".nc4"
                prepInfoFileName_ = fileName_.substr(0, pos) +
                                   prepInfoFileSuffix + std::string(".nc4");
            }
        }
        oops::mpi::broadcastString(this->commAll(), prepInfoFileName_, 0);
    } else {
        oops::mpi::broadcastString(this->commAll(), prepInfoFileName_, 0);
    }
}

//--------------------------------------------------------------------------------------
void ReaderSinglePool::restoreFilePrepInfo(std::map<int, std::vector<int>> & rankGrouping,
                                           int & expectedIoPoolRank) {
    const int numRanks = this->commAll().size();
    std::vector<std::size_t> numberLocations(numRanks, 0);
    std::vector<int> ioPoolRanks(numRanks, 0);
    rankGrouping.clear();
    if (this->commAll().rank() == 0) {
        // For now, the new input files will be hdf5 files.
        eckit::LocalConfiguration engineConfig =
            Engines::constructFileBackendConfig("hdf5", prepInfoFileName_);
        std::unique_ptr<Engines::ReaderBase> readerEngine =
            Engines::constructFileReaderFromConfig(timeWindow_, *commPool_, commTime_,
                obsVarNames_, isParallelIo_, engineConfig);
        ioda::Group fileGroup = readerEngine->getObsGroup();

        // Make sure the commAll size matches up with the file set
        int expectedNumRanks;
        int expectedPoolSize;
        fileGroup.atts.open("mpiCommAllSize").read<int>(expectedNumRanks);
        fileGroup.atts.open("mpiCommPoolSize").read<int>(expectedPoolSize);
        ASSERT(numRanks == expectedNumRanks);

        // Restore global information (ie same values on all MPI ranks)
        fileGroup.atts.open("globalNlocs").read<std::size_t>(globalNlocs_);
        fileGroup.atts.open("sourceNlocs").read<std::size_t>(sourceNlocs_);
        fileGroup.atts.open("sourceNlocsInsideTimeWindow")
            .read<std::size_t>(sourceNlocsInsideTimeWindow_);
        fileGroup.atts.open("sourceNlocsOutsideTimeWindow")
            .read<std::size_t>(sourceNlocsOutsideTimeWindow_);
        fileGroup.atts.open("sourceNlocsRejectQC").read<std::size_t>(sourceNlocsRejectQC_);
        fileGroup.atts.open("dtimeEpoch").read<std::string>(dtimeEpoch_);

        this->commAll().broadcast(globalNlocs_, 0);
        this->commAll().broadcast(sourceNlocs_, 0);
        this->commAll().broadcast(sourceNlocsInsideTimeWindow_, 0);
        this->commAll().broadcast(sourceNlocsOutsideTimeWindow_, 0);
        this->commAll().broadcast(sourceNlocsRejectQC_, 0);
        oops::mpi::broadcastString(this->commAll(), dtimeEpoch_, 0);

        // Note numberLocations is initialized to all zeros above which is what we want
        // when there is an empty file.
        std::vector<int> rankAssociation;
        fileGroup.vars.open("numberLocations").read<std::size_t>(numberLocations);
        fileGroup.vars.open("ioPoolRanks").read<int>(ioPoolRanks);
        fileGroup.vars.open("rankAssociation").read<int>(rankAssociation);
        ASSERT(numberLocations.size() == numRanks);
        ASSERT(ioPoolRanks.size() == numRanks);
        ASSERT(rankAssociation.size() == numRanks);

        // restore nlocs values to all ranks
        this->commAll().scatter(numberLocations, nlocs_, 0);

        // restore expected io pool rank values to all ranks
        this->commAll().scatter(ioPoolRanks, expectedIoPoolRank, 0);

        // restore targetPoolSize to all ranks
        targetPoolSize_ = numRanks - std::count(ioPoolRanks.begin(), ioPoolRanks.end(), -1);
        ASSERT(targetPoolSize_ == expectedPoolSize);
        this->commAll().broadcast(targetPoolSize_, 0);

        // restore rankGrouping - only need this on rank 0;
        for (std::size_t i = 0; i < rankAssociation.size(); ++i) {
            int poolRank = rankAssociation[i];
            if (i == poolRank) {
                // On an io pool rank, create a new entry in the map
                rankGrouping.insert(std::pair<int, std::vector<int>>(poolRank, {}));
            } else {
                // On a non pool rank (i), push back on the list for the pool rank
                rankGrouping.at(poolRank).push_back(i);
            }
        }
    } else {
        // Restore global information (ie same values on all MPI ranks)
        this->commAll().broadcast(globalNlocs_, 0);
        this->commAll().broadcast(sourceNlocs_, 0);
        this->commAll().broadcast(sourceNlocsInsideTimeWindow_, 0);
        this->commAll().broadcast(sourceNlocsOutsideTimeWindow_, 0);
        this->commAll().broadcast(sourceNlocsRejectQC_, 0);
        oops::mpi::broadcastString(this->commAll(), dtimeEpoch_, 0);

        // Restore nlocs values
        this->commAll().scatter(numberLocations, nlocs_, 0);

        // restore expected io pool rank values to all ranks
        this->commAll().scatter(ioPoolRanks, expectedIoPoolRank, 0);

        // restore targetPoolSize to all ranks
        this->commAll().broadcast(targetPoolSize_, 0);
    }
    emptyFile_ = (sourceNlocs_ == 0);
}

//------------------------------------------------------------------------------------
void ReaderSinglePool::adjustDistributionMap(const ioda::Group & fileGroup) {
    // The new mapping is located in the file top level variable "destinationRank".
    // Simply need to copy this information into the distributionMap_.
    // Only do this for the io pool member ranks.
    if (this->commPool() != nullptr) {
        // Read in the destination rank values. Use destination rank values for the keys
        // in the distributionMap_, along with the corresponding index value for the data
        // in the distributionMap_.
        const std::string varName = filePrepGroupName() + std::string("/destinationRank");
        std::vector<int> destRankValues;
        fileGroup.vars.open(varName).read<int>(destRankValues);

        // Don't alter the distributionMap_ if there are no obs left in this input file.
        if (destRankValues.size() > 0) {
            // Make two passes through the destination rank number. The first pass is to
            // count up the number of occurrences of each rank number which is done
            // to reserve the memory for the vectors in the distribution map. The second
            // pass is to copy the corresponding indices into the distribution map.
            std::map<int, std::size_t> destRankCounts;
            for (size_t i = 0; i < destRankValues.size(); ++i) {
                int key = destRankValues[i];
                if (destRankCounts.find(key) == destRankCounts.end()) {
                    // New entry, set count to 1
                    destRankCounts[key] = 1;
                } else {
                    // Existing entry, increment count
                    destRankCounts[key] += 1;
                }
            }
            distributionMap_.clear();
            for (size_t i = 0; i < destRankValues.size(); ++i) {
                int key = destRankValues[i];
                if (distributionMap_.find(key) == distributionMap_.end()) {
                    // New entry, allocate vector
                    distributionMap_[key].reserve(destRankCounts[key]);
                }
                // Add the index into the vector
                distributionMap_[key].push_back(i);
            }

            // At this point it is possible for the distribution map to be missing
            // entries. This situation comes about when the filtering and distribution
            // in the reader initialize step results in any of the processes in the
            // rank assignment getting zero obs. The code above won't produce an entry
            // for a rank with zero obs (including the pool member rank) and these cases
            // need to have an entry (with an empty vector) in the new distribution map.
            const int myRank = this->commAll().rank();
            if (distributionMap_.find(myRank) == distributionMap_.end()) {
                distributionMap_[myRank].clear();
            }
            for (const auto & rankAssign : rankAssignment_) {
                const int rank = rankAssign.first;
                if (distributionMap_.find(rank) == distributionMap_.end()) {
                    distributionMap_[rank].clear();
                }
            }
        }
    }
}

//--------------------------------------------------------------------------------------
void ReaderSinglePool::restoreIndicesRecNums(const ioda::Group & fileGroup) {
    // The distribution map is restored at this point so it can be used to
    // route the location indices and record numbers to their proper destination.
    if (this->commPool() != nullptr) {
        std::vector<std::size_t> locIndicesAllRanks;
        std::vector<std::size_t> recNumsAllRanks;
        fileGroup.vars.open("Location").read<std::size_t>(locIndicesAllRanks);
        fileGroup.vars.open(filePrepGroupName() + std::string("/recordNumbers"))
            .read<std::size_t>(recNumsAllRanks);

        // First save the location indices and record numbers belonging to this task
        int start = distributionMap_.at(this->commAll().rank())[0];
        int count = distributionMap_.at(this->commAll().rank()).size();
        locIndices_.assign(locIndicesAllRanks.begin() + start,
                           locIndicesAllRanks.begin() + start + count);
        recNums_.assign(recNumsAllRanks.begin() + start,
                        recNumsAllRanks.begin() + start + count);

        // Send the location indices and record numbers to the non pool members
        for (const auto & rankAssign : rankAssignment_) {
            const int toRank = rankAssign.first;
            start = distributionMap_.at(toRank)[0];
            count = distributionMap_.at(toRank).size();

            // First send the count, then send the data
            this->commAll().send(&count, 1, toRank, msgIsVarSize);
            this->commAll().send(locIndicesAllRanks.data() + start, count,
                                 toRank, msgIsVarSize);
            this->commAll().send(recNumsAllRanks.data() + start, count,
                                 toRank, msgIsVarData);
        }
    } else {
        // Receive the location indices and record numbers from the associated pool member rank
        for (const auto & rankAssign : rankAssignment_) {
            const int fromRank = rankAssign.first;
            int count;
            this->commAll().receive(&count, 1, fromRank, msgIsVarSize);
            locIndices_.resize(count);
            recNums_.resize(count);
            this->commAll().receive(locIndices_.data(), count, fromRank, msgIsVarSize);
            this->commAll().receive(recNums_.data(), count, fromRank, msgIsVarData);
        }
    }

    // Set nrecs_ based on the number of unique record numbers in recNums_
    const std::set<std::size_t> uniqueRecNums(recNums_.begin(), recNums_.end());
    nrecs_ = uniqueRecNums.size();
}

//--------------------------------------------------------------------------------------
void ReaderSinglePool::initialize() {
    oops::Log::trace() << "ReaderSinglePool::initialize, start" << std::endl;
    // If the file preparation is set to "internal", then we need to create the input
    // file set here. If it is external, then we skip the input file set creation.
    if (this->inputFilePrepType() == "internal") {
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
            oops::mpi::broadcastString(this->commAll(), fileName_, 0);

            // Send the engine applyLocationsCheck() value to the other ranks
            applyLocationsCheck = readerEngine->applyLocationsCheck();
            oops::mpi::broadcastBool(this->commAll(), applyLocationsCheck, 0);
        } else {
            oops::mpi::broadcastString(this->commAll(), fileName_, 0);
            oops::mpi::broadcastBool(this->commAll(), applyLocationsCheck, 0);
        }

        // Rank 0 does the preliminary checking and formation of the source location
        // indices and source record numbers. These are identical operations on each
        // MPI task so file io can be reduced by having rank 0 only do the io, generate
        // the indices and record numbers and broadcast that information to the other
        // ranks.

        // Check for required variables
        DateTimeFormat dtimeFormat;
        checkForRequiredVars(fileGroup, this->commAll(), readerSrc_, dtimeFormat, emptyFile_);

        // Read and convert the dtimeValues to the current epoch format if older formats are
        // being used in the source.
        std::vector<int64_t> dtimeValues;
        readSourceDtimeVar(fileGroup, this->commAll(), emptyFile_, dtimeFormat,
                           dtimeValues, dtimeEpoch_);

        // Convert the window start and end times to int64_t offsets from the dtimeEpoch
        // value. This will provide for a very fast "inside the timing window check".
        util::DateTime epochDt;
        convertEpochStringToDtime(dtimeEpoch_, epochDt);
        timeWindow_.setEpoch(epochDt);

        // Determine which locations will be retained by this process for its obs space
        // source_loc_indices_ holds the original source location index (position in
        // the 1D Location variable) and recNums_ holds the assigned record number.
        std::vector<float> lonValues;
        std::vector<float> latValues;
        setIndexAndRecordNums(fileGroup, this->commAll(), emptyFile_, distribution_, dtimeValues,
                              timeWindow_, applyLocationsCheck, obsGroupVarList_,
                              lonValues, latValues, sourceNlocs_,
                              sourceNlocsInsideTimeWindow_, sourceNlocsOutsideTimeWindow_,
                              sourceNlocsRejectQC_, locIndices_, recNums_,
                              globalNlocs_, nlocs_, nrecs_);

        // Check for consistency of the set of nlocs counts.
        ASSERT(sourceNlocs_ == sourceNlocsInsideTimeWindow_ + sourceNlocsOutsideTimeWindow_);
        ASSERT(sourceNlocs_ == globalNlocs_ + sourceNlocsOutsideTimeWindow_ + sourceNlocsRejectQC_);

        // Establish the reader pool which consists of assigning ranks in the "All"
        // communicator to the "Pool" communicator and then splitting the "All" communicator
        // to form the "Pool" communicator.
        buildIoPool(this->nlocs());

        // For each pool member record the source location indices that each associated
        // non-pool member requires. Note that the rankGrouping structure contains the
        // information about which ranks are in the pool, and the non-pool ranks
        // those pool ranks are associated with.
        setDistributionMap(*this, locIndices_, rankAssignment_, distributionMap_);

        // Make sure the work directory exists
        if (!ioda::Engines::haveDirRwxAccess(this->workDir())) {
            const std::string errMsg =
                std::string("Reader work directory is not accesible: ") + this->workDir();
            throw Exception(errMsg, ioda_Here());
        }
        oops::Log::info() << "ReaderSinglePool: reader work directory: "
                          << this->workDir() << std::endl;

        // Generate and record the new input file name for use here and in the save function.
        // Then create the new input files (one for each pool member)
        this->setPrepInfoFileName();
        this->setNewInputFileName();
        readerCreateFileSet(*this, fileGroup, dtimeValues, dtimeEpoch_,
                            lonValues, latValues);
    } else {
        // fileName_ can be set directly from the obsfile spec since we are using externally
        // prepared files.
        eckit::LocalConfiguration backendParamsConfig;
        this->readerParams_.serialize(backendParamsConfig);
        fileName_ = backendParamsConfig.getString("obsfile");
        this->setPrepInfoFileName();

        // Restore the file prep information
        std::map<int, std::vector<int>> rankGrouping;
        int expectedIoPoolRank;
        restoreFilePrepInfo(rankGrouping, expectedIoPoolRank);

        // Expand the rankGrouping map on rank 0 to rank assingments on all ranks
        assignRanksToIoPool(nlocs_, rankGrouping);

        // Create the io pool, which should match up with the MPI configuration information
        // in the prep info file. After creating the io pool, check to make sure the
        // pool rank matches up with the expected pool rank from the prep info file. Note
        // that a -1 means not in the io pool, and commPool set to a null pointer also
        // means not in the io pool.
        createIoPool(rankGrouping);
        if (this->commPool() != nullptr) {
            // this rank is in the io pool
            ASSERT(this->commPool()->rank() == expectedIoPoolRank);
        } else {
            // this rank is not int the io pool
            ASSERT(expectedIoPoolRank == -1);
        }

        // Need to create new input file names (ie, with the rank numbers appended).
        // First query the configuration in the reader parameters given to this io pool object
        // and set the fileName_ data member accordingly.
        this->setNewInputFileName();
    }
    oops::Log::trace() << "ReaderSinglePool::initialize, end" << std::endl;
}

//--------------------------------------------------------------------------------------
void ReaderSinglePool::load(Group & destGroup) {
    oops::Log::trace() << "ReaderSinglePool::load, start" << std::endl;
    Group fileGroup;
    std::unique_ptr<Engines::ReaderBase> readerEngine = nullptr;
    if (this->commPool() != nullptr) {
        // For now, the new input files will be hdf5 files.
        eckit::LocalConfiguration engineConfig =
            Engines::constructFileBackendConfig("hdf5", newInputFileName_);
        readerEngine = Engines::constructFileReaderFromConfig(timeWindow_,
            *commPool_, commTime_, obsVarNames_, isParallelIo_, engineConfig);
        fileGroup = readerEngine->getObsGroup();

        // Collect the destination from the reader engine instance
        std::ostringstream ss;
        ss << *readerEngine;
        readerSrc_ = ss.str();

        // Engine initialization
        readerEngine->initialize();
    }

    // Create the memory backend for the destGroup
    // TODO(srh) There needs to be a memory Engine structure created with ObsStore and
    // Hdf5Mem subclasses. Then call the corresponding factory function from here.
    Engines::BackendNames backendName = Engines::BackendNames::ObsStore;
    Engines::BackendCreationParameters backendParams;
    Group backend = constructBackend(backendName, backendParams);

    // Create the ObsGroup and attach the backend.
    destGroup = ObsGroup::generate(backend, {});

    // During the initialize() step the locations were rearranged into smaller
    // sets according to the destination ranks. This means that the distribution
    // maps calculated for the building the original input files
    // (during the initialization step) need to be adjusted to distribute the
    // locations in the new input files.
    if (nlocs_ > 0) {
        adjustDistributionMap(fileGroup);
        restoreIndicesRecNums(fileGroup);
    }

    // Copy the group structure (groups and their attributes) contained in the fileGroup
    // to the destGroup. Note that the readerCopyGroupStructure function will generate
    // a string holding YAML that describes the input file group structure.
    readerCopyGroupStructure(*this, fileGroup, emptyFile_, destGroup, groupStructureYaml_);

    // Transfer the variable data from the fileGroup to the memGroup for each MPI rank
    if (!emptyFile_) {
        readerTransferVarData(*this, fileGroup, destGroup, groupStructureYaml_);
    }

    // Engine finalization
    if (this->commPool() != nullptr) {
        if (readerEngine != nullptr) {
            readerEngine->finalize();
        }
    }
    oops::Log::trace() << "ReaderSinglePool::load, end" << std::endl;
}

//--------------------------------------------------------------------------------------
void ReaderSinglePool::finalize() {
    oops::Log::trace() << "ReaderSinglePool::finalize, start" << std::endl;
    // At this point there are two split communicator groups: one for the io pool and the
    // other for the processes not included in the io pool.
    if (eckit::mpi::hasComm(poolCommName_)) {
        eckit::mpi::deleteComm(poolCommName_);
    }
    if (eckit::mpi::hasComm(nonPoolCommName_)) {
        eckit::mpi::deleteComm(nonPoolCommName_);
    }
    oops::Log::trace() << "ReaderSinglePool::finalize, end" << std::endl;
}

//--------------------------------------------------------------------------------------
void ReaderSinglePool::print(std::ostream & os) const {
  int poolSize = 0;
  if (this->commPool() != nullptr) {
    poolSize = this->commPool()->size();
  }
  os << readerSrc_ << " (io pool size: " << poolSize << ")";
}

}  // namespace IoPool
}  // namespace ioda
