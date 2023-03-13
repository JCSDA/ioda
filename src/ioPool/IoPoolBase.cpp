/*
 * (C) Copyright 2022 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/ioPool/IoPoolBase.h"

#include <mpi.h>

#include <algorithm>
#include <cstdio>
#include <memory>
#include <numeric>
#include <sstream>
#include <utility>

#include "eckit/config/LocalConfiguration.h"

#include "ioda/Copying.h"
#include "ioda/Engines/EngineUtils.h"
#include "ioda/Engines/HH.h"
#include "ioda/Exception.h"

#include "oops/util/Logger.h"

namespace ioda {

constexpr int defaultMaxPoolSize = 10;

//--------------------------------------------------------------------------------------
void IoPoolBase::setTargetPoolSize() {
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
void IoPoolBase::assignRanksToIoPool(const std::size_t nlocs,
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
void IoPoolBase::createIoPool(IoPoolGroupMap & rankGrouping) {
    int myColor;
    if (rank_all_ == 0) {
        // Create the split communicator for the io pool. The rankGrouping structure contains
        // the distinction between pool ranks and non pool ranks. The eckit split communicator
        // command doesn't yet handle the MPI_UNDEFINED spec for a color value, so for now
        // create a pool communicator group and a non pool communicator group.
        std::vector<int> splitColors(size_all_, nonPoolColor_);
        for (auto & rankGroup : rankGrouping) {
            splitColors[rankGroup.first] = poolColor_;
        }
        comm_all_.scatter(splitColors, myColor, 0);
    } else {
        // Create the split. Receive the split color from a scatter, then call the split
        // command
        std::vector<int> dummyColors(size_all_);
        comm_all_.scatter(dummyColors, myColor, 0);
    }

    if (myColor == nonPoolColor_) {
        comm_all_.split(myColor, nonPoolCommName_);
        comm_pool_ = nullptr;  // mark that this rank does not belong to an io pool
        rank_pool_ = -1;
        size_pool_ = -1;
    } else {
        comm_pool_ = &(comm_all_.split(myColor, poolCommName_));
        rank_pool_ = comm_pool_->rank();
        size_pool_ = comm_pool_->size();
    }
}

//--------------------------------------------------------------------------------------
void IoPoolBase::setTotalNlocs(const std::size_t nlocs) {
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
void IoPoolBase::collectSingleFileInfo() {
    // Want to determine two pieces of information:
    //   1. global nlocs which is the sum of all nlocs in all ranks in the io pool
    //   2. starting point along nlocs dimension for each rank in the io pool
    //
    // Only the ranks in the io pool should participate in this function
    //
    // Need the total_nlocs_ values from all ranks for both pieces of information.
    // Have rank 0 do the processing and then send the information back to the other ranks.
    if (comm_pool_ != nullptr) {
        std::size_t root = 0;
        std::vector<std::size_t> totalNlocs(size_pool_);
        std::vector<std::size_t> nlocsStarts(size_pool_);

        comm_pool_->gather(total_nlocs_, totalNlocs, root);
        if (rank_pool_ == root) {
            global_nlocs_ = 0;
            std::size_t nlocsStartingPoint = 0;
            for (std::size_t i = 0; i < totalNlocs.size(); ++i) {
                global_nlocs_ += totalNlocs[i];
                nlocsStarts[i] = nlocsStartingPoint;
                nlocsStartingPoint += totalNlocs[i];
            }
        }
        comm_pool_->broadcast(global_nlocs_, root);
        comm_pool_->scatter(nlocsStarts, nlocs_start_, root);
    }
}

//--------------------------------------------------------------------------------------
IoPoolBase::IoPoolBase(const oops::Parameter<IoPoolParameters> & ioPoolParams,
               const eckit::mpi::Comm & commAll, const eckit::mpi::Comm & commTime,
               const util::DateTime & winStart, const util::DateTime & winEnd,
               const int poolColor, const int nonPoolColor,
               const char * poolCommName, const char * nonPoolCommName)
                   : params_(ioPoolParams),
                     comm_all_(commAll), rank_all_(commAll.rank()), size_all_(commAll.size()),
                     comm_time_(commTime), rank_time_(commTime.rank()),
                     size_time_(commTime.size()), win_start_(winStart), win_end_(winEnd),
                     poolColor_(poolColor), nonPoolColor_(nonPoolColor),
                     poolCommName_(poolCommName), nonPoolCommName_(nonPoolCommName),
                     total_nlocs_(0), global_nlocs_(0) {
}

IoPoolBase::~IoPoolBase() = default;

}  // namespace ioda
