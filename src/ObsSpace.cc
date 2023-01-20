/*
 * (C) Copyright 2017-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/ObsSpace.h"

#include <algorithm>
#include <cmath>
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
#include "ioda/Engines/HH.h"
#include "ioda/Engines/ODC.h"
#include "ioda/Engines/WriteOdbFile.h"
#include "ioda/Exception.h"
#include "ioda/Io/IoPool.h"
#include "ioda/io/ObsFrameRead.h"
#include "ioda/Variables/Variable.h"

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
ObsSpace::ObsSpace(const Parameters_ & params, const eckit::mpi::Comm & comm,
                   const util::DateTime & bgn, const util::DateTime & end,
                   const eckit::mpi::Comm & timeComm)
                     : oops::ObsSpaceBase(params, comm, bgn, end),
                       winbgn_(bgn), winend_(end), commMPI_(comm),
                       gnlocs_(0), nrecs_(0), obsvars_(),
                       obs_group_(), obs_params_(params, bgn, end, comm, timeComm)
{
    // Read the obs space name
    obsname_ = obs_params_.top_level_.obsSpaceName;
    util::printRunStats("ObsSpace::ObsSpace: start " + obsname_ + ": ", true);

    // Open the source (ObsFrame) of the data for initializing the obs_group_ (ObsGroup)
    ObsFrameRead obsFrame(obs_params_);

    // Retrieve the MPI distribution object
    dist_ = obsFrame.distribution();

    createObsGroupFromObsFrame(obsFrame);
    initFromObsSource(obsFrame);

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
        oops::Variables obVars(allObsVars, channels);
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

    // After walking through all the frames, gnlocs_ and gnlocs_outside_timewindow_
    // are set representing the entire file. This is because they are calculated
    // before doing the MPI distribution.
    gnlocs_ = obsFrame.globalNumLocs();
    gnlocs_outside_timewindow_ = obsFrame.globalNumLocsOutsideTimeWindow();

    if (this->obs_sort_var() != "") {
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

    fillChanNumToIndexMap();

    if (obs_params_.top_level_.obsExtend.value() != boost::none) {
        extendObsSpace(*(obs_params_.top_level_.obsExtend.value()));
    }

    const auto & distParams = obs_params_.top_level_.distribution.value().params.value();

    createMissingObsErrors();

    oops::Log::debug() << obsname() << ": " << globalNumLocsOutsideTimeWindow()
      << " observations are outside of time window out of "
      << (globalNumLocsOutsideTimeWindow() + globalNumLocs())
      << std::endl;

    oops::Log::trace() << "ObsSpace::ObsSpace constructed name = " << obsname() << std::endl;
    util::printRunStats("ObsSpace::ObsSpace: end: ", true);
}

// -----------------------------------------------------------------------------
void ObsSpace::save() {
    if (obs_params_.top_level_.obsDataOut.value() != boost::none) {
        util::printRunStats("ObsSpace::save: start " + obsname_ + ": ", true);
        const std::string baseFiletype =
        obs_params_.top_level_.obsDataOut.value()->engine.value().engineParameters.value().type;

        if (!baseFiletype.compare("ODB")) {
          Engines::ODC::ODC_Parameters odcparams;
          // TODO(srh, djd) Following is a workaround to get access to the specific odb writer
          // parameters. The engines parameters factory returns a base class
          // (WriterParametersBase) object which does not contain the specific parameter
          // settings for the odb writer backend. The idea here is to dynamic_cast a pointer
          // to the enginesParameters object to the subclass (WriteOdbFileParameters) which
          // then gives access to the parameters defined in that subclass.
          // The long term plan is to refactoring the odb writer code to encapsulate the
          // odb specific code (such as here) in the odb writer backend object. This will
          // resolve the odb writer parameter access issue here since the odb writer backend
          // object is already given access to the WriteOdbFileParameters object.
          const Engines::WriteOdbFileParameters * odbWriterParams =
              dynamic_cast<const Engines::WriteOdbFileParameters *>(&(obs_params_.top_level_.
                  obsDataOut.value()->engine.value().engineParameters.value()));
          if (odbWriterParams == nullptr) {
              throw Exception("Did not get expected WriteOdbFileParameters object", ioda_Here());
          }
          odcparams.mappingFile = odbWriterParams->mappingFileName.value();
          odcparams.outputFile = odbWriterParams->fileName.value();
          Group writerGroup = ioda::Engines::ODC::createFile(odcparams, obs_group_);
        } else {
          // Write the output file
          std::vector<bool> patchObsVec(nlocs());
          dist_->patchObs(patchObsVec);
          IoPool obsPool(obs_params_.top_level_.ioPool,
              obs_params_.top_level_.obsDataOut.value()->engine.value().engineParameters,
              obs_params_.comm(), obs_params_.timeComm() ,
              obs_params_.windowStart(), obs_params_.windowEnd(), patchObsVec);
          obsPool.save(obs_group_);
          // Wait for all processes to finish the save call so that we know the file
          // is complete and closed.
          oops::Log::info() << obsname() << ": save database to " << obsPool << std::endl;
          this->comm().barrier();
          obsPool.finalize();
        }

        // Call the mpi barrier command here to force all processes to wait until
        // all processes have finished writing their files. This is done to prevent
        // the early processes continuing and potentially executing their obs space
        // destructor before others finish writing. This situation is known to have
        // issues with hdf file handles getting deallocated before some of the MPI
        // processes are finished with them.
        this->comm().barrier();
        util::printRunStats("ObsSpace::save: end: ", true);
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
    // For backward compatibility, recognize and handle appropriately variable names with
    // channel suffixes.
    std::string nameToUse;
    std::vector<int> chanSelectToUse;
    splitChanSuffix(group, name, { }, nameToUse, chanSelectToUse, skipDerived);
    return obs_group_.vars.exists(fullVarName(group, nameToUse)) ||
           (!skipDerived && obs_group_.vars.exists(fullVarName("Derived" + group, nameToUse)));
}

// -----------------------------------------------------------------------------
ObsDtype ObsSpace::dtype(const std::string & group, const std::string & name,
                         bool skipDerived) const {
    // For backward compatibility, recognize and handle appropriately variable names with
    // channel suffixes.
    std::string nameToUse;
    std::vector<int> chanSelectToUse;
    splitChanSuffix(group, name, { }, nameToUse, chanSelectToUse, skipDerived);

    std::string groupToUse = "Derived" + group;
    if (skipDerived || !obs_group_.vars.exists(fullVarName(groupToUse, nameToUse)))
      groupToUse = group;

    // Set the type to None if there is no type from the backend
    ObsDtype VarType = ObsDtype::None;
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
    return VarType;
}

// -----------------------------------------------------------------------------
void ObsSpace::get_db(const std::string & group, const std::string & name,
                     std::vector<int> & vdata,
                     const std::vector<int> & chanSelect, bool skipDerived) const {
    loadVar<int>(group, name, chanSelect, vdata, skipDerived);
}

void ObsSpace::get_db(const std::string & group, const std::string & name,
                     std::vector<int64_t> & vdata,
                     const std::vector<int> & chanSelect, bool skipDerived) const {
    loadVar<int64_t>(group, name, chanSelect, vdata, skipDerived);
}

void ObsSpace::get_db(const std::string & group, const std::string & name,
                     std::vector<float> & vdata,
                     const std::vector<int> & chanSelect, bool skipDerived) const {
    loadVar<float>(group, name, chanSelect, vdata, skipDerived);
}

void ObsSpace::get_db(const std::string & group, const std::string & name,
                     std::vector<double> & vdata,
                     const std::vector<int> & chanSelect, bool skipDerived) const {
    // load the float values from the database and convert to double
    std::vector<float> floatData;
    loadVar<float>(group, name, chanSelect, floatData, skipDerived);
    ConvertVarType<float, double>(floatData, vdata);
}

void ObsSpace::get_db(const std::string & group, const std::string & name,
                     std::vector<std::string> & vdata,
                     const std::vector<int> & chanSelect, bool skipDerived) const {
    loadVar<std::string>(group, name, chanSelect, vdata, skipDerived);
}

void ObsSpace::get_db(const std::string & group, const std::string & name,
                     std::vector<util::DateTime> & vdata,
                     const std::vector<int> & chanSelect, bool skipDerived) const {
    std::vector<int64_t> timeOffsets;
    loadVar<int64_t>(group, name, chanSelect, timeOffsets, skipDerived);
    Variable dtVar = obs_group_.vars.open(group + std::string("/") + name);
    util::DateTime epochDt = getEpochAsDtime(dtVar);
    vdata = convertEpochDtToDtime(epochDt, timeOffsets);
}

void ObsSpace::get_db(const std::string & group, const std::string & name,
                      std::vector<bool> & vdata,
                      const std::vector<int> & chanSelect, bool skipDerived) const {
    // Boolean variables are currently stored internally as arrays of bytes (with each byte
    // holding one element of the variable).
    // TODO(wsmigaj): Store them as arrays of bits instead, at least in the ObsStore backend,
    // to reduce memory consumption and speed up the get_db and put_db functions.
    std::vector<char> charData(vdata.size());
    loadVar<char>(group, name, chanSelect, charData, skipDerived);
    vdata.assign(charData.begin(), charData.end());
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
    openCreateEpochDtimeVar(group, name, obs_params_.top_level_.epochDateTime,
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
void ObsSpace::createObsGroupFromObsFrame(ObsFrameRead & obsFrame) {
    // Determine the maximum frame size
    Dimensions_t maxFrameSize = obs_params_.top_level_.obsDataIn.value().maxFrameSize;

    // Create the dimension specs for obs_group_
    NewDimensionScales_t newDims;
    for (auto & dimNameObject : obsFrame.backendDimVarList()) {
        std::string dimName = dimNameObject.name;
        Variable srcDimVar = dimNameObject.var;
        Dimensions_t dimSize = srcDimVar.getDimensions().dimsCur[0];
        Dimensions_t maxDimSize = dimSize;
        Dimensions_t chunkSize = dimSize;

        // If the dimension is Location, we want to avoid allocating the file's entire
        // size because we could be taking a subset of the locations (MPI distribution,
        // removal of obs outside the DA window).
        //
        // Make Location unlimited size, and start with its size limited by the
        // max_frame_size parameter.
        if (dimName == dim_info_.get_dim_name(ObsDimensionId::Location)) {
            if (dimSize > maxFrameSize) {
                dimSize = maxFrameSize;
            }
            maxDimSize = Unlimited;
            chunkSize = dimSize;
        }

        // Since different platforms equate types to different fundamental
        // C++ data types (eg, int64_t is sometimes a long and other times a long long)
        // allow for a wide range of data types for a dimension variable.
        VarUtils::forAnySupportedVariableType(
              srcDimVar,
              [&](auto typeDiscriminator) {
                  typedef decltype(typeDiscriminator) T;
                  newDims.push_back(ioda::NewDimensionScale<T>(
                      dimName, dimSize, maxDimSize, chunkSize));
              },
              VarUtils::ThrowIfVariableIsOfUnsupportedType(dimName));
    }

    // Create the backend for obs_group_
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
    obs_group_ = ObsGroup::generate(backend, newDims);

    // fill in dimension coordinate values
    for (auto & dimNameObject : obsFrame.backendDimVarList()) {
        std::string dimName = dimNameObject.name;
        Variable srcDimVar = dimNameObject.var;
        Variable destDimVar = obs_group_.vars.open(dimName);

        // Set up the dimension selection objects. The prior loop declared the
        // sizes of all the dimensions in the frame so use that as a guide, and
        // transfer the first frame's worth of coordinate values accordingly.
        std::vector<Dimensions_t> srcDimShape = srcDimVar.getDimensions().dimsCur;
        std::vector<Dimensions_t> destDimShape = destDimVar.getDimensions().dimsCur;

        std::vector<Dimensions_t> counts = destDimShape;
        std::vector<Dimensions_t> starts(counts.size(), 0);

        Selection srcSelect;
        srcSelect.extent(srcDimShape).select({SelectionOperator::SET, starts, counts });
        Selection memSelect;
        memSelect.extent(destDimShape).select({SelectionOperator::SET, starts, counts });
        Selection destSelect;
        destSelect.extent(destDimShape).select({SelectionOperator::SET, starts, counts });

        // Since different platforms equate types to different fundamental
        // C++ data types (eg, int64_t is sometimes a long and other times a long long)
        // allow for a wide range of data types for a dimension variable.
        VarUtils::forAnySupportedVariableType(
              srcDimVar,
              [&](auto typeDiscriminator) {
                  typedef decltype(typeDiscriminator) T;
                  std::vector<T> dimCoords;
                  srcDimVar.read<T>(dimCoords, memSelect, srcSelect);
                  destDimVar.write<T>(dimCoords, memSelect, destSelect);
              },
              VarUtils::ThrowIfVariableIsOfUnsupportedType(dimName));
    }
}

// -----------------------------------------------------------------------------
template<typename VarType>
bool ObsSpace::readObsSource(ObsFrameRead & obsFrame,
                            const std::string & varName, std::vector<VarType> & varValues) {
    Variable sourceVar = obsFrame.getObsGroup().vars.open(varName);

    // Read the variable
    bool gotVarData = obsFrame.readFrameVar(varName, varValues);

    // Replace source fill values with corresponding missing marks
    if ((gotVarData) && (sourceVar.hasFillValue())) {
        VarType sourceFillValue;
        detail::FillValueData_t sourceFvData = sourceVar.getFillValue();
        sourceFillValue = detail::getFillValue<VarType>(sourceFvData);
        VarType varFillValue = this->getFillValue<VarType>();
        for (std::size_t i = 0; i < varValues.size(); ++i) {
            if ((varValues[i] == sourceFillValue) || std::isinf(varValues[i])
                                                  || std::isnan(varValues[i])) {
                varValues[i] = varFillValue;
            }
        }
    }
    return gotVarData;
}

template<>
bool ObsSpace::readObsSource(ObsFrameRead & obsFrame,
                            const std::string & varName, std::vector<std::string> & varValues) {
    Variable sourceVar = obsFrame.getObsGroup().vars.open(varName);

    // Read the variable
    bool gotVarData = obsFrame.readFrameVar(varName, varValues);

    // Replace source fill values with corresponding missing marks
    if ((gotVarData) && (sourceVar.hasFillValue())) {
        std::string sourceFillValue;
        detail::FillValueData_t sourceFvData = sourceVar.getFillValue();
        sourceFillValue = detail::getFillValue<std::string>(sourceFvData);
        std::string varFillValue = this->getFillValue<std::string>();
        for (std::size_t i = 0; i < varValues.size(); ++i) {
            if (varValues[i] == sourceFillValue) {
                varValues[i] = varFillValue;
            }
        }
    }
    return gotVarData;
}

// -----------------------------------------------------------------------------
void ObsSpace::initFromObsSource(ObsFrameRead & obsFrame) {
    // Walk through the frames and copy the data to the obs_group_ storage
    int iframe = 1;
    // Create the frame and the variables in obs_group_ based on those in the obs source.
    // Need to call frameInit before createVariables to get the frame established.
    // Make these calls outside the for loop so that the obs_group_ container will
    // get its variables created in the case that nlocs == 0.
    obsFrame.frameInit(obs_group_.atts);
    dims_attached_to_vars_ = obsFrame.varDimMap();
    createVariables(obsFrame.getObsGroup().vars, obs_group_.vars, dims_attached_to_vars_);
    for ( ; obsFrame.frameAvailable(); obsFrame.frameNext()) {
        Dimensions_t frameStart = obsFrame.frameStart();

        // Resize the Location dimesion according to the adjusted frame size produced
        // genFrameIndexRecNums. The second argument is to tell resizeLocation whether
        // to append or reset to the size given by the first arguemnt.
        resizeLocation(obsFrame.adjLocationFrameCount(), (iframe > 1));

        // Clear out the selection caches
        known_fe_selections_.clear();
        known_be_selections_.clear();

        // If the ioda input file only contained the string datetime representation
        // (variable MetaData/datetime), it has been converted to the epoch representation
        // (variable MetaData/dateTime) so the string datetime variable can be omitted
        // from the ObsSpace container. Same for the offset datetime representation
        // (variable MetaData/time)
        for (auto & varNameObject : obsFrame.varList()) {
            std::string varName = varNameObject.name;
            if ((varName == "MetaData/datetime") || (varName == "MetaData/time")) {
              continue;
            }
            Variable var = varNameObject.var;
            Dimensions_t beFrameStart;
            if (obsFrame.isVarDimByLocation(varName)) {
                beFrameStart = obsFrame.adjLocationFrameStart();
            } else {
                beFrameStart = frameStart;
            }
            Dimensions_t frameCount = obsFrame.frameCount(varName);

            // Transfer the variable to the in-memory storage
            VarUtils::forAnySupportedVariableType(
                  var,
                  [&](auto typeDiscriminator) {
                      typedef decltype(typeDiscriminator) T;
                      std::vector<T> varValues;
                      if (readObsSource<T>(obsFrame, varName, varValues)) {
                          storeVar<T>(varName, varValues, beFrameStart, frameCount);
                      }
                  },
                  VarUtils::ThrowIfVariableIsOfUnsupportedType(varName));
        }
        iframe++;
    }

    // Record locations and channels dimension sizes
    // The HDF library has an issue when a dimension marked UNLIMITED is queried for its
    // size a zero is returned instead of the proper current size. As a workaround for this
    // ask the frame how many locations it kept instead of asking the Location dimension for
    // its size.
    std::string LocationName = dim_info_.get_dim_name(ObsDimensionId::Location);
    std::size_t nLocs = obsFrame.frameNumLocs();
    dim_info_.set_dim_size(ObsDimensionId::Location, nLocs);

    std::string ChannelName = dim_info_.get_dim_name(ObsDimensionId::Channel);
    if (obs_group_.vars.exists(ChannelName)) {
        std::size_t nChans = obs_group_.vars.open(ChannelName).getDimensions().dimsCur[0];
        dim_info_.set_dim_size(ObsDimensionId::Channel, nChans);
    }

    // Record "record" information
    nrecs_ = obsFrame.frameNumRecs();
    indx_ = obsFrame.index();
    recnums_ = obsFrame.recnums();
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
            if (var.isDimensionScaleAttached(1, ChannelVar) && (chanSelectToUse.size() > 0)) {
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

// -----------------------------------------------------------------------------
// This function is for transferring data from a memory buffer into the ObsSpace
// container. At this point, the time window filtering, obs grouping and MPI
// distribution has been applied to the input memory buffer (varValues). Also,
// the variable has been resized according to appending a new frame's worth of
// data to the existing variable in the ObsSpace container.
//
// What this means is that you can always transfer the data as a single contiguous
// block which can be accomplished with a single hyperslab selection. There should
// be no need to cache these selections because of this.
template<typename VarType>
void ObsSpace::storeVar(const std::string & varName, std::vector<VarType> & varValues,
                       const Dimensions_t frameStart, const Dimensions_t frameCount) {
    // get the dimensions of the variable
    Variable var = obs_group_.vars.open(varName);
    std::vector<Dimensions_t> varDims = var.getDimensions().dimsCur;

    // check the caches for the selectors
    VarUtils::Vec_Named_Variable dims;
    for (auto & ivar : dims_attached_to_vars_) {
        if (ivar.first.name == varName) {
            dims = ivar.second;
            break;
        }
    }
    if (!known_fe_selections_.count(dims)) {
        // backend starts at frameStart, and the count for the first dimension
        // is the frame count
        std::vector<Dimensions_t> beCounts = varDims;
        beCounts[0] = frameCount;
        std::vector<Dimensions_t> beStarts(beCounts.size(), 0);
        beStarts[0] = frameStart;

        // front end always starts at zero, and the number of elements is equal to the
        // product of the var dimensions (with the first dimension adjusted by
        // the frame count)
        std::vector<Dimensions_t> feCounts(1, std::accumulate(
            beCounts.begin(), beCounts.end(), static_cast<Dimensions_t>(1),
            std::multiplies<Dimensions_t>()));
        std::vector<Dimensions_t> feStarts(1, 0);

        known_fe_selections_[dims] = Selection()
            .extent(feCounts).select({ SelectionOperator::SET, feStarts, feCounts });
        known_be_selections_[dims] = Selection()
            .extent(varDims).select({ SelectionOperator::SET, beStarts, beCounts });
    }
    Selection & feSelect = known_fe_selections_[dims];
    Selection & beSelect = known_be_selections_[dims];

    var.write<VarType>(varValues, feSelect, beSelect);
}

// -----------------------------------------------------------------------------
// NOTE(RH): Variable creation params rewritten so they are constructed once for each type.
//           There is no need to keep re-creating the same objects over and over again.
// TODO(?): Rewrite slightly so that the scales are opened all at once and are kept
//          open until the end of the function.
// TODO(?): Batch variable creation so that the collective function is used.
void ObsSpace::createVariables(const Has_Variables & srcVarContainer,
                              Has_Variables & destVarContainer,
                              const VarUtils::VarDimMap & dimsAttachedToVars) {
    // Set up reusable creation parameters for the loop below. Use the JEDI missing
    // values for the fill values.
    std::map<std::type_index, VariableCreationParameters> paramsByType;
    VarUtils::forEachSupportedVariableType(
          [&](auto typeDiscriminator) {
              typedef decltype(typeDiscriminator) T;
              paramsByType[typeid(T)] = VariableCreationParameters::defaults<T>();
              paramsByType.at(typeid(T)).setFillValue<T>(this->getFillValue<T>());
          });

    // Walk through map to get list of variables to create along with
    // their dimensions. Use the srcVarContainer to get the var data type.
    //
    // If the ioda input file only contained the string datetime representation
    // (variable MetaData/datetime), it has been converted to the epoch representation
    // (variable MetaData/dateTime) so the string datetime variable can be omitted
    // from the ObsSpace container. Same for the offset datetime representation
    // (variable MetaData/time). Note this function is only called by the ioda reader.
    for (auto & ivar : dimsAttachedToVars) {
        std::string varName = ivar.first.name;
        if ((varName == "MetaData/datetime") || (varName == "MetaData/time")) {
          continue;
        }
        VarUtils::Vec_Named_Variable srcVarDimNames = ivar.second;

        // Create a vector with dimension scale vector from destination container
        std::vector<Variable> varDims;
        for (auto & srcDimVar : srcVarDimNames) {
            varDims.push_back(destVarContainer.open(srcDimVar.name));
        }

        Variable srcVar = srcVarContainer.open(varName);
        VarUtils::forAnySupportedVariableType(
              srcVar,
              [&](auto typeDiscriminator) {
                  typedef decltype(typeDiscriminator) T;
                  Variable destVar = destVarContainer.createWithScales<T>(varName, varDims,
                                                       paramsByType.at(typeid(T)));
                  copyAttributes(srcVar.atts, destVar.atts);
              },
              [&] (const ioda::source_location &) {
                  if (this->comm().rank() == 0)
                      oops::Log::warning() << "WARNING: ObsSpace::createVariables: "
                          << "Skipping variable due to an unexpected data type for variable: "
                          << varName << std::endl;
            });
    }
}

// -----------------------------------------------------------------------------
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
        for (int i = 0; i < chanNumbers.size(); ++i) {
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
void ObsSpace::buildSortedObsGroups() {
    typedef std::map<std::size_t, std::vector<std::pair<float, std::size_t>>> TmpRecIdxMap;
    typedef TmpRecIdxMap::iterator TmpRecIdxIter;

    const float missingFloat = util::missingValue(missingFloat);
    const util::DateTime missingDateTime = util::missingValue(missingDateTime);
    const MissingSortValueTreatment missingSortValueTreatment =
      obs_params_.top_level_.obsDataIn.value().obsGrouping.value().missingSortValueTreatment;

    // Get the sort variable from the data store, and convert to a vector of floats.
    std::size_t nLocs = this->nlocs();
    std::vector<float> SortValues(nLocs);
    std::vector<bool> sortValueMissing(nLocs, false);
    if (this->obs_sort_var() == "dateTime") {
        std::vector<util::DateTime> Dates(nLocs);
        get_db("MetaData", this->obs_sort_var(), Dates);
        for (std::size_t iloc = 0; iloc < nLocs; iloc++) {
            SortValues[iloc] = (Dates[iloc] - Dates[0]).toSeconds();
            if (Dates[iloc] == missingDateTime)
              sortValueMissing[iloc] = true;
        }
    } else {
        get_db(this->obs_sort_group(), this->obs_sort_var(), SortValues);
        for (std::size_t iloc = 0; iloc < nLocs; iloc++) {
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

    for (size_t iloc = 0; iloc < nLocs; iloc++) {
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
  std::size_t nLocs = this->nlocs();
  for (size_t iloc = 0; iloc < nLocs; iloc++) {
    recidx_[recnums_[iloc]].push_back(iloc);
  }
}

// -----------------------------------------------------------------------------
template <typename DataType>
void ObsSpace::extendVariable(Variable & extendVar,
                              const size_t upperBoundOnGlobalNumOriginalRecs) {
    const DataType missing = util::missingValue(missing);

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
    // Increment gnlocs_.
    gnlocs_ += globalNumCompanionLocs;
  }
}

// -----------------------------------------------------------------------------
void ObsSpace::createMissingObsErrors() {
  std::vector<float> obserror;  // Will be initialized only if necessary

  for (size_t i = 0; i < obsvars_.size(); ++i) {
    if (!has("ObsError", obsvars_[i])) {
      if (obserror.empty())
        obserror.assign(nlocs(), util::missingValue(float()));
      put_db("DerivedObsError", obsvars_[i], obserror);
    }
  }
}
// -----------------------------------------------------------------------------

}  // namespace ioda
