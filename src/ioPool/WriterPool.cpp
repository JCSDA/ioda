/*
 * (C) Copyright 2022 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/ioPool/WriterPool.h"

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
#include "ioda/ioPool/WriterUtils.h"

#include "oops/util/Logger.h"

namespace ioda {

// For the MPI communicator splitting
constexpr int writerPoolColor = 1;
constexpr int writerNonPoolColor = 2;
const char writerPoolCommName[] = "writerIoPool";
const char writerNonPoolCommName[] = "writerNonIoPool";

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
void WriterPool::setTotalNlocs(const std::size_t nlocs) {
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
void WriterPool::collectSingleFileInfo() {
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
WriterPool::WriterPool(const oops::Parameter<IoPoolParameters> & ioPoolParams,
               const oops::RequiredPolymorphicParameter
                   <Engines::WriterParametersBase, Engines::WriterFactory> & writerParams,
               const eckit::mpi::Comm & commAll, const eckit::mpi::Comm & commTime,
               const util::DateTime & winStart, const util::DateTime & winEnd,
               const std::vector<bool> & patchObsVec)
                   : IoPoolBase(ioPoolParams, commAll, commTime, winStart, winEnd,
                     writerPoolColor, writerNonPoolColor,
                     writerPoolCommName, writerNonPoolCommName),
                     writer_params_(writerParams), patch_obs_vec_(patchObsVec),
                     total_nlocs_(0), global_nlocs_(0) {
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

    // Create an object of the writer pre-/post-processor here so that it can be
    // accessed throught the lifetime of the io pool object. The lifetime of the
    // writer engine is only during the save function. The writer pre-/post-processor
    // and writer engine classes are separated so that the pre-/post-processor steps
    // can manipulate files that the save command uses.
    Engines::WriterCreationParameters createParams(*comm_pool_, comm_time_,
                                                   create_multiple_files_, is_parallel_io_);
    writer_proc_ = Engines::WriterProcFactory::create(writer_params_, createParams);
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

//--------------------------------------------------------------------------------------
void WriterPool::finalize() {
    oops::Log::trace() << "WriterPool::finalize, start" << std::endl;
    // Call the post processor associated with the backend engine being
    // used in the save function.
    if (comm_pool_ != nullptr) {
        writer_proc_->post();
    }

    // At this point there are two split communicator groups: one for the io pool and the
    // other for the processes not included in the io pool.
    if (eckit::mpi::hasComm(poolCommName_)) {
        eckit::mpi::deleteComm(poolCommName_);
    }
    if (eckit::mpi::hasComm(nonPoolCommName_)) {
        eckit::mpi::deleteComm(nonPoolCommName_);
    }
    oops::Log::trace() << "WriterPool::finalize, end" << std::endl;
}

//--------------------------------------------------------------------------------------
void WriterPool::print(std::ostream & os) const {
  os << writerDest_ << " (io pool size: " << size_pool_ << ")";
}

}  // namespace ioda
