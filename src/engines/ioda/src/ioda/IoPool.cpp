/*
 * (C) Copyright 2022 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include <memory>
#include <mpi.h>
#include <numeric>

#include "ioda/Copying.h"
#include "ioda/Engines/Factory.h"
#include "ioda/Engines/HH.h"
#include "ioda/Misc/IoPool.h"
#include "ioda/Misc/IoPoolUtils.h"

#include "ioda/Exception.h"

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
void IoPool::setTargetPoolSize() {
    if (rank_all_ == 0) {
        // Determine the maximum pool size. Use the default if the io pool spec is not
        // present, which is done for backward compatibility.
        int maxPoolSize = defaultMaxPoolSize;
        if (params_.value().maxPoolSize.value() > 0) {
            maxPoolSize = params_.value().maxPoolSize.value();
        }

        // The pool size will be the minimum of the maxPoolSize or the entire size of the
        // comm_all_ communicator group (ie, size_all_).
        int poolSize = maxPoolSize;
        if (size_all_ <= maxPoolSize) {
            poolSize = size_all_;
        }

        // Broadcast the target pool size to the other ranks
        target_pool_size_ = poolSize;
        comm_all_.broadcast(target_pool_size_, 0);
    } else {
        // Receive the broadcast of the target pool size
        comm_all_.broadcast(target_pool_size_, 0);
    }
}
 
//--------------------------------------------------------------------------------------
void IoPool::groupRanks(IoPoolGroupMap & rankGrouping) {
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
void IoPool::assignRanksToIoPool(const std::size_t nlocs, const IoPoolGroupMap & rankGrouping) {
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
void IoPool::createIoPool(IoPoolGroupMap & rankGrouping) {
    int myColor;
    if (rank_all_ == 0) {
        // Create the split communicator for the io pool. The rankGrouping structure contains
        // the distinction between pool ranks and non pool ranks. The eckit split communicator
        // command doesn't yet handle the MPI_UNDEFINED spec for a color value, so for now
        // create a pool communicator group and a non pool communicator group.
        std::vector<int> splitColors(size_all_, nonPoolColor);
        for (auto & rankGroup : rankGrouping) {
            splitColors[rankGroup.first] = poolColor;
        }
        comm_all_.scatter(splitColors, myColor, 0);
    } else {
        // Create the split. Receive the split color from a scatter, then call the split
        // command
        std::vector<int> dummyColors(size_all_);
        comm_all_.scatter(dummyColors, myColor, 0);
    }

    if (myColor == nonPoolColor) {
        comm_all_.split(myColor, nonPoolCommName);
        comm_pool_ = nullptr;  // mark that this rank does not belong to an io pool
        rank_pool_ = -1;
        size_pool_ = -1;
    } else {
        comm_pool_ = &(comm_all_.split(myColor, poolCommName));
        rank_pool_ = comm_pool_->rank();
        size_pool_ = comm_pool_->size();
    }
}

//--------------------------------------------------------------------------------------
void IoPool::setTotalNlocs(const std::size_t nlocs) {
    // Sum up the nlocs from assigned ranks. Set total_nlocs_ to zero for ranks not in
    // the io pool.
    if (comm_pool_ == nullptr) {
        total_nlocs_ = 0;
    } else {
        total_nlocs_ = nlocs;
        for (std::size_t i = 0; i < rank_assignment_.size(); ++i) {
            total_nlocs_ += rank_assignment_[i].second;
        }
    }
}

//--------------------------------------------------------------------------------------
IoPool::IoPool(const eckit::mpi::Comm & commAll, int timeRank, std::size_t nlocs,
               const std::string & fileName, const oops::Parameter<IoPoolParameters> & params)
                   : nlocs_(nlocs), filename_(fileName), comm_all_(commAll),
                     rank_all_(commAll.rank()), size_all_(commAll.size()), params_(params) {
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
    // variable data.
    assignRanksToIoPool(nlocs, rankGrouping);

    // Create the io pool communicator group using the split communicator command.
    createIoPool(rankGrouping);

    // Calculate the total nlocs for each rank in the io pool.
    setTotalNlocs(nlocs);

    // Uniquify the file name using the pool rank number.
    filename_ = uniquifyFileName(filename_, rank_pool_, timeRank);
}

IoPool::~IoPool() = default;

//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
void IoPool::save(const Group & srcGroup) {
    Group fileGroup;
    if (comm_pool_ != nullptr) {

        // Open the output file and copy the ObsSpace top level group to the file top level group.
        Engines::BackendNames backendName = Engines::BackendNames::Hdf5File;
        Engines::BackendCreationParameters backendParams;

        // Create an hdf5 file, and allow overwriting an existing file (for now)
        // Tag on the rank number to the output file name to avoid collisions if running
        // with multiple MPI tasks.
        backendParams.fileName = filename_;
        backendParams.action = Engines::BackendFileActions::Create;
        backendParams.createMode = Engines::BackendCreateModes::Truncate_If_Exists;

        // Create the backend group
        // Use the None DataLyoutPolicy for now to accommodate the current file format
        fileGroup = constructBackend(backendName, backendParams);
    }

    // Copy the ObsSpace ObsGroup to the output file Group.
    ioWriteGroup(*this, srcGroup, fileGroup);
}

//--------------------------------------------------------------------------------------
void IoPool::finalize() {
    // At this point there are two split communicator groups: one for the io pool and the
    // other for the processes not included in the io pool.
    if (eckit::mpi::hasComm(poolCommName)) {
        eckit::mpi::deleteComm(poolCommName);
    }
    if (eckit::mpi::hasComm(nonPoolCommName)) {
        eckit::mpi::deleteComm(nonPoolCommName);
    }
}

}  // namespace ioda
