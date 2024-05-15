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
#include "ioda/Exception.h"

#include "oops/util/Logger.h"

namespace ioda {
namespace IoPool {

constexpr int defaultMaxPoolSize = 4;

//------------------------------------------------------------------------------------
// Common io pool creation parameters
//------------------------------------------------------------------------------------
IoPoolCreationParameters::IoPoolCreationParameters(
            const eckit::mpi::Comm & commAll, const eckit::mpi::Comm & commTime)
                : commAll(commAll), commTime(commTime) {
}

//------------------------------------------------------------------------------------
// Io pool base class
//------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
IoPoolBase::IoPoolBase(
               const IoPoolParameters & configParams,
               const eckit::mpi::Comm & commAll, const eckit::mpi::Comm & commTime,
               const int poolColor, const int nonPoolColor,
               const char * poolCommName, const char * nonPoolCommName)
                   : configParams_(configParams), commAll_(commAll), commTime_(commTime),
                     poolColor_(poolColor), nonPoolColor_(nonPoolColor),
                     poolCommName_(poolCommName), nonPoolCommName_(nonPoolCommName),
                     targetPoolSize_(0), nlocs_(0), globalNlocs_(0) {
}

//--------------------------------------------------------------------------------------
void IoPoolBase::setTargetPoolSize(const int numMpiTasks) {
    if (commAll().rank() == 0) {
        // Determine the maximum pool size. Use the default if the io pool spec is not
        // present, which is done for backward compatibility.
        int maxPoolSize = defaultMaxPoolSize;
        if (configParams_.maxPoolSize.value() > 0) {
            maxPoolSize = configParams_.maxPoolSize.value();
        }

        // The pool size will be the minimum of the maxPoolSize or the numMpiTasks
        // argument.
        int poolSize = maxPoolSize;
        if (numMpiTasks <= maxPoolSize) {
            poolSize = numMpiTasks;
        }

        // Broadcast the target pool size to the other ranks
        targetPoolSize_ = poolSize;
    }
    commAll().broadcast(targetPoolSize_, 0);
}

//--------------------------------------------------------------------------------------
void IoPoolBase::groupRanks(const int numMpiTasks, IoPoolGroupMap & rankGrouping) {
    rankGrouping.clear();
    if (commAll().rank() == 0) {
        // This default grouping preserves the ordering that we had before the io pool
        // was introduced. For the reader this doesn't matter all that much since the
        // distribution is driven by the location indices, and for the writer this
        // preserves the order we used to get when every task wrote a file and we
        // concatenating the output those files in rank order.
        //
        // To accomplish this we need to assign the tiles (block of locations from a given
        // rank in the all_comm_ group) in numeric order since this is how the
        // concatenator puts together the files from the current code. Ie, we want
        // the tiles from rank 0 first, rank 1 second, rank 2 third and so on.
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
        ASSERT(targetPoolSize_ != 0);
        int base_assign_size = numMpiTasks / targetPoolSize_;
        int rem_assign_size = numMpiTasks % targetPoolSize_;
        int start = 0;
        for (int i = 0; i < targetPoolSize_; ++i) {
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
void IoPoolBase::assignRanksToIoPool(const std::size_t nlocs,
                                     const IoPoolGroupMap & rankGrouping) {
    constexpr int mpiTagBase = 10000;

    // Collect the nlocs from all of the other ranks.
    std::vector<std::size_t> allNlocs(commAll().size());
    commAll().allGather(nlocs, allNlocs.begin(), allNlocs.end());

    if (commAll().rank() == 0) {
        // Follow the grouping that is contained in the rankGrouping structure to create
        // the assignments for the MPI send/recv transfers. The rankAssignments structure
        // contains the mapping that is required to effect the proper MPI send/recv
        // transfers. A pool rank will receive from one or more non pool ranks and the
        // non pool ranks will send to one pool rank. The outer vector of rankAssignments
        // is indexed by the all_comm_ rank number, and the inner vector contains the list
        // of ranks the outer index rank interacts with for data transfers. Once constructed,
        // each inner vector of rankAssignments is sent to the associated rank in the
        // commAll() group.
        std::vector<std::vector<std::pair<int, int>>> rankAssignments(commAll().size());
        std::vector<int> rankAssignSizes(commAll().size(), 0);
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
        commAll().scatter(rankAssignSizes, myRankAssignSize, 0);

        // Copy my assignment directory. Use MPI send/recv for all other ranks.
        rankAssignment_ = rankAssignments[0];
        for (std::size_t i = 1; i < rankAssignments.size(); ++i) {
            if (rankAssignSizes[i] > 0) {
                commAll().send(rankAssignments[i].data(), rankAssignSizes[i],
                               i, mpiTagBase + i);
            }
        }
    } else {
        // Receive the rank assignments from rank 0. First use scatter to receive the
        // sizes (number of ranks) in this rank's assignment.
        int myRankAssignSize;
        std::vector<int> dummyVector(commAll().size());
        commAll().scatter(dummyVector, myRankAssignSize, 0);

        rankAssignment_.resize(myRankAssignSize);
        if (myRankAssignSize > 0) {
            commAll().receive(rankAssignment_.data(), myRankAssignSize,
                              0, mpiTagBase + commAll().rank());
        }
    }
}

//--------------------------------------------------------------------------------------
void IoPoolBase::createIoPool(IoPoolGroupMap & rankGrouping) {
    int myColor;
    if (commAll().rank() == 0) {
        // Create the split communicator for the io pool. The rankGrouping structure contains
        // the distinction between pool ranks and non pool ranks. The eckit split communicator
        // command doesn't yet handle the MPI_UNDEFINED spec for a color value, so for now
        // create a pool communicator group and a non pool communicator group.
        std::vector<int> splitColors(commAll().size(), nonPoolColor_);
        for (auto & rankGroup : rankGrouping) {
            splitColors[rankGroup.first] = poolColor_;
        }
        commAll().scatter(splitColors, myColor, 0);
    } else {
        // Create the split. Receive the split color from a scatter, then call the split
        // command
        std::vector<int> dummyColors(commAll().size());
        commAll().scatter(dummyColors, myColor, 0);
    }

    if (myColor == nonPoolColor_) {
        commAll().split(myColor, nonPoolCommName_);
        commPool_ = nullptr;  // mark that this rank does not belong to an io pool
    } else {
        commPool_ = &(commAll().split(myColor, poolCommName_));
    }
}

//--------------------------------------------------------------------------------------
void IoPoolBase::buildIoPool(const std::size_t numLocs) {
    // Note that derived subclasses can override the functions being called in this
    // function. The comments here are the default behaviors when there are no overrides.

    // The target pool size is simply the minumum of the specified (or default) max
    // pool size and the size of the comm_all_ communicator group.
    setTargetPoolSize(commAll().size());

    // This call will return a data structure that shows how to assign the ranks
    // to the io pools, plus which non io pool ranks get associated with the io pool
    // ranks. Only rank 0 needs to have this data since it will be used to form and
    // send the assignments to the other ranks.
    IoPoolGroupMap rankGrouping;
    groupRanks(commAll().size(), rankGrouping);

    // This call will fill in the vector data member rank_assignment_, which holds all of
    // the ranks each member of the io pool needs to communicate with to collect the
    // variable data. Use the patch nlocs (ie, the number of locations "owned" by this
    // rank) to represent the number of locations after any duplicated locations are
    // removed.
    assignRanksToIoPool(numLocs, rankGrouping);

    // Create the io pool communicator group using the split communicator command.
    createIoPool(rankGrouping);
}

}  // namespace IoPool
}  // namespace ioda
