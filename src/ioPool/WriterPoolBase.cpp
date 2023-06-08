/*
 * (C) Copyright 2022 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/ioPool/WriterPoolBase.h"

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
#include "ioda/ioPool/WriterPoolUtils.h"

#include "oops/util/Logger.h"

namespace ioda {

// For the MPI communicator splitting
constexpr int writerPoolColor = 1;
constexpr int writerNonPoolColor = 2;
const char writerPoolCommName[] = "writerIoPool";
const char writerNonPoolCommName[] = "writerNonIoPool";

//------------------------------------------------------------------------------------
// Writer pool creation parameters
//------------------------------------------------------------------------------------
WriterPoolCreationParameters::WriterPoolCreationParameters(
            const eckit::mpi::Comm & commAll, const eckit::mpi::Comm & commTime,
            const oops::RequiredPolymorphicParameter
                <Engines::WriterParametersBase, Engines::WriterFactory> & writerParams,
            const std::vector<bool> & patchObsVec)
                : IoPoolCreationParameters(commAll, commTime),
                  writerParams(writerParams), patchObsVec(patchObsVec) {
}


//------------------------------------------------------------------------------------
// Writer pool base class
//------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
WriterPoolBase::WriterPoolBase(const IoPoolParameters & configParams,
                               const WriterPoolCreationParameters & createParams)
                : IoPoolBase(configParams, createParams.commAll, createParams.commTime,
                             writerPoolColor, writerNonPoolColor,
                             writerPoolCommName, writerNonPoolCommName),
                  writerParams_(createParams.writerParams),
                  patchObsVec_(createParams.patchObsVec),
                  totalNlocs_(0), nlocsStart_(0), patchNlocs_(0) {
}

//--------------------------------------------------------------------------------------
void WriterPoolBase::setTotalNlocs(const std::size_t nlocs) {
    // Sum up the nlocs from assigned ranks. Set totalNlocs_ to zero for ranks not in
    // the io pool.
    if (this->commPool() == nullptr) {
        totalNlocs_ = 0;
    } else {
        totalNlocs_ = nlocs;
        for (std::size_t i = 0; i < rankAssignment().size(); ++i) {
            totalNlocs_ += rankAssignment()[i].second;
        }
    }
}

//--------------------------------------------------------------------------------------
void WriterPoolBase::collectSingleFileInfo() {
    // Want to determine two pieces of information:
    //   1. global nlocs which is the sum of all nlocs in all ranks in the io pool
    //   2. starting point along nlocs dimension for each rank in the io pool
    //
    // Only the ranks in the io pool should participate in this function
    //
    // Need the totalNlocs_ values from all ranks for both pieces of information.
    // Have rank 0 do the processing and then send the information back to the other ranks.
    if (this->commPool() != nullptr) {
        std::size_t root = 0;
        std::vector<std::size_t> totalNlocs(this->commPool()->size());
        std::vector<std::size_t> nlocsStarts(this->commPool()->size());

        this->commPool()->gather(totalNlocs_, totalNlocs, root);
        if (this->commPool()->rank() == root) {
            globalNlocs_ = 0;
            std::size_t nlocsStartingPoint = 0;
            for (std::size_t i = 0; i < totalNlocs.size(); ++i) {
                globalNlocs_ += totalNlocs[i];
                nlocsStarts[i] = nlocsStartingPoint;
                nlocsStartingPoint += totalNlocs[i];
            }
        }
        this->commPool()->broadcast(globalNlocs_, root);
        this->commPool()->scatter(nlocsStarts, nlocsStart_, root);
    }
}

}  // namespace ioda
