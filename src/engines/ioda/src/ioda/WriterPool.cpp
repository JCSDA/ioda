/*
 * (C) Copyright 2022 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/Io/WriterPool.h"

#include <algorithm>
#include <cstdio>
#include <memory>
#include <mpi.h>
#include <numeric>
#include <sstream>
#include <utility>

#include "eckit/config/LocalConfiguration.h"

#include "ioda/Copying.h"
#include "ioda/Engines/EngineUtils.h"
#include "ioda/Engines/HH.h"
#include "ioda/Exception.h"
#include "ioda/Io/IoPoolUtils.h"
#include "ioda/Io/WriterUtils.h"


#include "oops/util/Logger.h"

namespace ioda {

constexpr int defaultMaxPoolSize = 10;

// These next two constants are the "color" values used for the MPI split comm command.
// They just need to be two different numbers, which will create the pool communicator,
// and a second communicator that holds all of the other ranks not in the pool.
//
// Unfortunately, the eckit interface doesn't appear to support using MPI_UNDEFINED for
// the nonPoolColor. Ie, you need to assign all ranks into a communcator group.
constexpr int poolColor = 1;
constexpr int nonPoolColor = 2;
const char poolCommName[] = "IoPool";
const char nonPoolCommName[] = "NonIoPool";

//--------------------------------------------------------------------------------------
void WriterPool::groupRanks(IoPoolGroupMap & rankGrouping) {
    rankGrouping.clear();
    if (rank_all_ == 0) {
        // We want the order of the locations in the resulting single output file after
        // concatenating the output files created by the io pool. To do this we need to
        // assign the tiles (block of locations from a given rank in the all_comm_ group)
        // in numeric order since this is how the concatenator puts together the files
        // from the current code. Ie, we want the tiles from rank 0 first, rank 1 second,
        // rank 2 third and so on.
        //
        // We also want to avoid transferring data between ranks selected for the io pool
        // since this isn't necessary. Ie, each rank in the pool should own its own tile.
        //
        // To accomplish this, divide the total number of ranks into groupings of an even
        // number of ranks under the assumption that the obs are fairly well load balanced.
        // TODO(srh) This assumption likely falls apart with the halo distribution but that
        // can be addressed later. If needed we can do the same type of grouping but base
        // it on the number of locations instead of the ranks which will make the MPI
        // transfers more complicated.
        int base_assign_size = size_all_ / target_pool_size_;
        int rem_assign_size = size_all_ % target_pool_size_;
        int start = 0;
        for (std::size_t i = 0; i < target_pool_size_; ++i) {
            int count = base_assign_size;
            if (i < rem_assign_size) {
                count += 1;
            }
            // start is the rank that goes into the pool, and the remaining sequence
            // of count-1 numbers starting with start+1 are the non pool ranks that
            // are associated with the pool rank (start).
            std::vector<int> rankGroup(count - 1);
            std::iota(rankGroup.begin(), rankGroup.end(), start + 1);
            rankGrouping.insert(std::make_pair(start, rankGroup));
            start += count;
        }
    }
}

//--------------------------------------------------------------------------------------
void WriterPool::assignRanksToIoPool(const std::size_t nlocs,
                                     const IoPoolGroupMap & rankGrouping) {
    constexpr int mpiTagBase = 10000;

    // Collect the nlocs from all of the other ranks.
    std::vector<std::size_t> allNlocs(size_all_);
    comm_all_.allGather(nlocs, allNlocs.begin(), allNlocs.end());

    if (rank_all_ == 0) {
        // Follow the grouping that is contained in the rankGrouping structure to create
        // the assignments for the MPI send/recv transfers. The rankAssignments structure
        // contains the mapping that is required to effect the proper MPI send/recv
        // transfers. A pool rank will receive from one or more non pool ranks and the
        // non pool ranks will send to one pool rank. The outer vector of rankAssignments
        // is indexed by the all_comm_ rank number, and the inner vector contains the list
        // of ranks the outer index rank interacts with for data transfers. Once constructed,
        // each inner vector of rankAssignments is sent to the associated rank in the
        // comm_all_ group.
        std::vector<std::vector<std::pair<int, int>>> rankAssignments(size_all_);
        std::vector<int> rankAssignSizes(size_all_, 0);
        for (auto & rankGroup : rankGrouping) {
            // rankGroup is a std::pair<int, std::vector<int>>
            // The first element is the pool rank, and the second element is
            // the list of associated non pool ranks.
            std::vector<std::pair<int, int>> rankGroupPairs(rankGroup.second.size());
            std::size_t i = 0;
            for (auto & nonPoolRank : rankGroup.second) {
                rankGroupPairs[i] = std::make_pair(nonPoolRank, allNlocs[nonPoolRank]);
                std::vector<std::pair<int, int>> associatedPoolRank;
                associatedPoolRank.push_back(
                    std::make_pair(rankGroup.first, allNlocs[nonPoolRank]));
                rankAssignments[nonPoolRank] = associatedPoolRank;
                rankAssignSizes[nonPoolRank] = 1;
                i += 1;
            }
            rankAssignments[rankGroup.first] = rankGroupPairs;
            rankAssignSizes[rankGroup.first] = rankGroupPairs.size();
        }

        // Send the rank assignments to the other ranks. Use scatter to spread the
        // sizes (number of ranks) in each rank's assignment. Then use send/receive
        // to transfer each ranks assignment.
        int myRankAssignSize;
        comm_all_.scatter(rankAssignSizes, myRankAssignSize, 0);

        // Copy my assignment directory. Use MPI send/recv for all other ranks.
        rank_assignment_ = rankAssignments[0];
        for (std::size_t i = 1; i < rankAssignments.size(); ++i) {
            if (rankAssignSizes[i] > 0) {
                comm_all_.send(rankAssignments[i].data(), rankAssignSizes[i], i, mpiTagBase + i);
            }
        }
    } else {
        // Receive the rank assignments from rank 0. First use scatter to receive the
        // sizes (number of ranks) in this rank's assignment.
        int myRankAssignSize;
        std::vector<int> dummyVector(size_all_);
        comm_all_.scatter(dummyVector, myRankAssignSize, 0);

        rank_assignment_.resize(myRankAssignSize);
        if (myRankAssignSize > 0) {
            comm_all_.receive(rank_assignment_.data(), myRankAssignSize, 0, mpiTagBase + rank_all_);
        }
    }
}

//--------------------------------------------------------------------------------------
WriterPool::WriterPool(const oops::Parameter<IoPoolParameters> & ioPoolParams,
               const oops::RequiredPolymorphicParameter
                   <Engines::WriterParametersBase, Engines::WriterFactory> & writerParams,
               const eckit::mpi::Comm & commAll, const eckit::mpi::Comm & commTime,
               const util::DateTime & winStart, const util::DateTime & winEnd,
               const std::vector<bool> & patchObsVec)
                   : IoPoolBase(ioPoolParams, commAll, commTime, winStart, winEnd),
                     writer_params_(writerParams), patch_obs_vec_(patchObsVec) {
    nlocs_ = patchObsVec.size();
    patch_nlocs_ = std::count(patchObsVec.begin(), patchObsVec.end(), true);
    // For now, the target pool size is simply the minumum of the specified (or default) max
    // pool size and the size of the comm_all_ communicator group.
    setTargetPoolSize();

    // This call will return a data structure that shows how to assign the ranks
    // to the io pools, plus which non io pool ranks get associated with the io pool
    // ranks. Only rank 0 needs to have this data since it will be used to form and
    // send the assignments to the other ranks.
    std::map<int, std::vector<int>> rankGrouping;
    groupRanks(rankGrouping);

    // This call will fill in the vector data member rank_assignment_, which holds all of
    // the ranks each member of the io pool needs to communicate with to collect the
    // variable data. Use the patch nlocs (ie, the number of locations "owned" by this
    // rank) to represent the number of locations after any duplicated locations are
    // removed.
    assignRanksToIoPool(patch_nlocs(), rankGrouping);

    // Create the io pool communicator group using the split communicator command.
    createIoPool(rankGrouping);

    // Calculate the total nlocs for each rank in the io pool.
    // This sets the total_nlocs_ data member and that holds the sum of
    // the nlocs from each rank (from comm_all_) that is assigned to this rank.
    // Use patch nlocs to get proper count after duplicate obs are removed.
    setTotalNlocs(patch_nlocs());

    // Calculate the "global nlocs" which is the sum of total_nlocs_ from each rank
    // in the io pool. This is used to set the sizes of the variables (dimensioned
    // by nlocs) for the single file output. Also calculate the nlocs starting point
    // (offset) into the single file output for this rank.
    collectSingleFileInfo();

    // Set the is_parallel_io_ flag. If a rank is not in the io pool, this gets set to
    // false, which is okay since the non io pool ranks do not use it.
    if (comm_pool_ != nullptr) {
        is_parallel_io_ = ((!params_.value().writeMultipleFiles) && (comm_pool_->size() > 1));
    } else {
        is_parallel_io_ = false;
    }

    // Set the create_multiple_files_ flag. If rank is not in the io pool, this gets
    // set to false which is okay since the non io pool ranks do not use it.
    if (comm_pool_ != nullptr) {
        create_multiple_files_ =
            ((params_.value().writeMultipleFiles) && (comm_pool_->size() > 1));
    } else {
        create_multiple_files_ = false;
    }
}

WriterPool::~WriterPool() = default;

//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
void WriterPool::save(const Group & srcGroup) {
    oops::Log::trace() << "WriterPool::save, start" << std::endl;
    Group fileGroup;
    if (comm_pool_ != nullptr) {
        Engines::WriterCreationParameters createParams(*comm_pool_, comm_time_,
                                          create_multiple_files_, is_parallel_io_);
        std::unique_ptr<Engines::WriterBase> writerEngine =
            Engines::WriterFactory::create(writer_params_, createParams);

        fileGroup = writerEngine->getObsGroup();

        // collect the destination from the writer engine instance
        std::ostringstream ss;
        ss << *writerEngine;
        writerDest_ = ss.str();
    }

    // Copy the ObsSpace ObsGroup to the output file Group.
    ioWriteGroup(*this, srcGroup, fileGroup, is_parallel_io_);
    oops::Log::trace() << "WriterPool::save, end" << std::endl;
}

void WriterPool::workaroundGenFileNames(std::string & finalFileName,
                                        std::string & tempFileName) {
    tempFileName = writer_params_.value().fileName;
    finalFileName = tempFileName;

    // Append "_flenstr" (fixed length strings) to the temp file name, then uniquify
    // in the same manner as the writer backend.
    std::size_t found = tempFileName.find_last_of(".");
    if (found == std::string::npos)
        found = tempFileName.length();
    tempFileName.insert(found, "_flenstr");

    std::size_t mpiRank = comm_pool_->rank();
    int mpiTimeRank = -1; // a value of -1 tells uniquifyFileName to skip this value
    if (comm_time_.size() > 1) {
        mpiTimeRank = comm_time_.rank();
    }
    if (create_multiple_files_) {
        // Tag on the rank number to the output file name to avoid collisions.
        // Don't use the time communicator rank number in the suffix if the size of
        // the time communicator is 1.
        tempFileName = uniquifyFileName(tempFileName, mpiRank, mpiTimeRank);
        finalFileName = uniquifyFileName(finalFileName, mpiRank, mpiTimeRank);
    } else {
        // TODO(srh) With the upcoming release (Sep 2022) we need to keep the uniquified
        // file name in order to prevent trashing downstream tools. We can get rid of
        // the file suffix after the release.
        // If we got to here, we either have just one process in io pool, or we
        // are going to write out the file in parallel mode. In either case, we want
        // the suffix part related to mpiRank to always be zero.
        tempFileName = uniquifyFileName(tempFileName, 0, mpiTimeRank);
        finalFileName = uniquifyFileName(finalFileName, 0, mpiTimeRank);
    }
}

void WriterPool::workaroundFixToVarLenStrings(const std::string & finalFileName,
                                              const std::string & tempFileName) {
    oops::Log::debug() << "WriterPool::finalize: applying flen to vlen strings workaround: "
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
    Engines::ReaderCreationParameters readerCreateParams(win_start_, win_end_,
                                      *comm_pool_, comm_time_, {}, isParallel);
    std::unique_ptr<Engines::ReaderBase> readerEngine = Engines::ReaderFactory::create(
          readerParams.engine.value().engineParameters, readerCreateParams);

    // Writer backend
    WorkaroundWriterParameters writerParams;
    eckit::LocalConfiguration writerConfig;
    eckit::LocalConfiguration writerSubConfig;
    writerSubConfig.set("type", "H5File");
    writerSubConfig.set("obsfile", writer_params_.value().fileName);
    writerConfig.set("engine", writerSubConfig);
    std::unique_ptr<Engines::WriterBase> workaroundWriter;

    // We want each rank to write its own corresponding file, and that can be accomplished
    // in telling the writer that we are to create multiple files and not use the parallel
    // io mode.
    writerParams.validateAndDeserialize(writerConfig);
    const bool createMultipleFiles = true;
    Engines::WriterCreationParameters writerCreateParams(*comm_pool_, comm_time_,
                                      createMultipleFiles, isParallel);
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

//--------------------------------------------------------------------------------------
void WriterPool::finalize() {
    oops::Log::trace() << "WriterPool::finalize, start" << std::endl;
    // TODO(srh) Workaround until we get fixed length string support in the netcdf-c
    // library. This is expected to be available in the 4.9.1 release of netcdf-c.
    // For now move the file with fixed length strings to a temporary
    // file (obsdataout.obsfile spec with "_flenstr" appended to the filename) and
    // then copy that file to the intended output file while changing the fixed
    // length strings to variable length strings.
    if (comm_pool_ != nullptr) {
        // Create the temp file name, move the output file to the temp file name,
        // then copy the file to the intended file name.
        std::string tempFileName;
        std::string finalFileName;
        workaroundGenFileNames(finalFileName, tempFileName);

        // If the output file was created using parallel io, then we only need rank 0
        // to do the rename, copy workaround.
        if (is_parallel_io_) {
            if (comm_pool_->rank() == 0) {
                workaroundFixToVarLenStrings(finalFileName, tempFileName);
            }
        } else {
            workaroundFixToVarLenStrings(finalFileName, tempFileName);
        }
    }

    // At this point there are two split communicator groups: one for the io pool and the
    // other for the processes not included in the io pool.
    if (eckit::mpi::hasComm(poolCommName)) {
        eckit::mpi::deleteComm(poolCommName);
    }
    if (eckit::mpi::hasComm(nonPoolCommName)) {
        eckit::mpi::deleteComm(nonPoolCommName);
    }
    oops::Log::trace() << "WriterPool::finalize, end" << std::endl;
}

//--------------------------------------------------------------------------------------
void WriterPool::print(std::ostream & os) const {
  os << writerDest_ << " (io pool size: " << size_pool_ << ")";
}

}  // namespace ioda
