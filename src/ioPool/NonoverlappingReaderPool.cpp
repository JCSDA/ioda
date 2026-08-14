/*
 * (C) Crown Copyright 2025 Met Office UK
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/ioPool/NonoverlappingReaderPool.h"

#include <algorithm>
#include <memory>
#include <numeric>
#include <optional>  // NOLINT(build/include_order): linter mis-identifies C++ header as C
#include <sstream>

#include "eckit/config/YAMLConfiguration.h"

#include "ioda/distribution/Distribution.h"
#include "ioda/Engines/EngineUtils.h"
#include "ioda/ioPool/ReaderPoolFactory.h"
#include "ioda/ioPool/ReaderPoolUtils.h"

#include "oops/util/Logger.h"
#include "oops/mpi/mpi.h"

namespace ioda {
namespace IoPool {

namespace {

//--------------------------------------------------------------------------------------
/// \brief Return the rank of the MPI process with the lowest rank that holds at least one location,
/// or nullopt if no MPI process has any locations.
std::optional<size_t> lowestRankWithAnyLocations(bool thisRankHasAnyLocations,
                                                 const eckit::mpi::Comm &comm) {
    size_t minRankWithLocations =
        thisRankHasAnyLocations ? comm.rank() : std::numeric_limits<size_t>::max();
    comm.allReduceInPlace(minRankWithLocations, eckit::mpi::Operation::MIN);
    if (minRankWithLocations != std::numeric_limits<size_t>::max())
        return minRankWithLocations;
    else
        // No MPI process has any locations
        return std::nullopt;
}

struct CommDeleter {
    void operator()(eckit::mpi::Comm *comm) {
        if (comm)
            eckit::mpi::deleteComm(comm->name().c_str());
    }
};

}  // namespace

//--------------------------------------------------------------------------------------
// Io pool factory maker
static ReaderPoolMaker<NonoverlappingReaderPool> nonoverlappingReaderPoolMaker(
    "NonoverlappingPool");

//--------------------------------------------------------------------------------------
NonoverlappingReaderPool::NonoverlappingReaderPool(
    const IoPoolParameters & configParams, const ReaderPoolCreationParameters & createParams)
  : ReaderPoolBase(configParams, createParams) {
  isParallelIo_ = true;
}

//--------------------------------------------------------------------------------------
void NonoverlappingReaderPool::initialize() {
  oops::Log::trace() << "NonoverlappingReaderPool::initialize, start" << std::endl;
  if (distribution()->name() != "Identity")
    throw eckit::UserError("Distribution '" + distribution()->name() +
                           "' is incompatible with NonoverlappingReaderPool. "
                           "Use the Identity distribution instead.", Here());
  // We need to copy the commAll_ communicator to the commPool_ communicator.
  // Two of the IoPoolBase virtual functions (setTargetPoolSize and groupRanks) are being
  // overridden here to accomplish this.
  buildIoPool(this->nlocs());
  oops::Log::trace() << "NonoverlappingReaderPool::initialize, end" << std::endl;
}

//--------------------------------------------------------------------------------------
void NonoverlappingReaderPool::load(Group & destGroup) {
  oops::Log::trace() << "NonoverlappingReaderPool::load, start" << std::endl;

  Engines::ReaderCreationParameters
      createParams(timeWindow_, *commPool_, commTime_, obsVarNames_, isParallelIo_);
  std::unique_ptr<Engines::ReaderBase> readerEngine =
      Engines::ReaderFactory::create(readerParams_, createParams);

  Group srcGroup = readerEngine->getObsGroup();

  // Engine initialization
  readerEngine->initialize();

  // Collect the destination from the reader engine instance
  std::ostringstream ss;
  ss << *readerEngine;
  readerSrc_ = ss.str();

  // Check which ranks have read any locations
  const bool thisRankHasLocations =
          srcGroup.vars.exists("Location") &&
          srcGroup.vars.open("Location").getDimensions().dimsCur.front() > 0;
  int anyRankHasLocations = thisRankHasLocations;
  commPool_->allReduceInPlace(anyRankHasLocations, eckit::mpi::Operation::MAX);
  int allRanksHaveLocations = thisRankHasLocations;
  commPool_->allReduceInPlace(allRanksHaveLocations, eckit::mpi::Operation::MIN);

  // Set the emptyFile_ member variable.
  emptyFile_ = anyRankHasLocations == 0;
  if (emptyFile_)
      oops::Log::warning() << "WARNING: Input file " << readerSrc_
                           << " contains zero observations" << std::endl;

  std::vector<int64_t> dtimeValues;
  std::vector<float> lonValues;
  std::vector<float> latValues;
  // Filter locations and assign certain member variables on ranks that have read any locations.
  if (thisRankHasLocations)
      extractGlobalInfoFromSource(oops::mpi::myself(), srcGroup, emptyFile_, timeWindow_,
                                  readerEngine->applyLocationsCheck(),
                                  obsGroupVarList_,
                                  dtimeValues, lonValues, latValues, locIndices_, recNums_,
                                  dtimeEpoch_, globalNlocs_, sourceNlocs_,
                                  sourceNlocsInsideTimeWindow_,
                                  sourceNlocsOutsideTimeWindow_, sourceNlocsRejectQC_);

  if (anyRankHasLocations && !allRanksHaveLocations) {
      // On ranks that haven't read any locations, create the same groups, dimensions, variables
      // and attributes as on the lowest rank that has read some locations.
      copyGroupStructureFromLowestRankWithLocationsToRanksWithoutLocations(thisRankHasLocations,
                                                                           srcGroup);
  }

  // Now we're ready to assign the same member variables as above also on ranks that haven't read
  // any locations.
  if (!thisRankHasLocations)
      extractGlobalInfoFromSource(oops::mpi::myself(), srcGroup, emptyFile_, timeWindow_,
                                  readerEngine->applyLocationsCheck(),
                                  obsGroupVarList_,
                                  dtimeValues, lonValues, latValues, locIndices_, recNums_,
                                  dtimeEpoch_, globalNlocs_, sourceNlocs_,
                                  sourceNlocsInsideTimeWindow_,
                                  sourceNlocsOutsideTimeWindow_, sourceNlocsRejectQC_);

  // Count locations and records held on this rank.
  nlocs_ = locIndices_.size();
  if (recNums_.empty())
    nrecs_ = 0;
  else
    nrecs_ = *std::max_element(recNums_.begin(), recNums_.end()) + 1;

  // Create the memory backend for the destGroup
  // TODO(srh) There needs to be a memory Engine structure created with ObsStore and
  // Hdf5Mem subclasses. Then call the corresponding factory function from here.
  Engines::BackendNames backendName = Engines::BackendNames::ObsStore;
  Engines::BackendCreationParameters backendParams;
  Group backend = constructBackend(backendName, backendParams);

  // Create the ObsGroup and attach the backend.
  destGroup = ObsGroup::generate(backend, {});

  // globalNlocs_ must be the pool-wide total *before* ioReadGroup runs, since it drives
  // chunk-size calculations for the Location-dimensioned variables.
  commPool_->allReduceInPlace(globalNlocs_, eckit::mpi::Operation::SUM);

  // Copy the ObsSpace ObsGroup to the output file Group.
  ioReadGroup(*this, srcGroup, destGroup, dtimeValues, dtimeEpoch_,
              lonValues, latValues, isParallelIo_, emptyFile_);

  // Add up the numbers of locations read/rejected/accepted on all MPI ranks. sourceNlocs_
  // and friends must stay local (per-rank) until after ioReadGroup/readerCopyVarData, which
  // relies on ioPool.sourceNlocs() being this rank's own local count.
  for (size_t *count : {&sourceNlocs_, &sourceNlocsInsideTimeWindow_,
       &sourceNlocsOutsideTimeWindow_, &sourceNlocsRejectQC_})
    commPool_->allReduceInPlace(*count, eckit::mpi::Operation::SUM);

  // Offset location and record indices held on individual MPI ranks to make them globally unique.
  size_t numLocationsOnLowerRanks = nlocs_;
  oops::mpi::exclusiveScan(*commPool_, numLocationsOnLowerRanks);
  size_t numRecordsOnLowerRanks = nrecs_;
  oops::mpi::exclusiveScan(*commPool_, numRecordsOnLowerRanks);

  std::transform(locIndices_.begin(), locIndices_.end(), locIndices_.begin(),
                 [numLocationsOnLowerRanks](size_t loc) { return loc + numLocationsOnLowerRanks; });
  std::transform(recNums_.begin(), recNums_.end(), recNums_.begin(),
                 [numRecordsOnLowerRanks](size_t loc) { return loc + numRecordsOnLowerRanks; });

  // We haven't called the assignRecord() method of the distribution. Instead just tell it
  // how many locations are held on each rank.
  this->distribution()->setNumberLocations(nlocs());

  // Engine finalization
  readerEngine->finalize();
  oops::Log::trace() << "NonoverlappingReaderPool::load, end" << std::endl;
}

void NonoverlappingReaderPool::
copyGroupStructureFromLowestRankWithLocationsToRanksWithoutLocations(bool thisRankHasLocations,
                                                                     Group &group)
{
    const size_t lowestRankWithLocations = *lowestRankWithAnyLocations(thisRankHasLocations,
                                                                       *commPool_);
    const int hasNoLocationsOrIsLowestRankWithLocations =
            !thisRankHasLocations || commPool_->rank() == lowestRankWithLocations;

    // Create a subcommunicator encompassing the lowest rank with locations and all ranks without
    // locations. Wrap it in an unique_ptr that will delete the subcommunicator when it goes out of
    // scope.
    std::unique_ptr<eckit::mpi::Comm, CommDeleter> subcomm(
                &commPool_->split(hasNoLocationsOrIsLowestRankWithLocations,
                                  "NonoverlappingReaderPoolSubcommunicator"));
    if (hasNoLocationsOrIsLowestRankWithLocations) {
        const size_t subcommRankWithLocations = *lowestRankWithAnyLocations(thisRankHasLocations,
                                                                            *subcomm);
        // setGroupStructureOnRanksWithoutLocations() will build a YAML definition of the group
        // structure and set a YAML anchor to the value of the dtimeEpoch_ member variable.
        // Ensure this variable is set also on ranks without locations.
        setDtimeEpochOnRanksWithoutLocations(*subcomm, subcommRankWithLocations);
        setGroupStructureOnRanksWithoutLocations(*subcomm, subcommRankWithLocations, group);
    }

    commPool_->barrier();
}

void NonoverlappingReaderPool::setDtimeEpochOnRanksWithoutLocations(const eckit::mpi::Comm &subcomm,
                                                                    size_t rankWithLocations) {
    oops::mpi::broadcastString(subcomm, dtimeEpoch_, rankWithLocations);
}

void NonoverlappingReaderPool::setGroupStructureOnRanksWithoutLocations(
        const eckit::mpi::Comm &subcomm, size_t rankWithLocations, Group &group) {
    const size_t rank = subcomm.rank();

    // Serialize the structure of the group held by the rank with locations into a YAML string.
    // Broadcast it to the remaining ranks (without locations).
    std::string groupStructureYaml;
    if (rank == rankWithLocations)
        groupStructureYaml = serializeGroupStructure(group, false /*emptyFile?*/);
    oops::mpi::broadcastString(subcomm, groupStructureYaml, rankWithLocations);

    // Append definitions of rank-dependent anchors.
    readerDefineYamlAnchors(*this, groupStructureYaml);

    if (rank != rankWithLocations) {
        // Extend the group held by this rank with all variables, variable groups, dimensions
        // and attributes present on the rank with locations.
        readerDeserializeGroupStructure(*this, group, groupStructureYaml, true /*overwrite*/);
    }

    // Transfer the contents of all dimension variables except Location (which should normally
    // be the same on all ranks) from the rank with locations to other ranks (without locations).

    const eckit::YAMLConfiguration config(groupStructureYaml);
    std::vector<eckit::LocalConfiguration> dimConfigs;
    config.get("dimensions", dimConfigs);
    std::vector<char> buffer;
    for (const eckit::LocalConfiguration & dimConfig : dimConfigs) {
        const std::string dimName = dimConfig.getString("dimension.name");
        if (dimName == "Location")
            continue;

        ioda::Variable dimVar = group.vars.open(dimName);
        const bool varIsStringVector = dimVar.isA<std::string>();
        if (varIsStringVector)
            throw eckit::NotImplemented("NonoverlappingReaderPool does not support dimension "
                                        "variables of type string yet", Here());

        const ioda::Dimensions_t varDataTypeSize = getVarDataTypeSize(dimVar, dimName);
        const std::vector<ioda::Dimensions_t> varShape = dimVar.getDimensions().dimsCur;
        const ioda::Dimensions_t numBytes = varDataTypeSize *
                std::accumulate(varShape.begin(), varShape.end(), 1,
                                std::multiplies<ioda::Dimensions_t>());
        buffer.resize(numBytes);
        if (rank == rankWithLocations)
            readerLoadSourceVarReplaceFill(*this, dimVar, dimName, buffer);
        subcomm.broadcast(buffer.data(), numBytes, rankWithLocations);

        if (rank != rankWithLocations)
            readerSaveDestVar(dimName, buffer, dimVar);
    }
}

//--------------------------------------------------------------------------------------
void NonoverlappingReaderPool::finalize() {
  oops::Log::trace() << "NonoverlappingReaderPool::finalize, start" << std::endl;
  // At this point there are two split communicator groups: one for the io pool and the
  // other for the processes not included in the io pool.
  if (eckit::mpi::hasComm(poolCommName_)) {
    eckit::mpi::deleteComm(poolCommName_);
  }
  if (eckit::mpi::hasComm(nonPoolCommName_)) {
    eckit::mpi::deleteComm(nonPoolCommName_);
  }
  oops::Log::trace() << "NonoverlappingReaderPool::finalize, end" << std::endl;
}

//--------------------------------------------------------------------------------------
void NonoverlappingReaderPool::print(std::ostream & os) const {
  int poolSize = 0;
  if (this->commPool() != nullptr) {
    poolSize = this->commPool()->size();
  }
  os << readerSrc_ << " (io pool size: " << poolSize << ")";
}

//--------------------------------------------------------------------------------------
void NonoverlappingReaderPool::setTargetPoolSize(const int numMpiTasks) {
  // For now this reader is placing all tasks into the pool. This means
  // that the desired number of pool member tasks is simply the size of the
  // commAll communicator group.
  targetPoolSize_ = numMpiTasks;
}

//--------------------------------------------------------------------------------------
void NonoverlappingReaderPool::groupRanks(const int numMpiTasks,
                                          IoPoolGroupMap & rankGrouping) {
  // We need to copy the commAll_ communicator to the commPool_ communicator. This can be
  // accomplished by constructing the rankGrouping map with each commAll_ rank assigned only to
  // itself. Note need to assign empty vector to each element of rankGrouping since these vectors
  // represent the other ranks associated with each pool member. Ie, all ranks are in the pool, and
  // each rank has no associated ranks.
  for (int i = 0; i < numMpiTasks; ++i) {
    rankGrouping[i] = std::vector<int>(0);
  }
}

}  // namespace IoPool
}  // namespace ioda
