/*
 * (C) Copyright 2022 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/Io/IoPoolBase.h"

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
void IoPoolBase::createIoPool(IoPoolGroupMap & rankGrouping) {
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
            for(std::size_t i = 0; i < totalNlocs.size(); ++i) {
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
               const util::DateTime & winStart, const util::DateTime & winEnd)
                   : params_(ioPoolParams),
                     comm_all_(commAll), rank_all_(commAll.rank()), size_all_(commAll.size()),
                     comm_time_(commTime), rank_time_(commTime.rank()),
                     size_time_(commTime.size()), win_start_(winStart), win_end_(winEnd),
                     total_nlocs_(0), global_nlocs_(0) {
}

IoPoolBase::~IoPoolBase() = default;

}  // namespace ioda
