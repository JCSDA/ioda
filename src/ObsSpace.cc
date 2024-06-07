/*
 * (C) Copyright 2017-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/ObsSpace.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "eckit/config/Configuration.h"
#include "eckit/exception/Exceptions.h"

#include "oops/mpi/mpi.h"
#include "oops/util/abor1_cpp.h"
#include "oops/util/DateTime.h"
#include "oops/util/Duration.h"
#include "oops/util/Logger.h"
#include "oops/util/missingValues.h"
#include "oops/util/printRunStats.h"
#include "oops/util/Random.h"
#include "oops/util/stringFunctions.h"

#include "ioda/Copying.h"
#include "ioda/distribution/Accumulator.h"
#include "ioda/distribution/DistributionFactory.h"
#include "ioda/distribution/DistributionUtils.h"
#include "ioda/distribution/PairOfDistributions.h"
#include "ioda/Engines/EngineUtils.h"
#include "ioda/Exception.h"
#include "ioda/ioPool/IoPoolParameters.h"
#include "ioda/ioPool/ReaderPoolBase.h"
#include "ioda/ioPool/ReaderPoolFactory.h"
#include "ioda/ioPool/WriterPoolBase.h"
#include "ioda/ioPool/WriterPoolFactory.h"
#include "ioda/Variables/Variable.h"
#include "ioda/Variables/VarUtils.h"

namespace ioda {
namespace {

// If the variable name \p name ends with an underscore followed by a number (potentially a channel
// number), split it at that underscore, store the two parts in \p nameWithoutChannelSuffix and
// \p channel, and return true. Otherwise return false.
bool extractChannelSuffixIfPresent(const std::string &name,
                                   std::string &nameWithoutChannelSuffix, int &channel) {
    const std::string::size_type lastUnderscore = name.find_last_of('_');
    if (lastUnderscore != std::string::npos &&
        name.find_first_not_of("0123456789", lastUnderscore + 1) == std::string::npos) {
        // The variable name has a numeric suffix.
        channel = std::stoi(name.substr(lastUnderscore + 1));
        nameWithoutChannelSuffix = name.substr(0, lastUnderscore);
        return true;
    }
    return false;
}

}  // namespace

// ----------------------------- public functions ------------------------------
// -----------------------------------------------------------------------------
ObsDimInfo::ObsDimInfo() {
    // The following code needs to stay in sync with the ObsDimensionId enum object.
    // The entries are the standard dimension names according to the unified naming convention.
    std::string dimName = "Location";
    dim_id_name_[ObsDimensionId::Location] = dimName;
    dim_id_size_[ObsDimensionId::Location] = 0;
    dim_name_id_[dimName] = ObsDimensionId::Location;

    dimName = "Channel";
    dim_id_name_[ObsDimensionId::Channel] = dimName;
    dim_id_size_[ObsDimensionId::Channel] = 0;
    dim_name_id_[dimName] = ObsDimensionId::Channel;
}

ObsDimensionId ObsDimInfo::get_dim_id(const std::string & dimName) const {
    return dim_name_id_.at(dimName);
}

std::string ObsDimInfo::get_dim_name(const ObsDimensionId dimId) const {
    return dim_id_name_.at(dimId);
}

std::size_t ObsDimInfo::get_dim_size(const ObsDimensionId dimId) const {
    return dim_id_size_.at(dimId);
}

void ObsDimInfo::set_dim_size(const ObsDimensionId dimId, std::size_t dimSize) {
    dim_id_size_.at(dimId) = dimSize;
}

// -----------------------------------------------------------------------------
/*!
 * \details Config based constructor for an ObsSpace object. This constructor will read
 *          in from the obs file and transfer the variables into the obs container. Obs
 *          falling outside the DA timing window, specified by bgn and end, will be
 *          discarded before storing them in the obs container.
 *
 * \param[in] config ECKIT configuration segment holding obs types specs
 * \param[in] comm   MPI communicator containing all processes that hold the observations for a
 *                   given time slot or sub-window.
 * \param[in] bgn    DateTime object holding the start of the DA timing window
 * \param[in] end    DateTime object holding the end of the DA timing window
 * \param[in] time   MPI communicator across time so that the 2D array of processes represented by
 *                   the product of the comm and time communicators hold all observations in the
 *                   ObsSpace.
 */
ObsSpace::ObsSpace(const eckit::Configuration & config, const eckit::mpi::Comm & comm,
                   const util::TimeWindow timeWindow,
                   const eckit::mpi::Comm & timeComm)
                     : oops::ObsSpaceBase(config, comm, timeWindow),
                       timeWindow_(timeWindow),
                       commMPI_(comm), commTime_(timeComm),
                       source_nlocs_(0), gnlocs_(0), gnlocs_outside_timewindow_(0),
                       gnlocs_reject_qc_(0), nrecs_(0), obs_group_(),
                       obs_params_(config, timeWindow_, comm, timeComm), obsvars_()
{
    // Determine if run stats should be dumped out from the environment variable
    // IODA_PRINT_RUNSTATS.
    //    IODA_PRINT_RUNSTATS == 0 -> disable printing of run stats
    //    IODA_PRINT_RUNSTATS > 0 -> enable printing of run stats
    //         Leave open the possibility of setting verbosity levels in this case
    //             1 - print runstats at beginning and end of both ObsSpace constructor
    //                 and ObsSpace save function.
    //            >1 - for now, same as level 1
    char * iodaPrintRunstats = std::getenv("IODA_PRINT_RUNSTATS");
    if (iodaPrintRunstats == nullptr) {
        print_run_stats_ = 0;
    } else {
        // strtol returns zero if a conversion from the input string could not be made
        // this will result in the print stats being disabled
        print_run_stats_ = std::strtol(iodaPrintRunstats, nullptr, 10);
    }

    // Read the obs space name
    obsname_ = obs_params_.top_level_.obsSpaceName;
    if (print_run_stats_ > 0) {
        util::printRunStats("ioda::ObsSpace::ObsSpace: start " + obsname_ + ": ", true, comm);
    }

    // Create an MPI distribution object
    const auto & distParams = obs_params_.top_level_.distribution.value().params.value();
    dist_ = DistributionFactory::create(obs_params_.comm(), distParams);

    // Create a vector of obsdatain configs (one per input file) for the loop below
    std::vector<eckit::LocalConfiguration> obsDataInConfigs =
        expandInputFileConfigs(obs_params_.top_level_.obsDataIn.value());

    // Load the obs space data (into obs_group_) from the obs source (file or generator)
    dim_info_.set_dim_size(ObsDimensionId::Location, 0);
    indx_.clear();
    recnums_.clear();
    ObsGroup tempObsGroup;
    ObsSourceStats obsSourceStats;
    for (int i = 0; i < obsDataInConfigs.size(); ++i) {
        load(obsDataInConfigs[i], tempObsGroup, obsSourceStats);
        appendObsGroup(tempObsGroup, obsSourceStats);
    }

    // Assign Location variable with the source index numbers that were kept
    assignLocationValues();

    // The distribution object has a notion of patch obs which are the observations
    // "owned" by the corresponding obs space. When an overlapping distribution (eg, Halo)
    // is used, there is a need to identify all the unique obs (ie, locations) for functions
    // that access obs across the MPI tasks. Computing an ObsVector dot product, and
    // output IO are two examples. The ownership (patch) marks which obs participate in
    // the MPI distributed functions, and collectively make up a total set of obs that contain
    // no duplicates.
    //
    // Take the Halo distribution for an example. Each MPI task holds locations (obs) that
    // are within a horizontal radius from a given center point. This brings up the situation
    // where multiple obs spaces (geographic neighbors) can both contain the same locations
    // since their spatial coverages can overlap. The ownership is given to the MPI task whose
    // center is closer to that location. That way one MPI task owns the obs and the other
    // does not which is then used to make sure the duplicate location is not used in
    // MPI collective operations (such as the dot product function).
    dist_->computePatchLocs();

    // Get list of observed variables
    // Either read from yaml list, use all variables in input file if 'obsdatain' is specified
    // or set to simulated variables if 'generate' is specified.
    const bool usingObsGenerator =
      ((obs_params_.top_level_.obsDataIn.value().engine.value()
                   .engineParameters.value().type.value() == "GenList") ||
       (obs_params_.top_level_.obsDataIn.value().engine.value()
                   .engineParameters.value().type.value() == "GenRandom"));

    if (obs_params_.top_level_.ObservedVars.value().size()
            + obs_params_.top_level_.derivedSimVars.value().size() != 0) {
      // Read from yaml
      obsvars_ = obs_params_.top_level_.ObservedVars;
    } else if (usingObsGenerator) {
      obsvars_ = obs_params_.top_level_.simVars;
    } else {
      // Use all variables found in the ObsValue group in the file. If there is no ObsValue
      // group (rare), then copy the simulated variables list.
      if (obs_group_.exists("ObsValue")) {
        Group obsValueGroup = obs_group_.open("ObsValue");
        const std::vector<std::string>
                allObsVars = obsValueGroup.listObjects<ObjectType::Variable>(false);
        // ToDo (JAW): Get the channels from the input file (currently using the ones from simVars)
        std::vector<int> channels = obs_params_.top_level_.simVars.value().channels();
        oops::ObsVariables obVars(allObsVars, channels);
        obsvars_ = obVars;
      } else {
        obsvars_ = obs_params_.top_level_.simVars;
      }
    }

    // Store the intial list of variables read from the yaml of input file.
    initial_obsvars_ = obsvars_;

    // Add derived varible names to observed variables list
    if (obs_params_.top_level_.derivedSimVars.value().size() != 0) {
      // As things stand, this assert cannot fail, since both variables take the list of channels
      // from the same "channels" YAML option.
      ASSERT(obs_params_.top_level_.derivedSimVars.value().channels() == obsvars_.channels());
      obsvars_ += obs_params_.top_level_.derivedSimVars;
      derived_obsvars_ = obs_params_.top_level_.derivedSimVars;
    }

    // Get list of variables to be simulated
    assimvars_ = obs_params_.top_level_.simVars;


    oops::Log::info() << this->obsname() << " processed vars: " << obsvars_ << std::endl;
    oops::Log::info() << this->obsname() << " assimilated vars: " << assimvars_ << std::endl;

    for (size_t jv = 0; jv < assimvars_.size(); ++jv) {
      if (!obsvars_.has(assimvars_[jv])) {
          throw eckit::UserError(assimvars_[jv] + " is specified as a simulated variable"
                                 " but it has not been specified as an observed or"
                                 " a derived variable." , Here());
      }
    }

    // Construct the recidx_ map
    buildRecIdx();

    fillChanNumToIndexMap();

    if (obs_params_.top_level_.obsExtend.value() != boost::none) {
        extendObsSpace(*(obs_params_.top_level_.obsExtend.value()));
    }

    createMissingObsErrors();

    oops::Log::debug() << obsname() << ": " << globalNumLocsOutsideTimeWindow()
      << " observations are outside of time window out of " << sourceNumLocs() << std::endl;
    oops::Log::debug() << obsname() << ": " << globalNumLocsRejectQC()
      << " observations were rejected by QC checks out of " << sourceNumLocs() << std::endl;

    oops::Log::trace() << "ObsSpace::ObsSpace constructed name = " << obsname() << std::endl;
    if (print_run_stats_ > 0) {
        util::printRunStats("ioda::ObsSpace::ObsSpace: end " + obsname_ + ": ", true, comm);
    }
}

// -----------------------------------------------------------------------------
void ObsSpace::save() {
    if (obs_params_.top_level_.obsDataOut.value() != boost::none) {
        if (print_run_stats_ > 0) {
            util::printRunStats("ioda::ObsSpace::save: start " + obsname_ + ": ", true, comm());
        }

        std::vector<bool> patchObsVec(nlocs());
        dist_->patchObs(patchObsVec);

        IoPool::WriterPoolCreationParameters createParams(
            obs_params_.comm(), obs_params_.timeComm() ,
            obs_params_.top_level_.obsDataOut.value()->engine.value().engineParameters,
            patchObsVec);
        std::unique_ptr<IoPool::WriterPoolBase> writePool =
            IoPool::WriterPoolFactory::create(obs_params_.top_level_.ioPool, createParams);

        writePool->initialize();
        writePool->save(obs_group_);
        // Wait for all processes to finish the save call so that we know the file
        // is complete and closed.
        oops::Log::info() << obsname() << ": save database to " << *writePool << std::endl;
        this->comm().barrier();
        writePool->finalize();

        // Call the mpi barrier command here to force all processes to wait until
        // all processes have finished writing their files. This is done to prevent
        // the early processes continuing and potentially executing their obs space
        // destructor before others finish writing. This situation is known to have
        // issues with hdf file handles getting deallocated before some of the MPI
        // processes are finished with them.
        this->comm().barrier();
        if (print_run_stats_ > 0) {
            util::printRunStats("ioda::ObsSpace::save: end " + obsname_ + ": ", true, comm());
        }
    } else {
        oops::Log::info() << obsname() << " :  no output" << std::endl;
    }
}

// -----------------------------------------------------------------------------
std::size_t ObsSpace::nvars() const {
    // Nvars is the number of variables in the ObsValue group. By querying
    // the ObsValue group, nvars will keep track of new variables that are added
    // during a run.
    // Some of the generators, upon construction, do not create variables in ObsValue
    // since the MakeObs function will do that. In this case, they instead create
    // error estimates in ObsError with the expectation that ObsValue will be filled
    // in later. So upon construction, nvars will be the number of variables in ObsError
    // instead of ObsValue.
    // Because of the generator case above, query ObsValue first and if ObsValue doesn't
    // exist query ObsError.
    std::size_t numVars = 0;
    if (obs_group_.exists("ObsValue")) {
         numVars = obs_group_.open("ObsValue").vars.list().size();
    } else if (obs_group_.exists("ObsError")) {
         numVars = obs_group_.open("ObsError").vars.list().size();
    }
    return numVars;
}

// -----------------------------------------------------------------------------
const std::vector<std::string> & ObsSpace::obs_group_vars() const {
    return obs_params_.top_level_.obsDataIn.value().obsGrouping.value().obsGroupVars;
}

// -----------------------------------------------------------------------------
std::string ObsSpace::obs_sort_var() const {
    return obs_params_.top_level_.obsDataIn.value().obsGrouping.value().obsSortVar;
}

// -----------------------------------------------------------------------------
std::string ObsSpace::obs_sort_group() const {
    return obs_params_.top_level_.obsDataIn.value().obsGrouping.value().obsSortGroup;
}

// -----------------------------------------------------------------------------
std::string ObsSpace::obs_sort_order() const {
    return obs_params_.top_level_.obsDataIn.value().obsGrouping.value().obsSortOrder;
}

// -----------------------------------------------------------------------------
/*!
 * \details This method checks for the existence of the group, name combination
 *          in the obs container. If the combination exists, "true" is returned,
 *          otherwise "false" is returned.
 */
bool ObsSpace::has(const std::string & group, const std::string & name, bool skipDerived) const {
    // For an empty obs space, make it appear that any variable exists.
    if (this->empty()) {
        return true;
    } else {
        // For backward compatibility, recognize and handle appropriately variable names with
        // channel suffixes.
        std::string nameToUse;
        std::vector<int> chanSelectToUse;
        splitChanSuffix(group, name, { }, nameToUse, chanSelectToUse, skipDerived);
        return obs_group_.vars.exists(fullVarName(group, nameToUse)) ||
          (!skipDerived && obs_group_.vars.exists(fullVarName("Derived" + group, nameToUse)));
    }
}

// -----------------------------------------------------------------------------
ObsDtype ObsSpace::dtype(const std::string & group, const std::string & name,
                         bool skipDerived) const {
    // Set the type to None if there is no type from the backend
    ObsDtype VarType = ObsDtype::None;

    // Want to make an empty obs space look like any variable exists. Use the special
    // data type marker of "Empty" to distinguish from "None" which is the marker for
    // when the backend doesn't know what type the variable is.
    if (this->empty()) {
        VarType = ObsDtype::Empty;
    } else {
        // For backward compatibility, recognize and handle appropriately variable names with
        // channel suffixes.
        std::string nameToUse;
        std::vector<int> chanSelectToUse;
        splitChanSuffix(group, name, { }, nameToUse, chanSelectToUse, skipDerived);

        std::string groupToUse = "Derived" + group;
        if (skipDerived || !obs_group_.vars.exists(fullVarName(groupToUse, nameToUse)))
          groupToUse = group;

        if (has(groupToUse, nameToUse, skipDerived)) {
            const std::string varNameToUse = fullVarName(groupToUse, nameToUse);
            Variable var = obs_group_.vars.open(varNameToUse);
            VarUtils::switchOnSupportedVariableType(
                  var,
                  [&] (int)   {
                      VarType = ObsDtype::Integer;
                  },
                  [&] (int64_t)   {
                      if ((group == "MetaData") && (nameToUse == "dateTime")) {
                          VarType = ObsDtype::DateTime;
                          // TODO(srh) Workaround to cover when datetime was stored
                          // as a util::DateTime object (back when the obs space container
                          // was a boost::multiindex container). For now, ioda accepts
                          // int64_t offset times with its epoch datetime representation.
                      } else {
                          VarType = ObsDtype::Integer_64;
                      }
                  },
                  [&] (float) {
                      VarType = ObsDtype::Float;
                  },
                  [&] (std::string) {
                      if ((group == "MetaData") && (nameToUse == "datetime")) {
                          // TODO(srh) Workaround to cover when datetime was stored
                          // as a util::DateTime object (back when the obs space container
                          // was a boost::multiindex container). For now ioda accepts
                          // string datetime representation.
                          VarType = ObsDtype::DateTime;
                      } else {
                          VarType = ObsDtype::String;
                      }
                  },
                  [&] (char) {
                      VarType = ObsDtype::Bool;
                  },
                  VarUtils::ThrowIfVariableIsOfUnsupportedType(varNameToUse));
        }
    }
    return VarType;
}

// -----------------------------------------------------------------------------
// TODO(srh) For now we will make it look like any variable exists when we have
// read in an empty input file. The empty input file can minimally contain only
// the dimension Location set to zero. This is done so that r2d2 can use the exact
// same empty file for any obs type. If we didn't fake the existence of any variable
// then r2d2 would have to supply an empty input file tailored to each obs type which
// contains all of the MetaData variables expected for a particular obs type.
//
// To accomplish making it look like any variables exists, we will have get_db simply
// return immediately with an zero length vector without actually accessing the
// obs_group_ container.
//
// In the future, we may want r2d2 to supply obs type specific empty files. If and when
// that happens, we can remove this "fake-it" method.
void ObsSpace::get_db(const std::string & group, const std::string & name,
                     std::vector<int> & vdata,
                     const std::vector<int> & chanSelect, bool skipDerived) const {
    if (this->empty()) {
        vdata.resize(0);
    } else {
        loadVar<int>(group, name, chanSelect, vdata, skipDerived);
    }
}

void ObsSpace::get_db(const std::string & group, const std::string & name,
                     std::vector<int64_t> & vdata,
                     const std::vector<int> & chanSelect, bool skipDerived) const {
    if (this->empty()) {
        vdata.resize(0);
    } else {
        loadVar<int64_t>(group, name, chanSelect, vdata, skipDerived);
    }
}

void ObsSpace::get_db(const std::string & group, const std::string & name,
                     std::vector<float> & vdata,
                     const std::vector<int> & chanSelect, bool skipDerived) const {
    if (this->empty()) {
        vdata.resize(0);
    } else {
        loadVar<float>(group, name, chanSelect, vdata, skipDerived);
    }
}

void ObsSpace::get_db(const std::string & group, const std::string & name,
                     std::vector<double> & vdata,
                     const std::vector<int> & chanSelect, bool skipDerived) const {
    if (this->empty()) {
        vdata.resize(0);
    } else {
        // load the float values from the database and convert to double
        std::vector<float> floatData;
        loadVar<float>(group, name, chanSelect, floatData, skipDerived);
        ConvertVarType<float, double>(floatData, vdata);
    }
}

void ObsSpace::get_db(const std::string & group, const std::string & name,
                     std::vector<std::string> & vdata,
                     const std::vector<int> & chanSelect, bool skipDerived) const {
    if (this->empty()) {
        vdata.resize(0);
    } else {
        loadVar<std::string>(group, name, chanSelect, vdata, skipDerived);
    }
}

void ObsSpace::get_db(const std::string & group, const std::string & name,
                     std::vector<util::DateTime> & vdata,
                     const std::vector<int> & chanSelect, bool skipDerived) const {
    if (this->empty()) {
        vdata.resize(0);
    } else {
        std::vector<int64_t> timeOffsets;
        loadVar<int64_t>(group, name, chanSelect, timeOffsets, skipDerived);
        Variable dtVar = obs_group_.vars.open(group + std::string("/") + name);
        util::DateTime epochDt = getEpochAsDtime(dtVar);
        vdata = convertEpochDtToDtime(epochDt, timeOffsets);
    }
}

void ObsSpace::get_db(const std::string & group, const std::string & name,
                      std::vector<bool> & vdata,
                      const std::vector<int> & chanSelect, bool skipDerived) const {
    if (this->empty()) {
        vdata.resize(0);
    } else {
        // Boolean variables are currently stored internally as arrays of bytes (with each
        // byte holding one element of the variable).
        // TODO(wsmigaj): Store them as arrays of bits instead, at least in the ObsStore
        // backend, to reduce memory consumption and speed up the get_db and put_db functions.
        std::vector<char> charData(vdata.size());
        loadVar<char>(group, name, chanSelect, charData, skipDerived);
        vdata.assign(charData.begin(), charData.end());
    }
}

// -----------------------------------------------------------------------------
void ObsSpace::put_db(const std::string & group, const std::string & name,
                     const std::vector<int> & vdata,
                     const std::vector<std::string> & dimList) {
    saveVar(group, name, vdata, dimList);
}

void ObsSpace::put_db(const std::string & group, const std::string & name,
                     const std::vector<int64_t> & vdata,
                     const std::vector<std::string> & dimList) {
    saveVar(group, name, vdata, dimList);
}

void ObsSpace::put_db(const std::string & group, const std::string & name,
                     const std::vector<float> & vdata,
                     const std::vector<std::string> & dimList) {
    saveVar(group, name, vdata, dimList);
}

void ObsSpace::put_db(const std::string & group, const std::string & name,
                     const std::vector<double> & vdata,
                     const std::vector<std::string> & dimList) {
    // convert to float, then save to the database
    std::vector<float> floatData;
    ConvertVarType<double, float>(vdata, floatData);
    saveVar(group, name, floatData, dimList);
}

void ObsSpace::put_db(const std::string & group, const std::string & name,
                     const std::vector<std::string> & vdata,
                     const std::vector<std::string> & dimList) {
    saveVar(group, name, vdata, dimList);
}

void ObsSpace::put_db(const std::string & group, const std::string & name,
                     const std::vector<util::DateTime> & vdata,
                     const std::vector<std::string> & dimList) {
    // Make sure the variable exists before calling saveVar. Doing it this way instead
    // of through the openCreateVar call in saveVar because of the need to get the
    // epoch value for converting the data before calling saveVar. Use the epoch DateTime
    // parameter for the units if creating a new variable.
    Variable dtVar;
    openCreateEpochDtimeVar(group, name, gnlocs_, obs_params_.top_level_.epochDateTime,
                            dtVar, obs_group_.vars);
    util::DateTime epochDtime = getEpochAsDtime(dtVar);
    std::vector<int64_t> timeOffsets = convertDtimeToTimeOffsets(epochDtime, vdata);
    saveVar(group, name, timeOffsets, dimList);
}

void ObsSpace::put_db(const std::string & group, const std::string & name,
                      const std::vector<bool> & vdata,
                      const std::vector<std::string> & dimList) {
    // Boolean variables are currently stored internally as arrays of bytes (with each byte
    // holding one element of the variable).
    // TODO(wsmigaj): Store them as arrays of bits instead, at least in the ObsStore backend,
    // to reduce memory consumption and speed up the get_db and put_db functions.
    std::vector<char> boolsAsBytes(vdata.begin(), vdata.end());
    saveVar(group, name, boolsAsBytes, dimList);
}

// -----------------------------------------------------------------------------
const ObsSpace::RecIdxIter ObsSpace::recidx_begin() const {
  return recidx_.begin();
}

// -----------------------------------------------------------------------------
const ObsSpace::RecIdxIter ObsSpace::recidx_end() const {
  return recidx_.end();
}

// -----------------------------------------------------------------------------
bool ObsSpace::recidx_has(const std::size_t recNum) const {
  RecIdxIter irec = recidx_.find(recNum);
  return (irec != recidx_.end());
}

// -----------------------------------------------------------------------------
std::size_t ObsSpace::recidx_recnum(const RecIdxIter & irec) const {
  return irec->first;
}

// -----------------------------------------------------------------------------
const std::vector<std::size_t> & ObsSpace::recidx_vector(const RecIdxIter & irec) const {
  return irec->second;
}

// -----------------------------------------------------------------------------
const std::vector<std::size_t> & ObsSpace::recidx_vector(const std::size_t recNum) const {
  RecIdxIter Irec = recidx_.find(recNum);
  if (Irec == recidx_.end()) {
    std::string ErrMsg =
      "ObsSpace::recidx_vector: Record number, " + std::to_string(recNum) +
      ", does not exist in record index map.";
    ABORT(ErrMsg);
  }
  return Irec->second;
}

// -----------------------------------------------------------------------------
std::vector<std::size_t> ObsSpace::recidx_all_recnums() const {
  std::vector<std::size_t> RecNums(nrecs_);
  std::size_t recnum = 0;
  for (RecIdxIter Irec = recidx_.begin(); Irec != recidx_.end(); ++Irec) {
    RecNums[recnum] = Irec->first;
    recnum++;
  }
  return RecNums;
}

// -----------------------------------------------------------------------------
void ObsSpace::reduce(const ioda::CompareAction compareAction, const int threshold,
                      const std::vector<int> & checkValues) {
    ASSERT(checkValues.size() == this->nlocs());
    // Transform the reduce specs into a boolean vector where true means keep,
    // and false means remove.
    std::vector<bool> keepLocs;
    generateLocationsToKeep(compareAction, threshold, checkValues, keepLocs);
    this->reduce(keepLocs);
}

// -----------------------------------------------------------------------------
void ObsSpace::reduce(const std::vector<bool> & keepLocs) {
    // Reduce the data values stored in the obs_group_ container
    const std::size_t newNlocs = reduceVarDataValues(keepLocs);

    // Resize the obs_group_ container according to the newNlocs value
    Variable locVar = obs_group_.vars.open("Location");
    obs_group_.resize({std::pair<Variable, Dimensions_t>(locVar, newNlocs)});
    dim_info_.set_dim_size(ObsDimensionId::Location, newNlocs);

    // Update the nrecs_ and recidx_ data members according to the reduce
    // (ie, removed) locations.
    adjustDataMembersAfterReduce(keepLocs);

    // Reduce all the associated data structures
    for (auto & data : obs_space_associated_) {
      data.get().reduce(keepLocs);
    }
}

// ----------------------------- private functions -----------------------------
/*!
 * \details This method provides a way to print an ObsSpace object in an output
 *          stream.
 */
void ObsSpace::print(std::ostream & os) const {
  std::size_t totalNlocs = this->globalNumLocs();
  std::size_t nvars = this->obsvariables().size();
  std::size_t nobs = totalNlocs * nvars;

  os << obsname() << ": nlocs: " << totalNlocs
     << ", nvars: " << nvars << ", nobs: " << nobs;
}

// -----------------------------------------------------------------------------
void ObsSpace::assignLocationValues() {
    // Only do the assignment if the Location variable exists and if there
    // are more that zero locations.
    if ((indx_.size() > 0) && (obs_group_.vars.exists("Location"))) {
        // (TODO: srh) the location variable is getting defined as different types
        // by the ioda converters. The converters need to converge on the convention
        // type which is int64_t. But for now, Location can be int64_t, int, float.
        // The static_cast from size_t to int, int64_t and float should be safe, but
        // want to eventually get rid of this and expect only int64_t. The safety of
        // the static_cast exists because the max location index value is limited by
        // the type in the input file (float: 6 or 7 digits of precision,
        // int: ~2 billion, etc) and we are static_cast'ing to the same type as what
        // is in the file.
        Variable locVar = obs_group_.vars.open("Location");
        if (locVar.isA<int>()) {
            std::vector<int> locValues(indx_.size());
            for (std::size_t i = 0; i < indx_.size(); ++i) {
                locValues[i] = static_cast<int>(indx_[i]);
            }
            locVar.write<int>(locValues);
        } else if (locVar.isA<float>()) {
            std::vector<float> locValues(indx_.size());
            for (std::size_t i = 0; i < indx_.size(); ++i) {
                locValues[i] = static_cast<float>(indx_[i]);
            }
            locVar.write<float>(locValues);
        } else if (locVar.isA<int64_t>()) {
            std::vector<int64_t> locValues(indx_.size());
            for (std::size_t i = 0; i < indx_.size(); ++i) {
                locValues[i] = static_cast<int64_t>(indx_[i]);
            }
            locVar.write<int64_t>(locValues);
        } else {
            throw Exception("Location variable has unexpected data type", ioda_Here());
        }
    }
}

// -----------------------------------------------------------------------------
void ObsSpace::load(const eckit::LocalConfiguration & obsDataInConfig,
                    ioda::ObsGroup & destObsGroup, ObsSourceStats & obsSourceStats) {
    if (print_run_stats_ > 0) {
        util::printRunStats("ioda::ObsSpace::load: start " + obsname_ + ": ", true, comm());
    }

    // Open the source of the data for initializing the destObsGroup
    // Temporarily allow for the new reader to be selected. This is done to allow
    // the new reader to be developed in parallel with the current reader. When the
    // new reader becomes fully functional it will replace the current reader.
    ObsDataInParameters readerParams;
    readerParams.deserialize(obsDataInConfig);
    IoPool::ReaderPoolCreationParameters createParams(
        obs_params_.comm(), obs_params_.timeComm(),
        readerParams.engine.value().engineParameters, obs_params_.timeWindow(),
        obs_params_.top_level_.simVars.value().variables(), dist_,
        obs_params_.top_level_.obsDataIn.value().obsGrouping.value().obsGroupVars,
        obs_params_.top_level_.obsDataIn.value().prepType);

    std::unique_ptr<IoPool::ReaderPoolBase> readPool =
            IoPool::ReaderPoolFactory::create(obs_params_.top_level_.ioPool, createParams);

    // Make sure the initialize step completes on all tasks before moving on to the
    // load step (with the barrier call). This is especially important for the case
    // where files are being created in the initialize step that are used in the load step.
    readPool->initialize();
    this->comm().barrier();

    // Transfer the obs data from the source to the obs space container (ObsGroup)
    readPool->load(destObsGroup);

    // Record location and record information
    obsSourceStats.nlocs = readPool->nlocs();
    obsSourceStats.nrecs = readPool->nrecs();
    obsSourceStats.locIndices = readPool->index();
    obsSourceStats.recNums = readPool->recnums();

    // After loading the obs data, gnlocs_ and gnlocs_outside_timewindow_
    // are set representing the entire obs source. This is because they are calculated
    // before distributing the data to all of the MPI tasks.
    obsSourceStats.gNlocs = readPool->globalNlocs();
    obsSourceStats.gNlocsOutsideTimewindow = readPool->sourceNlocsOutsideTimeWindow();
    obsSourceStats.gNlocsRejectQc = readPool->sourceNlocsRejectQC();
    obsSourceStats.sourceNlocs = readPool->sourceNlocs();

    // Wait for all processes to finish the load call so that we know the file
    // is complete and closed.
    oops::Log::info() << obsname() << ": read database from " << *readPool << std::endl;
    this->comm().barrier();
    readPool->finalize();

    if (print_run_stats_ > 0) {
        util::printRunStats("ioda::ObsSpace::load: end " + obsname_ + ": ", true, comm());
    }
}

// -----------------------------------------------------------------------------
void ObsSpace::appendObsGroup(const ObsGroup & appendObsGroup, ObsSourceStats & obsSourceStats) {
    // append the ObsGroup (save for now)
    obs_group_.append(appendObsGroup);

    // accumulate stats from the obs source
    nrecs_ += obsSourceStats.nrecs;
    gnlocs_ += obsSourceStats.gNlocs;
    gnlocs_outside_timewindow_ += obsSourceStats.gNlocsOutsideTimewindow;
    gnlocs_reject_qc_ += obsSourceStats.gNlocsRejectQc;
    source_nlocs_ += obsSourceStats.sourceNlocs;
    indx_.insert(indx_.end(), obsSourceStats.locIndices.begin(), obsSourceStats.locIndices.end());
    recnums_.insert(recnums_.end(), obsSourceStats.recNums.begin(), obsSourceStats.recNums.end());

    // Record locations and channels dimension sizes
    // The HDF library has an issue when a dimension marked UNLIMITED is queried for its
    // size a zero is returned instead of the proper current size. As a workaround for this
    // ask the frame how many locations it kept instead of asking the Location dimension for
    // its size.
    std::size_t nlocs = dim_info_.get_dim_size(ObsDimensionId::Location) + obsSourceStats.nlocs;
    dim_info_.set_dim_size(ObsDimensionId::Location, nlocs);

    std::string ChannelName = dim_info_.get_dim_name(ObsDimensionId::Channel);
    if (obs_group_.vars.exists(ChannelName)) {
        std::size_t nChans = obs_group_.vars.open(ChannelName).getDimensions().dimsCur[0];
        dim_info_.set_dim_size(ObsDimensionId::Channel, nChans);
    }
}

// -----------------------------------------------------------------------------
std::vector<eckit::LocalConfiguration> ObsSpace::expandInputFileConfigs(
                                           const ObsDataInParameters & obsDatainParams) {
    // TODO(srh) For now we are still allowing only one input file, so it is sufficient
    // to just create a single LocalConfiguration (vector of size 1) to pass to the
    // ObsSpace::load function. Eventually we want an entry in the vector of
    // LocalConfiguration for each specified input file.
    std::vector<eckit::LocalConfiguration> obsDataInConfigs(1);
    obsDatainParams.serialize(obsDataInConfigs[0]);

    return obsDataInConfigs;
}


// -----------------------------------------------------------------------------
void ObsSpace::resizeLocation(const Dimensions_t LocationSize, const bool append) {
    Variable LocationVar = obs_group_.vars.open(dim_info_.get_dim_name(ObsDimensionId::Location));
    Dimensions_t LocationResize;
    if (append) {
        LocationResize = LocationVar.getDimensions().dimsCur[0] + LocationSize;
    } else {
        LocationResize = LocationSize;
    }
    obs_group_.resize(
        { std::pair<Variable, Dimensions_t>(LocationVar, LocationResize) });
}

// -----------------------------------------------------------------------------

template<typename VarType>
void ObsSpace::loadVar(const std::string & group, const std::string & name,
                       const std::vector<int> & chanSelect,
                       std::vector<VarType> & varValues,
                       bool skipDerived) const {
    // For backward compatibility, recognize and handle appropriately variable names with
    // channel suffixes.
    std::string nameToUse;
    std::vector<int> chanSelectToUse;
    splitChanSuffix(group, name, chanSelect, nameToUse, chanSelectToUse);

    // Prefer variables from Derived* groups.
    std::string groupToUse = "Derived" + group;
    if (skipDerived || !obs_group_.vars.exists(fullVarName(groupToUse, nameToUse)))
      groupToUse = group;

    // Try to open the variable.
    ioda::Variable var = obs_group_.vars.open(fullVarName(groupToUse, nameToUse));

    std::string ChannelVarName = this->get_dim_name(ObsDimensionId::Channel);

    // In the following code, assume that if a variable has channels, the
    // Channel dimension will be the second dimension.
    if (obs_group_.vars.exists(ChannelVarName)) {
        Variable ChannelVar = obs_group_.vars.open(ChannelVarName);
        if (var.getDimensions().dimensionality > 1) {
            if (var.isDimensionScaleAttached(1, ChannelVar) &&
               (chanSelectToUse.size() > 0)) {
                // This variable has Channel as the second dimension, and channel
                // selection has been specified. Build selection objects based on the
                // channel numbers. For now, select all locations (first dimension).
                const std::size_t ChannelDimIndex = 1;
                Selection memSelect;
                Selection obsGroupSelect;
                const std::size_t numElements = createChannelSelections(
                      var, ChannelDimIndex, chanSelectToUse, memSelect, obsGroupSelect);

                var.read<VarType>(varValues, memSelect, obsGroupSelect);
                varValues.resize(numElements);
            } else {
              // Not a radiance variable, just read in the whole variable
              var.read<VarType>(varValues);
            }
        } else {
            // Not a radiance variable, just read in the whole variable
            var.read<VarType>(varValues);
        }
    } else {
        // Not a radiance variable, just read in the whole variable
        var.read<VarType>(varValues);
    }
}

// -----------------------------------------------------------------------------

template<typename VarType>
void ObsSpace::saveVar(const std::string & group, std::string name,
                      const std::vector<VarType> & varValues,
                      const std::vector<std::string> & dimList) {
    // For backward compatibility, recognize and handle appropriately variable names with
    // channel suffixes.

    std::vector<int> channels;

    const std::string ChannelVarName = this->get_dim_name(ObsDimensionId::Channel);
    if (group != "MetaData" && obs_group_.vars.exists(ChannelVarName)) {
        // If the variable does not already exist and its name ends with an underscore followed by
        // a number, interpret the latter as a channel number selecting a slice of the "Channel"
        // dimension.
        std::string nameToUse;
        splitChanSuffix(group, name, {}, nameToUse, channels);
        name = std::move(nameToUse);
    }

    const std::string fullName = fullVarName(group, name);

    std::vector<std::string> dimListToUse = dimList;
    if (!obs_group_.vars.exists(fullName) && !channels.empty()) {
        // Append "channels" to the dimensions list if not already present.
        const size_t ChannelDimIndex =
            std::find(dimListToUse.begin(), dimListToUse.end(), ChannelVarName) -
            dimListToUse.begin();
        if (ChannelDimIndex == dimListToUse.size())
            dimListToUse.push_back(ChannelVarName);
    }
    Variable var = openCreateVar<VarType>(fullName, dimListToUse);

    if (channels.empty()) {
        var.write<VarType>(varValues);
    } else {
        // Find the index of the Channel dimension
        Variable ChannelVar = obs_group_.vars.open(ChannelVarName);
        std::vector<std::vector<Named_Variable>> dimScales =
            var.getDimensionScaleMappings({Named_Variable(ChannelVarName, ChannelVar)});
        size_t ChannelDimIndex = std::find_if(dimScales.begin(), dimScales.end(),
                                             [](const std::vector<Named_Variable> &x)
                                             { return !x.empty(); }) - dimScales.begin();
        if (ChannelDimIndex == dimScales.size())
            throw eckit::UserError("Variable " + fullName +
                                   " is not indexed by channel numbers", Here());

        Selection memSelect;
        Selection obsGroupSelect;
        createChannelSelections(var, ChannelDimIndex, channels,
                                memSelect, obsGroupSelect);
        var.write<VarType>(varValues, memSelect, obsGroupSelect);
    }
}

// -----------------------------------------------------------------------------

std::size_t ObsSpace::createChannelSelections(const Variable & variable,
                                             std::size_t ChannelDimIndex,
                                             const std::vector<int> & channels,
                                             Selection & memSelect,
                                             Selection & obsGroupSelect) const {
    // Create a vector with the channel indices corresponding to
    // the channel numbers that have been requested.
    std::vector<Dimensions_t> chanIndices;
    chanIndices.reserve(channels.size());
    for (std::size_t i = 0; i < channels.size(); ++i) {
        auto ichan = chan_num_to_index_.find(channels[i]);
        if (ichan != chan_num_to_index_.end()) {
            chanIndices.push_back(ichan->second);
        } else {
            throw eckit::BadParameter("Selected channel number " +
                std::to_string(channels[i]) + " does not exist.", Here());
        }
    }

    // Form index style selection for selecting channels
    std::vector<Dimensions_t> varDims = variable.getDimensions().dimsCur;
    std::vector<std::vector<Dimensions_t>> dimSelects(varDims.size());
    Dimensions_t numElements = 1;
    for (std::size_t i = 0; i < varDims.size(); ++i) {
        if (i == ChannelDimIndex) {
            // channels are the second dimension
            numElements *= chanIndices.size();
            dimSelects[i] = chanIndices;
        } else {
            numElements *= varDims[i];
            std::vector<Dimensions_t> allIndices(varDims[i]);
            std::iota(allIndices.begin(), allIndices.end(), 0);
            dimSelects[i] = allIndices;
        }
    }

    std::vector<Dimensions_t> memStarts(1, 0);
    std::vector<Dimensions_t> memCounts(1, numElements);
    memSelect.extent(memCounts)
             .select({SelectionOperator::SET, memStarts, memCounts});

    // If numElements is zero, can't use the dimension selection style for
    // the ObsStore backend. In this case use a hyperslab style selection with
    // zero counts along each dimension which will produce the desired effect
    // (of the selection specifying zero elements).
    if (numElements == 0) {
        // hyperslab style selection
        std::vector<Dimensions_t> obsGroupStarts(varDims.size(), 0);
        std::vector<Dimensions_t> obsGroupCounts(varDims.size(), 0);
        obsGroupSelect.extent(varDims)
                      .select({SelectionOperator::SET, obsGroupStarts, obsGroupCounts});
    } else {
        // dimension style selection
        obsGroupSelect.extent(varDims)
                      .select({SelectionOperator::SET, 0, dimSelects[0]});
        for (std::size_t i = 1; i < dimSelects.size(); ++i) {
            obsGroupSelect.select({SelectionOperator::AND, i, dimSelects[i]});
        }
    }

    return numElements;
}

void ObsSpace::fillChanNumToIndexMap() {
    // If there is a channels dimension, load up the channel number to index map
    // for channel selection feature.
    std::string ChannelVarName = this->get_dim_name(ObsDimensionId::Channel);
    if (obs_group_.vars.exists(ChannelVarName)) {
        // Get the vector of channel numbers
        Variable ChannelVar = obs_group_.vars.open(ChannelVarName);
        std::vector<int> chanNumbers;
        if (ChannelVar.isA<int>()) {
            ChannelVar.read<int>(chanNumbers);
        } else if (ChannelVar.isA<float>()) {
            std::vector<float> floatChanNumbers;
            ChannelVar.read<float>(floatChanNumbers);
            ConvertVarType<float, int>(floatChanNumbers, chanNumbers);
        }

        // Walk through the vector and place the number to index mapping into
        // the map structure.
        for (size_t i = 0; i < chanNumbers.size(); ++i) {
            chan_num_to_index_[chanNumbers[i]] = i;
        }
    }
}

// -----------------------------------------------------------------------------
void ObsSpace::splitChanSuffix(const std::string & group, const std::string & name,
                               const std::vector<int> & chanSelect, std::string & nameToUse,
                               std::vector<int> & chanSelectToUse,
                               bool skipDerived) const {
    nameToUse = name;
    chanSelectToUse = chanSelect;
    // For backward compatibility, recognize and handle appropriately variable names with
    // channel suffixes.
    if (chanSelect.empty() &&
        !obs_group_.vars.exists(fullVarName(group, name)) &&
        (skipDerived || !obs_group_.vars.exists(fullVarName("Derived" + group, name)))) {
        int channelNumber;
        if (extractChannelSuffixIfPresent(name, nameToUse, channelNumber))
            chanSelectToUse = {channelNumber};
    }
}

// -----------------------------------------------------------------------------
void ObsSpace::buildRecIdx() {
    if (this->obs_sort_var() != "") {
        // Fill the recidx_ map with indices that represent each group, while the list
        // of indices within each of the groups is sorted according to the obs space
        // configuration. This is typically used to group obs into individual
        // radiosonde soundings, and have each sounding sorted along the vertical
        // (ie, pressure or height).
        buildSortedObsGroups();
        recidx_is_sorted_ = true;
    } else {
        // Fill the recidx_ map with indices that represent each group, but are not
        // sorted. This is done so the recidx_ structure can be used to walk
        // through the individual groups. For example, this can be used to calculate
        // RMS values for each group.
        buildRecIdxUnsorted();
        recidx_is_sorted_ = false;
    }
}

// -----------------------------------------------------------------------------
void ObsSpace::buildSortedObsGroups() {
    typedef std::map<std::size_t, std::vector<std::pair<float, std::size_t>>> TmpRecIdxMap;
    typedef TmpRecIdxMap::iterator TmpRecIdxIter;

    const float missingFloat = util::missingValue<float>();
    const util::DateTime missingDateTime = util::missingValue<util::DateTime>();
    const MissingSortValueTreatment missingSortValueTreatment =
      obs_params_.top_level_.obsDataIn.value().obsGrouping.value().missingSortValueTreatment;

    // Get the sort variable from the data store, and convert to a vector of floats.
    std::size_t nlocs = this->nlocs();
    std::vector<float> SortValues(nlocs);
    std::vector<bool> sortValueMissing(nlocs, false);
    if (this->obs_sort_var() == "dateTime") {
        std::vector<util::DateTime> Dates(nlocs);
        get_db("MetaData", this->obs_sort_var(), Dates);
        for (std::size_t iloc = 0; iloc < nlocs; iloc++) {
            SortValues[iloc] = (Dates[iloc] - Dates[0]).toSeconds();
            if (Dates[iloc] == missingDateTime)
              sortValueMissing[iloc] = true;
        }
    } else {
        get_db(this->obs_sort_group(), this->obs_sort_var(), SortValues);
        for (std::size_t iloc = 0; iloc < nlocs; iloc++) {
          if (SortValues[iloc] == missingFloat)
            sortValueMissing[iloc] = true;
        }
    }

    // Construct a temporary structure to do the sorting, then transfer the results
    // to the data member recidx_.
    TmpRecIdxMap TmpRecIdx;
    // Indices of missing sort values for each record number.
    std::map<std::size_t, std::vector<std::size_t>> TmpRecIdx_missing;
    // Whether or not each location in a record has a missing sort value.
    std::map<std::size_t, std::vector<bool>> sortValueMissingInRecord;
    // Indicates whether a particular record has at least one missing sort value.
    std::map<std::size_t, bool> recordContainsAtLeastOneMissingSortValue;

    for (size_t iloc = 0; iloc < nlocs; iloc++) {
        const std::size_t recnum = recnums_[iloc];
        if (missingSortValueTreatment == MissingSortValueTreatment::SORT) {
          TmpRecIdx[recnum].push_back(std::make_pair(SortValues[iloc], iloc));
        } else if (missingSortValueTreatment == MissingSortValueTreatment::NO_SORT) {
          TmpRecIdx[recnum].push_back(std::make_pair(SortValues[iloc], iloc));
          if (sortValueMissing[iloc])
            recordContainsAtLeastOneMissingSortValue[recnum] = true;
        } else if (missingSortValueTreatment == MissingSortValueTreatment::IGNORE_MISSING) {
          if (sortValueMissing[iloc]) {
            TmpRecIdx_missing[recnum].push_back(iloc);
            sortValueMissingInRecord[recnum].push_back(true);
          } else {
            TmpRecIdx[recnum].push_back(std::make_pair(SortValues[iloc], iloc));
            sortValueMissingInRecord[recnum].push_back(false);
          }
        }
    }

    for (TmpRecIdxIter irec = TmpRecIdx.begin(); irec != TmpRecIdx.end(); ++irec) {
        // Check if any values of the sort variable in this profile are missing.
        // If so, do not proceed with the sort.
        if (missingSortValueTreatment == MissingSortValueTreatment::NO_SORT &&
            recordContainsAtLeastOneMissingSortValue[irec->first])
          continue;

        if (this->obs_sort_order() == "ascending") {
            sort(irec->second.begin(), irec->second.end());
        } else {
            // Use a lambda function to access the std::pair greater-than operator to
            // implement a descending order sort, ensuring the associated indices remain
            // in ascending order.
            sort(irec->second.begin(), irec->second.end(),
                        [](const std::pair<float, std::size_t> & p1,
                           const std::pair<float, std::size_t> & p2){
                   return (p2.first < p1.first ||
                           (!(p1.first < p2.first) && p2.second > p1.second));});
        }
    }

    // Copy indexing to the recidx_ data member.
    recidx_.clear();
    for (TmpRecIdxIter irec = TmpRecIdx.begin(); irec != TmpRecIdx.end(); ++irec) {
        const size_t recnum = irec->first;
        recidx_[recnum].resize(irec->second.size());
        if (missingSortValueTreatment == MissingSortValueTreatment::SORT ||
            missingSortValueTreatment == MissingSortValueTreatment::NO_SORT) {
          for (std::size_t iloc = 0; iloc < irec->second.size(); iloc++) {
            recidx_[recnum][iloc] = irec->second[iloc].second;
          }
        } else if (missingSortValueTreatment == MissingSortValueTreatment::IGNORE_MISSING) {
          // Locations with missing sort values in this record.
          const std::vector<std::size_t> locations_missing = TmpRecIdx_missing[recnum];
          // Locations with non-missing sort values in this record.
          const std::vector<std::pair<float, std::size_t>> locations_present = TmpRecIdx[recnum];
          // Whether or not sort values are missing at each location in this record.
          const std::vector<bool> sortValueMissingInThisRecord = sortValueMissingInRecord[recnum];
          recidx_[recnum].resize(sortValueMissingInThisRecord.size());
          // Indices of locations with a non-missing sort value.
          std::vector<std::size_t> locations_present_vector;
          for (std::size_t iloc = 0; iloc < locations_present.size(); ++iloc) {
            locations_present_vector.push_back(locations_present[iloc].second);
          }
          // Counts of missing and non-missing locations.
          std::size_t count_present = 0;
          std::size_t count_missing = 0;
          for (std::size_t iloc = 0; iloc < sortValueMissingInThisRecord.size(); ++iloc) {
            if (sortValueMissingInThisRecord[iloc]) {
              recidx_[recnum][iloc] = locations_missing[count_missing++];
            } else {
              recidx_[recnum][iloc] = locations_present_vector[count_present++];
            }
          }
        }
    }
}

// -----------------------------------------------------------------------------
void ObsSpace::buildRecIdxUnsorted() {
  recidx_.clear();
  std::size_t nlocs = this->nlocs();
  for (size_t iloc = 0; iloc < nlocs; iloc++) {
    recidx_[recnums_[iloc]].push_back(iloc);
  }
}

// -----------------------------------------------------------------------------
template <typename DataType>
void ObsSpace::extendVariable(Variable & extendVar,
                              const size_t upperBoundOnGlobalNumOriginalRecs) {
    const DataType missing = util::missingValue<DataType>();

    // Read in variable data values. At this point the values will contain
    // the extended region filled with missing values. The read call will size
    // the varVals vector accordingly.
    std::vector<DataType> varVals;
    extendVar.read<DataType>(varVals);

    for (const auto & recordindex : recidx_) {
      // Only deal with records in the original ObsSpace.
      if (recordindex.first >= upperBoundOnGlobalNumOriginalRecs) break;

      // Find the first non-missing value in the original record.
      DataType fillValue = missing;
      for (const auto & jloc : recordindex.second) {
        if (varVals[jloc] != missing) {
          fillValue = varVals[jloc];
          break;
        }
      }

      // Fill the companion record with the first non-missing value in the original record.
      // (If all values are missing, do nothing.)
      if (fillValue != missing) {
        for (const auto & jloc : recidx_[recordindex.first + upperBoundOnGlobalNumOriginalRecs]) {
          varVals[jloc] = fillValue;
        }
      }
    }

    // Write out values of the companion record.
    extendVar.write<DataType>(varVals);
}

// -----------------------------------------------------------------------------
void ObsSpace::extendObsSpace(const ObsExtendParameters & params) {
  // In this function we use the following terminology:
  // * The word 'original' refers to locations and records present in the ObsSpace before its
  //   extension.
  // * The word 'companion' refers to locations and records created when extending the ObsSpace.
  // * The word 'extended' refers to the original and companion locations and records taken
  //   together.
  // * The word 'local` refers to locations and records held on the current process.
  // * The word 'global` refers to locations and records held on any process.

  const int nlevs = params.companionRecordLength;

  const size_t numOriginalLocs = this->nlocs();
  const bool recordsExist = !this->obs_group_vars().empty();
  if (nlevs > 0 &&
      gnlocs_ > 0 &&
      recordsExist) {
    // Identify the indices of all local original records.
    const std::set<size_t> uniqueOriginalRecs(recnums_.begin(), recnums_.end());

    // Find the largest global indices of locations and records in the original ObsSpace.
    // Increment them by one to produce the initial values for the global indices of locations
    // and records in the companion ObsSpace.

    // These are *upper bounds* on the global numbers of original locations and records
    // because the sequences of global location indices and records may contain gaps.
    size_t upperBoundOnGlobalNumOriginalLocs = 0;
    size_t upperBoundOnGlobalNumOriginalRecs = 0;
    if (numOriginalLocs > 0) {
      upperBoundOnGlobalNumOriginalLocs = indx_.back() + 1;
      upperBoundOnGlobalNumOriginalRecs = *uniqueOriginalRecs.rbegin() + 1;
    }
    dist_->max(upperBoundOnGlobalNumOriginalLocs);
    dist_->max(upperBoundOnGlobalNumOriginalRecs);

    // The replica distribution will be used to place each companion record on the same process
    // as the corresponding original record.
    std::shared_ptr<Distribution> replicaDist = createReplicaDistribution(
          commMPI_, dist_, recnums_);

    // Create companion locations and records.

    // Local index of a companion location. Note that these indices, like local indices of
    // original locations, start from 0.
    size_t companionLoc = 0;
    for (size_t originalRec : uniqueOriginalRecs) {
      ASSERT(dist_->isMyRecord(originalRec));
      const size_t companionRec = originalRec;
      const size_t extendedRec = upperBoundOnGlobalNumOriginalRecs + companionRec;
      nrecs_++;
      // recidx_ stores the locations belonging to each record on the local processor.
      std::vector<size_t> &locsInRecord = recidx_[extendedRec];
      for (int ilev = 0; ilev < nlevs; ++ilev, ++companionLoc) {
        const size_t extendedLoc = numOriginalLocs + companionLoc;
        const size_t globalCompanionLoc = originalRec * nlevs + ilev;
        const size_t globalExtendedLoc = upperBoundOnGlobalNumOriginalLocs + globalCompanionLoc;
        // Geographical position shouldn't matter -- the replica distribution is expected
        // to assign records to processors solely on the basis of their indices.
        replicaDist->assignRecord(companionRec, globalCompanionLoc, eckit::geometry::Point2());
        ASSERT(replicaDist->isMyRecord(companionRec));
        recnums_.push_back(extendedRec);
        indx_.push_back(globalExtendedLoc);
        locsInRecord.push_back(extendedLoc);
      }
    }
    replicaDist->computePatchLocs();

    const size_t numCompanionLocs = companionLoc;
    const size_t numExtendedLocs = numOriginalLocs + numCompanionLocs;

    // Extend all existing vectors with missing values.
    // Only vectors with (at least) one dimension equal to nlocs are modified.
    // Second argument (bool) to resizeLocation tells function:
    //       true -> append the amount in first argument to the existing size
    //      false -> reset the existing size to the amount in the first argument
    this->resizeLocation(numExtendedLocs, false);

    // Extend all existing vectors with missing values, excepting those
    // that have been selected to be filled with non-missing values.
    // By default, some spatial and temporal coordinates are filled in this way.
    //
    // The resizeLocation() call above has extended all variables with Location as a first
    // dimension to the new Locationext size, and filled all the extended parts with
    // missing values. Go through the list of variables that are to be filled with
    // non-missing values, check if they exist and if so fill in the extended section
    // with non-missing values.
    const std::vector <std::string> &nonMissingExtendedVars = params.nonMissingExtendedVars;
    for (auto & varName : nonMissingExtendedVars) {
      // It is implied that these variables are in the MetaData group
      const std::string groupName = "MetaData";
      const std::string fullVname = fullVarName(groupName, varName);
      if (obs_group_.vars.exists(fullVname)) {
        // Note Location at this point holds the original size before extending.
        // The numOriginalLocs argument passed to extendVariable indicates where to start filling.
        Variable extendVar = obs_group_.vars.open(fullVname);
        VarUtils::forAnySupportedVariableType(
              extendVar,
              [&](auto typeDiscriminator) {
                  typedef decltype(typeDiscriminator) T;
                  extendVariable<T>(extendVar, upperBoundOnGlobalNumOriginalRecs);
              },
              VarUtils::ThrowIfVariableIsOfUnsupportedType(fullVname));
      }
    }

    // Fill extendedObsSpace with 0, which indicates the standard section of the ObsSpace,
    // and 1, which indicates the extended section.
    std::vector <int> extendedObsSpace(numExtendedLocs, 0);
    std::fill(extendedObsSpace.begin() + numOriginalLocs, extendedObsSpace.end(), 1);
    // Save extendedObsSpace for use in filters.
    put_db("MetaData", "extendedObsSpace", extendedObsSpace);

    // Calculate the number of newly created locations on all processes (counting those
    // held on multiple processes only once).
    std::unique_ptr<Accumulator<size_t>> accumulator = replicaDist->createAccumulator<size_t>();
    for (size_t companionLoc = 0; companionLoc < numCompanionLocs; ++companionLoc)
      accumulator->addTerm(companionLoc, 1);
    size_t globalNumCompanionLocs = accumulator->computeResult();

    // Replace the original distribution with a PairOfDistributions, covering
    // both the original and companion locations.
    dist_ = std::make_shared<PairOfDistributions>(commMPI_, dist_, replicaDist,
                                                  numOriginalLocs,
                                                  upperBoundOnGlobalNumOriginalRecs);

    // Increment nlocs on this processor.
    dim_info_.set_dim_size(ObsDimensionId::Location, numExtendedLocs);
    // Increment gnlocs_ and source_nlocs_.
    gnlocs_ += globalNumCompanionLocs;
    source_nlocs_ += globalNumCompanionLocs;
  }
}

// -----------------------------------------------------------------------------
void ObsSpace::createMissingObsErrors() {
  std::vector<float> obserror;  // Will be initialized only if necessary

  for (size_t i = 0; i < obsvars_.size(); ++i) {
    if (!has("ObsError", obsvars_[i])) {
      if (obserror.empty())
        obserror.assign(nlocs(), util::missingValue<float>());
      put_db("DerivedObsError", obsvars_[i], obserror);
    }
  }
}

// -----------------------------------------------------------------------------
void ObsSpace::generateLocationsToKeep(const CompareAction compareAction, const int threshold,
                                       const std::vector<int> & checkValues,
                                       std::vector<bool> & keepLocs) {
    // Form a boolean vector that shows which locations to keep from the specs for
    // the reduction (input args).
    keepLocs.resize(checkValues.size());
    if (compareAction == CompareAction::Equal) {
        for (std::size_t i = 0; i < checkValues.size(); ++i) {
            keepLocs[i] = (checkValues[i] == threshold);
        }
    } else if (compareAction == CompareAction::NotEqual) {
        for (std::size_t i = 0; i < checkValues.size(); ++i) {
            keepLocs[i] = (checkValues[i] != threshold);
        }
    } else if (compareAction == CompareAction::GreaterThan) {
        for (std::size_t i = 0; i < checkValues.size(); ++i) {
            keepLocs[i] = (checkValues[i] > threshold);
        }
    } else if (compareAction == CompareAction::LessThan) {
        for (std::size_t i = 0; i < checkValues.size(); ++i) {
            keepLocs[i] = (checkValues[i] < threshold);
        }
    } else if (compareAction == CompareAction::GreaterThanOrEqual) {
        for (std::size_t i = 0; i < checkValues.size(); ++i) {
            keepLocs[i] = (checkValues[i] >= threshold);
        }
    } else if (compareAction == CompareAction::LessThanOrEqual) {
        for (std::size_t i = 0; i < checkValues.size(); ++i) {
            keepLocs[i] = (checkValues[i] <= threshold);
        }
    }
}

// -----------------------------------------------------------------------------
std::size_t ObsSpace::reduceVarDataValues(const std::vector<bool> & keepLocs) {
    // Walk through the variables in the obs_group_ container, and if a variable is
    // dimensioned by Location then perform the reduction. This is done by:
    //   1. read the variable data into a vector
    //   2. reduce in place in this vector (don't resize since the obs_group_ resize will
    //      do that step)
    //   3. write the vector back into the variable
    //
    //   Skip over the dimension variables
    //
    std::size_t numLocs = this->nlocs();
    std::size_t reducedNlocs = 0;
    Variable locVar = obs_group_.vars.open("Location");
    for (const auto & varName : obs_group_.listObjects<ObjectType::Variable>(true)) {
        Variable var = obs_group_.vars.open(varName);

        // skip if var is a dimension variable other than Location
        if (var.isDimensionScale() && (varName != "Location")) {
            continue;
        }

        // Process the variable if it is dimensioned by Location (which is always
        // the first dimension) or the variable is Location
        if (var.isDimensionScaleAttached(0, locVar) || (varName == "Location")) {
            std::vector<Dimensions_t> varShape = var.getDimensions().dimsCur;
            VarUtils::forAnySupportedVariableType(
                var,
                [&] (auto typeDiscriminator) {
                    typedef decltype(typeDiscriminator) T;
                    std::vector<T> varValues(numLocs);
                    var.read<T>(varValues);
                    reducedNlocs = reduceVarDataInPlace<T>(keepLocs, varShape, varValues);
                    var.write<T>(varValues);
                },
                VarUtils::ThrowIfVariableIsOfUnsupportedType(varName));
        }
    }

    // The adjusted number of locations is the count of true values in the keepLocs vector
    return reducedNlocs;
}

// -----------------------------------------------------------------------------
template <typename DataType>
std::size_t ObsSpace::reduceVarDataInPlace(const std::vector<bool> & keepLocs,
                                           const std::vector<Dimensions_t> & varShape,
                                           std::vector<DataType> & varValues,
                                           const bool doResize) {
    // The idea here is to walk through the vector while checking keepLocs and at the
    // same time keeping track of the next available index for moving the value to
    // the "left" when necessary.
    //
    // We need to handle multidimensioned variables which is what the varShape argument
    // is for. Since location is the first dimension, it will be the slowest varying
    // (row-major) and each location contains a contiguous block of memory to the
    // adjacent location. varShape can be used to figure out the size of these
    // contiguous blocks which will be the product of the sizes of the second
    // through N dimensions.
    const std::size_t blockSize = std::accumulate(varShape.begin() + 1, varShape.end(), 1,
                                                  std::multiplies<Dimensions_t>());
    std::size_t nextAvailable = 0;
    std::size_t iloc = 0;
    const std::size_t nlocs = varShape[0];
    while (iloc < nlocs) {
        // If keepLocs[iloc] is false, we will throw away varValues[iloc], so have
        // nextAvailable remain where it is. This is the next slot available for
        // the next location block we keep.
        if (keepLocs[iloc]) {
            // If nextAvailable == iloc, there is no need to move, but we still want
            // to advance nextAvailable to keep track of the next available slot
            if (iloc != nextAvailable) {
                const std::size_t locStart = iloc * blockSize;
                const std::size_t moveStart = nextAvailable * blockSize;
                for (size_t jloc = 0; jloc < blockSize; ++jloc) {
                    varValues[moveStart + jloc] = varValues[locStart + jloc];
                }
            }
            ++nextAvailable;
        }
        ++iloc;
    }

    // Note after exiting the loop above nextAvailable will be equal to the number
    // of locations kept
    if (doResize) {
        varValues.resize(nextAvailable);
    }
    return nextAvailable;
}

// -----------------------------------------------------------------------------
void ObsSpace::adjustDataMembersAfterReduce(const std::vector<bool> & keepLocs) {
    // Need to adjust data members related to locations and records according
    // to the locations that have been removed.

    // The data members indx_ and recnums_ are both 1D vectors that are "dimensioned"
    // by Location, so it is convenient to use the keepLocs vector and the
    // reduceVarDataInPlace function to properly adjust their values.
    // Note 4th arguement of reduceVarDataInPlace when set to true tells that function
    // to resize the output vector.
    std::size_t reducedNlocs = reduceVarDataInPlace<std::size_t>(keepLocs,
        { static_cast<Dimensions_t>(indx_.size()) }, indx_, true);
    reducedNlocs = reduceVarDataInPlace<std::size_t>(keepLocs,
        { static_cast<Dimensions_t>(recnums_.size()) }, recnums_, true);

    // Adjust gnlocs_, this is simply the sum across mpi tasks (allReduce) of the
    // adjusted nlocs (reducedNlocs)
    this->comm().allReduce(reducedNlocs, gnlocs_, eckit::mpi::sum());

    // The adjusted nrecs_ is the number of unique values in recnums_ (which has
    // already been adjusted).
    std::set<std::size_t> uniqueRecNums;
    for (auto & recNum : recnums_) {
        uniqueRecNums.insert(recNum);
    }
    nrecs_ = uniqueRecNums.size();

    // Rebuild the patch location information
    dist_->computePatchLocs();

    // Rebuild the recidx_ data member using the newly adjusted indx_ and recnums_
    // data members.
    buildRecIdx();
}

}  // namespace ioda
