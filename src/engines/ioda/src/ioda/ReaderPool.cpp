/*
 * (C) Copyright 2022 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/Io/ReaderPool.h"

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
#include "ioda/Io/ReaderUtils.h"


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
void ReaderPool::groupRanks(IoPoolGroupMap & rankGrouping) {
}

//--------------------------------------------------------------------------------------
void ReaderPool::assignRanksToIoPool(const std::size_t nlocs,
                                     const IoPoolGroupMap & rankGrouping) {
}

//--------------------------------------------------------------------------------------
ReaderPool::ReaderPool(const oops::Parameter<IoPoolParameters> & ioPoolParams,
               const oops::RequiredPolymorphicParameter
                   <Engines::ReaderParametersBase, Engines::ReaderFactory> & readerParams,
               const eckit::mpi::Comm & commAll, const eckit::mpi::Comm & commTime,
               const util::DateTime & winStart, const util::DateTime & winEnd,
               const std::vector<std::string> & obsVarNames)
                   : IoPoolBase(ioPoolParams, commAll, commTime, winStart, winEnd),
                     reader_params_(readerParams), obs_var_names_(obsVarNames) {
}

ReaderPool::~ReaderPool() = default;

//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
void ReaderPool::load(Group & destGroup) {
    Group fileGroup;
    if (comm_pool_ != nullptr) {
        Engines::ReaderCreationParameters
            createParams(win_start_, win_end_, *comm_pool_, comm_time_,
                         obs_var_names_, is_parallel_io_);
        std::unique_ptr<Engines::ReaderBase> readerEngine =
            Engines::ReaderFactory::create(reader_params_, createParams);

        fileGroup = readerEngine->getObsGroup();

        // collect the destination from the reader engine instance
        std::ostringstream ss;
        ss << *readerEngine;
        readerDest_ = ss.str();
    }

    // Copy the ObsSpace ObsGroup to the output file Group.
    ioReadGroup(*this, fileGroup, destGroup, is_parallel_io_);
}

//--------------------------------------------------------------------------------------
void ReaderPool::finalize() {
}

//--------------------------------------------------------------------------------------
void ReaderPool::print(std::ostream & os) const {
  os << readerDest_ << " (io pool size: " << size_pool_ << ")";
}

}  // namespace ioda
