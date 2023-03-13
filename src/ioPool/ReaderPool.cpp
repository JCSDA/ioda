/*
 * (C) Copyright 2022 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/ioPool/ReaderPool.h"

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
#include "ioda/Engines/HH.h"
#include "ioda/Exception.h"
#include "ioda/ioPool/ReaderUtils.h"

#include "oops/util/Logger.h"
#include "oops/util/missingValues.h"

namespace ioda {

// For the MPI communicator splitting
constexpr int readerPoolColor = 3;
constexpr int readerNonPoolColor = 4;
const char readerPoolCommName[] = "readerIoPool";
const char readerNonPoolCommName[] = "readerNonIoPool";

//--------------------------------------------------------------------------------------
void ReaderPool::groupRanks(IoPoolGroupMap & rankGrouping) {
    // TODO(srh) Until the actual reader pool is implemented we need to copy the
    // comm_all_ communicator to the comm_pool_ communicator. This can be accomplished
    // by constructing the rankGrouping map with each comm_all_ rank assigned only
    // to itself.
    rankGrouping.clear();
    for (std::size_t i = 0; i < comm_all_.size(); ++i) {
        rankGrouping[i] = std::move(std::vector<int>(1, i));
    }
}

//--------------------------------------------------------------------------------------
ReaderPool::ReaderPool(const oops::Parameter<IoPoolParameters> & ioPoolParams,
               const oops::RequiredPolymorphicParameter
                   <Engines::ReaderParametersBase, Engines::ReaderFactory> & readerParams,
               const eckit::mpi::Comm & commAll, const eckit::mpi::Comm & commTime,
               const util::DateTime & winStart, const util::DateTime & winEnd,
               const std::vector<std::string> & obsVarNames,
               const std::shared_ptr<Distribution> & distribution,
               const std::vector<std::string> & obsGroupVarList)
                   : IoPoolBase(ioPoolParams, commAll, commTime, winStart, winEnd,
                     readerPoolColor, readerNonPoolColor,
                     readerPoolCommName, readerNonPoolCommName),
                     global_nlocs_(0), nlocs_(0), nrecs_(0), source_nlocs_(0),
                     source_nlocs_inside_timewindow_(0), source_nlocs_outside_timewindow_(0),
                     source_nlocs_reject_qc_(0), reader_params_(readerParams),
                     obs_var_names_(obsVarNames), dist_(distribution),
                     obs_group_var_list_(obsGroupVarList) {
    // Save a persistent copy of the JEDI missing value for a string variable that can
    // be used to properly replace a string fill value from the obs source with this
    // JEDI missing value. The replaceFillWithMissing function needs a char * pointing
    // to this copy of the JEDI missing value to transfer that value to the obs space
    // container.
    jedi_missing_value_string_ = std::make_shared<std::string>(util::missingValue(std::string()));

    // TODO(srh) Until the actual reader pool is implemented we need to copy the
    // comm_all_ communicator to the comm_pool_ communicator. The following
    // calls will fall into place for the io pool so use them now to accomplish the
    // copy.

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
    assignRanksToIoPool(nlocs(), rankGrouping);

    // Create the io pool communicator group using the split communicator command.
    createIoPool(rankGrouping);
}

ReaderPool::~ReaderPool() = default;

//--------------------------------------------------------------------------------------
void ReaderPool::load(Group & destGroup) {
    Group fileGroup;
    Engines::ReaderCreationParameters
        createParams(win_start_, win_end_, *comm_pool_, comm_time_,
                     obs_var_names_, is_parallel_io_);
    std::unique_ptr<Engines::ReaderBase> readerEngine =
        Engines::ReaderFactory::create(reader_params_, createParams);

    fileGroup = readerEngine->getObsGroup();

    // Collect the destination from the reader engine instance
    std::ostringstream ss;
    ss << *readerEngine;
    readerSrc_ = ss.str();

    // Check for the required variables in the obs source
    DateTimeFormat dtimeFormat;
    bool emptyFile;
    checkForRequiredVars(fileGroup, readerSrc_, dtimeFormat, emptyFile);

    std::vector<int64_t> dtimeValues;
    std::string dtimeEpoch;
    std::vector<float> lonValues;
    std::vector<float> latValues;
    if (!emptyFile) {
        // Read the datetime variable in the obs source. This function will convert the
        // older formats (offset, string) to the conventional epoch format.
        readSourceDtimeVar(fileGroup, dtimeValues, dtimeEpoch, dtimeFormat);

        // Convert the window start and end times to int64_t offsets from the dtimeEpoch
        // value. This will provide for a very fast "inside the timing window check".
        util::DateTime epochDt;
        convertEpochStringToDtime(dtimeEpoch, epochDt);
        const int64_t windowStart = (win_start_ - epochDt).toSeconds();
        const int64_t windowEnd = (win_end_ - epochDt).toSeconds();

        // Determine which locations will be retained by this process for its obs space
        // source_loc_indices_ holds the original source location index (position in
        // the 1D Location variable) and recnums_ holds the assigned record number.
        //
        // For now, use the comm_all_ (instead of comm_pool_) communicator. We are
        // effectively making the io pool consist of all of the tasks in the comm_all_
        // communicator group.
        setIndexAndRecordNums(fileGroup, &(comm_all_), dist_, dtimeValues,
                              windowStart, windowEnd,
                              readerEngine->applyLocationsCheck(), obs_group_var_list_,
                              lonValues, latValues, source_nlocs_,
                              source_nlocs_inside_timewindow_, source_nlocs_outside_timewindow_,
                              source_nlocs_reject_qc_, loc_indices_, recnums_,
                              global_nlocs_, nlocs_, nrecs_);
    }
    // Check for consistency of the set of nlocs counts.
    ASSERT(source_nlocs_ == source_nlocs_inside_timewindow_ + source_nlocs_outside_timewindow_);
    ASSERT(source_nlocs_ ==
              global_nlocs_ + source_nlocs_outside_timewindow_ + source_nlocs_reject_qc_);

    // Create the memory backend for the destGroup
    // TODO(srh) There needs to be a memory Engine structure created with ObsStore and
    // Hdf5Mem subclasses. Then call the corresponding factory function from here.
    Engines::BackendNames backendName = Engines::BackendNames::ObsStore;  // Hdf5Mem; ObsStore;
    Engines::BackendCreationParameters backendParams;
    // These parameters only matter if Hdf5Mem is the engine selected. ObsStore ignores.
    backendParams.action = Engines::BackendFileActions::Create;
    backendParams.createMode = Engines::BackendCreateModes::Truncate_If_Exists;
    backendParams.fileName = ioda::Engines::HH::genUniqueName();
    backendParams.allocBytes = 1024*1024*50;
    backendParams.flush = false;
    Group backend = constructBackend(backendName, backendParams);

    // Create the ObsGroup and attach the backend.
    destGroup = ObsGroup::generate(backend, {});

    // Copy the ObsSpace ObsGroup to the output file Group.
    ioReadGroup(*this, fileGroup, destGroup, dtimeFormat, dtimeValues, dtimeEpoch,
                lonValues, latValues, is_parallel_io_, emptyFile);
}

//--------------------------------------------------------------------------------------
void ReaderPool::finalize() {
    oops::Log::trace() << "ReaderPool::finalize, start" << std::endl;

    // At this point there are two split communicator groups: one for the io pool and the
    // other for the processes not included in the io pool.
    if (eckit::mpi::hasComm(poolCommName_)) {
        eckit::mpi::deleteComm(poolCommName_);
    }
    if (eckit::mpi::hasComm(nonPoolCommName_)) {
        eckit::mpi::deleteComm(nonPoolCommName_);
    }
    oops::Log::trace() << "ReaderPool::finalize, end" << std::endl;
}

//--------------------------------------------------------------------------------------
void ReaderPool::print(std::ostream & os) const {
  os << readerSrc_ << " (io pool size: " << size_pool_ << ")";
}

}  // namespace ioda
