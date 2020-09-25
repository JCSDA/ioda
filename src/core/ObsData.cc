/*
 * (C) Copyright 2017-2019 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/core/ObsData.h"

#include <algorithm>
#include <fstream>
#include <functional>
#include <iomanip>
#include <map>
#include <memory>
#include <numeric>
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
#include "oops/util/Random.h"
#include "oops/util/stringFunctions.h"

#include "ioda/distribution/DistributionFactory.h"
#include "ioda/io/ObsFrameFactory.h"
#include "ioda/Variables/Variable.h"

#include "atlas/util/Earth.h"

namespace ioda {

// ----------------------------- public functions ------------------------------
// -----------------------------------------------------------------------------
ObsData::ObsData(const eckit::Configuration & config, const eckit::mpi::Comm & comm,
                 const util::DateTime & bgn, const util::DateTime & end)
                     : config_(config), winbgn_(bgn), winend_(end), commMPI_(comm),
                       gnlocs_(0), nlocs_(0), nvars_(0), nrecs_(0), obsvars_(),
                       obs_group_(), obs_params_(bgn, end, comm)
{
    oops::Log::trace() << "ObsData::ObsData config  = " << config << std::endl;
    obs_params_.deserialize(config);

    obsname_ = obs_params_.top_level_.obsSpaceName;
    obsvars_ = oops::Variables(config, "simulated variables");
    oops::Log::info() << this->obsname() << " vars: " << obsvars_ << std::endl;

    // Create the MPI distribution object
    std::unique_ptr<DistributionFactory> distFactory;
    dist_.reset(distFactory->createDistribution(this->comm(), this->distname()));

    // Open the source (ObsFrame) of the data for initializing the obs_group_ (ObsGroup)
    std::shared_ptr<ObsFrame> obsFrame;
    if (obs_params_.in_type() == ObsIoTypes::OBS_FILE) {
        obsFrame = ObsFrameFactory::create(ObsIoActions::OPEN_FILE,
                                           ObsIoModes::READ_ONLY, obs_params_);
    } else if ((obs_params_.in_type() == ObsIoTypes::GENERATOR_RANDOM) ||
               (obs_params_.in_type() == ObsIoTypes::GENERATOR_LIST)) {
        obsFrame = ObsFrameFactory::create(ObsIoActions::CREATE_GENERATOR,
                                           ObsIoModes::READ_ONLY, obs_params_);
    } else {
      // Error - must have one of obsdatain or generate
      std::string ErrorMsg =
          std::string("ObsData::ObsData: Must use one of 'obsdatain' or 'generate' ");
          std::string(" in the YAML configuration.");
      ABORT(ErrorMsg);
    }

    createObsGroupFromObsFrame(obsFrame);
    initFromObsSource(obsFrame);

    gnlocs_ = obsFrame->ioNumLocs();

    if (this->obs_sort_var() != "") {
      BuildSortedObsGroups();
    }

    oops::Log::trace() << "ObsData::ObsData contructed name = " << obsname() << std::endl;
}

// -----------------------------------------------------------------------------
/// \details Destructor for an ObsData object. This destructor will clean up the ObsData
///          object and optionally write out the contents of the obs container into
///          the output file. The save-to-file operation is invoked when an output obs
///          file is specified in the ECKIT configuration segment associated with the
///          ObsData object.
ObsData::~ObsData() {
///     if (obs_params_.out_type() == ObsIoTypes::OBS_FILE) {
///         std::string fileName = obs_params_.top_level_.obsOutFile.value()->fileName;
///         oops::Log::info() << obsname() << ": save database to " << fileName << std::endl;
///         // SaveToFile(fileName, obs_params_.out_file.maxFrameSize);
///     } else {
///         oops::Log::info() << obsname() << " :  no output" << std::endl;
///     }
    oops::Log::trace() << "ObsData::ObsData destructor" << std::endl;
}

// -----------------------------------------------------------------------------
std::size_t ObsData::nlocs() const {
    return obs_group_.vars.open("nlocs").getDimensions().dimsCur[0];
}

// -----------------------------------------------------------------------------
std::size_t ObsData::nrecs() const {
    return nrecs_;
}

// -----------------------------------------------------------------------------
std::size_t ObsData::nvars() const {
    Group g = obs_group_.open("ObsValue");
    return g.vars.list().size();
}

// -----------------------------------------------------------------------------
std::string ObsData::obs_group_var() const {
    std::string obsGroupVar = "";
    if (obs_params_.top_level_.obsInFile.value() != boost::none) {
        obsGroupVar =
            obs_params_.top_level_.obsInFile.value()->obsGrouping.value().obsGroupVar;
    }
    return obsGroupVar;
}

// -----------------------------------------------------------------------------
std::string ObsData::obs_sort_var() const {
    std::string obsSortVar = "";
    if (obs_params_.top_level_.obsInFile.value() != boost::none) {
        obsSortVar =
            obs_params_.top_level_.obsInFile.value()->obsGrouping.value().obsSortVar;
    }
    return obsSortVar;
}

// -----------------------------------------------------------------------------
std::string ObsData::obs_sort_order() const {
    std::string obsSortOrder = "";
    if (obs_params_.top_level_.obsInFile.value() != boost::none) {
        obsSortOrder =
            obs_params_.top_level_.obsInFile.value()->obsGrouping.value().obsSortOrder;
    }
    return obsSortOrder;
}

// -----------------------------------------------------------------------------
bool ObsData::has(const std::string & group, const std::string & name) const {
    return obs_group_.vars.exists(fullVarName(group, name));
}

// -----------------------------------------------------------------------------
ObsDtype ObsData::dtype(const std::string & group, const std::string & name) const {
    // Set the type to None if there is no type from the backend
    ObsDtype VarType = ObsDtype::None;
    if (has(group, name)) {
        Variable var = obs_group_.vars.open(fullVarName(group, name));
        if (var.isA<int>()) {
            VarType = ObsDtype::Integer;
        } else if (var.isA<float>()) {
            VarType = ObsDtype::Float;
        } else if (var.isA<std::string>()) {
            VarType = ObsDtype::String;
        }
    }
    return VarType;
}

// -----------------------------------------------------------------------------
void ObsData::get_db(const std::string & group, const std::string & name,
                     std::vector<int> & vdata) const {
    Variable var = obs_group_.vars.open(fullVarName(group, name));
    var.read<int>(vdata);
}

void ObsData::get_db(const std::string & group, const std::string & name,
                     std::vector<float> & vdata) const {
    Variable var = obs_group_.vars.open(fullVarName(group, name));
    var.read<float>(vdata);
}

void ObsData::get_db(const std::string & group, const std::string & name,
                     std::vector<double> & vdata) const {
    // load the float values from the database and convert to double
    std::vector<float> floatData(vdata.size(), 0.0);
    Variable var = obs_group_.vars.open(fullVarName(group, name));
    var.read<float>(floatData);
    ConvertVarType<float, double>(floatData, vdata);
}

void ObsData::get_db(const std::string & group, const std::string & name,
                     std::vector<std::string> & vdata) const {
    Variable var = obs_group_.vars.open(fullVarName(group, name));
    var.read<std::string>(vdata);
}

void ObsData::get_db(const std::string & group, const std::string & name,
                      std::vector<util::DateTime> & vdata) const {
    // TODO(srh) fill in read strings, convert to DateTime objects
}

// -----------------------------------------------------------------------------
void ObsData::put_db(const std::string & group, const std::string & name,
                      const std::vector<int> & vdata) {
  Variable var = openCreateVar<int>(fullVarName(group, name));
  var.write<int>(vdata);
}

void ObsData::put_db(const std::string & group, const std::string & name,
                      const std::vector<float> & vdata) {
  Variable var = openCreateVar<float>(fullVarName(group, name));
  var.write<float>(vdata);
}

void ObsData::put_db(const std::string & group, const std::string & name,
                      const std::vector<double> & vdata) {
  // convert to float, then load into the database
  std::vector<float> floatData(vdata.size());
  ConvertVarType<double, float>(vdata, floatData);
  Variable var = openCreateVar<float>(fullVarName(group, name));
  var.write<float>(floatData);
}

void ObsData::put_db(const std::string & group, const std::string & name,
                      const std::vector<std::string> & vdata) {
  Variable var = openCreateVar<std::string>(fullVarName(group, name));
  var.write<std::string>(vdata);
}

void ObsData::put_db(const std::string & group, const std::string & name,
                      const std::vector<util::DateTime> & vdata) {
  // TODO(srh) convert DateTime objects to strings and write
}







// -----------------------------------------------------------------------------
/*!
 * \details This method returns the begin iterator associated with the
 *          recidx_ data member.
 */
const ObsData::RecIdxIter ObsData::recidx_begin() const {
  return recidx_.begin();
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the end iterator associated with the
 *          recidx_ data member.
 */
const ObsData::RecIdxIter ObsData::recidx_end() const {
  return recidx_.end();
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns a boolean value indicating whether the
 *          given record number exists in the recidx_ data member.
 */
bool ObsData::recidx_has(const std::size_t RecNum) const {
  RecIdxIter irec = recidx_.find(RecNum);
  return (irec != recidx_.end());
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the current record number, pointed to by the
 *          given iterator, from the recidx_ data member.
 */
std::size_t ObsData::recidx_recnum(const RecIdxIter & Irec) const {
  return Irec->first;
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the current vector, pointed to by the
 *          given iterator, from the recidx_ data member.
 */
const std::vector<std::size_t> & ObsData::recidx_vector(const RecIdxIter & Irec) const {
  return Irec->second;
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the current vector, pointed to by the
 *          given iterator, from the recidx_ data member.
 */
const std::vector<std::size_t> & ObsData::recidx_vector(const std::size_t RecNum) const {
  RecIdxIter Irec = recidx_.find(RecNum);
  if (Irec == recidx_.end()) {
    std::string ErrMsg =
      "ObsData::recidx_vector: Record number, " + std::to_string(RecNum) +
      ", does not exist in record index map.";
    ABORT(ErrMsg);
  }
  return Irec->second;
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the all of the record numbers from the
 *          recidx_ data member (ie, all the key values) in a vector.
 */
std::vector<std::size_t> ObsData::recidx_all_recnums() const {
  std::vector<std::size_t> RecNums(nrecs_);
  std::size_t recnum = 0;
  for (RecIdxIter Irec = recidx_.begin(); Irec != recidx_.end(); ++Irec) {
    RecNums[recnum] = Irec->first;
    recnum++;
  }
  return RecNums;
}

// ----------------------------- private functions -----------------------------
// -----------------------------------------------------------------------------
void ObsData::print(std::ostream & os) const {
    os << "ObsData::print not implemented";
}

// -----------------------------------------------------------------------------
void ObsData::createObsGroupFromObsFrame(const std::shared_ptr<ObsFrame> & obsFrame) {
    // Create the dimension specs for obs_group_
    NewDimensionScales_t newDims;
    for (auto & dimName : obsFrame->ioDimVarList()) {
        Variable var = obsFrame->vars().open(dimName);
        Dimensions_t dimSize = var.getDimensions().dimsCur[0];
        Dimensions_t maxDimSize = dimSize;
        Dimensions_t chunkSize = dimSize;

        // If the dimension is nlocs, we want to avoid allocating the file's entire
        // size because we could be taking a subset of the locations (MPI distribution,
        // removal of obs outside the DA window).
        //
        // Make nlocs unlimited size, and start with a small size. Then when writing into
        // the ObsGroup backend, resize along nlocs and put in the new data.
        if (dimName == "nlocs") {
            dimSize = 10;
            maxDimSize = Unlimited;
            chunkSize = 10;
        }
        newDims.push_back(std::make_shared<ioda::NewDimensionScale<int>>(
            dimName, dimSize, maxDimSize, chunkSize));
    }

    // Create the backend for obs_group_
    Engines::BackendNames backendName = Engines::BackendNames::ObsStore;
    Engines::BackendCreationParameters backendParams;
    Group backend = constructBackend(backendName, backendParams);

    // Create the ObsGroup and attach the backend.
    obs_group_ = ObsGroup::generate(backend, newDims);
}

// -----------------------------------------------------------------------------
void ObsData::initFromObsSource(const std::shared_ptr<ObsFrame> & obsFrame) {
    // Walk through the frames and copy the data to the obs_group_ storage
    std::vector<std::string> varList = obsFrame->ioVarList();

    // Create variables in obs_group_ based on those in the obs source
    createVariablesFromObsSource(obsFrame, varList);

    Variable nlocsVar = obsFrame->vars().open("nlocs");
    int iframe = 1;
    for (obsFrame->frameInit(); obsFrame->frameAvailable(); obsFrame->frameNext()) {
        Dimensions_t frameStart = obsFrame->frameStart();

        // Generate the indices (for selection) for variables dimensioned by nlocs
        obsFrame->genFrameIndexRecNums(dist_);

        // Resize the nlocs dimesion according to the adjusted frame size produced
        // genFrameIndexRecNums. The second argument is to tell resizeNlocs whether
        // to append or reset to the size given by the first arguemnt.
        resizeNlocs(obsFrame->adjNlocsFrameCount(), (iframe > 1));

        for (auto & varName : varList) {
            Variable var = obsFrame->vars().open(varName);
            Dimensions_t beFrameStart;
            if (var.isDimensionScaleAttached(0, nlocsVar)) {
                beFrameStart = obsFrame->adjNlocsFrameStart();
            } else {
                beFrameStart = frameStart;
            }
            Dimensions_t frameCount = obsFrame->frameCount(var);
            if (frameCount > 0) {
                // Transfer the variable to the in-memory storage
                if (var.isA<int>()) {
                    std::vector<int> varValues;
                    readObsSource<int>(obsFrame, varName, varValues);
                    storeVar<int>(varName, varValues, beFrameStart, frameCount);
                } else if (var.isA<float>()) {
                    std::vector<float> varValues;
                    readObsSource<float>(obsFrame, varName, varValues);
                    storeVar<float>(varName, varValues, beFrameStart, frameCount);
                } else if (var.isA<std::string>()) {
                    std::vector<std::string> varValues;
                    readObsSource<std::string>(obsFrame, varName, varValues);
                    storeVar<std::string>(varName, varValues, beFrameStart, frameCount);
                }
            }
        }

        iframe++;
    }

    nrecs_ = obsFrame->frameNumLocs();
    indx_ = obsFrame->index();
    recnums_ = obsFrame->recnums();
}

// -----------------------------------------------------------------------------
void ObsData::resizeNlocs(const Dimensions_t nlocsSize, const bool append) {
    Variable nlocsVar = obs_group_.vars.open("nlocs");
    Dimensions_t nlocsResize;
    if (append) {
        nlocsResize = nlocsVar.getDimensions().dimsCur[0] + nlocsSize;
    } else {
        nlocsResize = nlocsSize;
    }
    obs_group_.resize(
        { std::pair<Variable, Dimensions_t>(nlocsVar, nlocsResize) });
}

// -----------------------------------------------------------------------------
std::vector<Variable> ObsData::setVarDimsFromObsSource(
    const std::shared_ptr<ObsFrame> & obsFrame, const std::string & varName) {
    // Get a list of dimension variables from the source (obsIo)
    std::map<std::string, Variable> srcDimVars;
    for (auto & dimVarName : obsFrame->ioDimVarList()) {
        Variable dimVar = obsFrame->vars().open(dimVarName);
        srcDimVars.insert(std::pair<std::string, Variable>(dimVarName, dimVar));
    }

    // find which dimensions are attached to the source variable
    Variable sourceVar = obsFrame->vars().open(varName);
    Dimensions sourceDims = sourceVar.getDimensions();
    std::vector<std::string> dimNames;
    for (std::size_t i = 0; i < sourceDims.dimensionality; ++i) {
        for (auto & dimVar : srcDimVars) {
            if (sourceVar.isDimensionScaleAttached(i, dimVar.second)) {
                dimNames.push_back(dimVar.first);
            }
        }
    }

    // convert the list of names to a list of variables
    std::vector<Variable> destDimVars;
    for (auto & dimName : dimNames) {
        destDimVars.push_back(obs_group_.vars.open(dimName));
    }

    return destDimVars;
}

// -----------------------------------------------------------------------------
void ObsData::createVariablesFromObsSource(const std::shared_ptr<ObsFrame> & obsFrame,
                                           const std::vector<std::string> & varList) {
    for (std::size_t i = 0; i < varList.size(); ++i) {
        std::string varName = varList[i];
        Variable var = obsFrame->vars().open(varName);
        if (var.isA<int>()) {
            createVarFromObsSource<int>(obsFrame, varName);
        } else if (var.isA<float>()) {
            createVarFromObsSource<float>(obsFrame, varName);
        } else if (var.isA<std::string>()) {
            createVarFromObsSource<std::string>(obsFrame, varName);
        } else {
            if (this->comm().rank() == 0) {
                oops::Log::warning() << "WARNING: ObsData::createVariablesFromObsSpace: "
                    << "Skipping variable due to an unexpected data type for variable: "
                    << varName << std::endl;
            }
        }
    }
}







// -----------------------------------------------------------------------------
/*!
 * \details This method will construct a data structure that holds the
 *          location order within each group sorted by the values of
 *          the specified sort variable.
 */
void ObsData::BuildSortedObsGroups() {
  typedef std::map<std::size_t, std::vector<std::pair<float, std::size_t>>> TmpRecIdxMap;
  typedef TmpRecIdxMap::iterator TmpRecIdxIter;

  // Get the sort variable from the data store, and convert to a vector of floats.
  std::vector<float> SortValues(nlocs_);
  if (this->obs_sort_var() == "datetime") {
    std::vector<util::DateTime> Dates(nlocs_);
    get_db("MetaData", this->obs_sort_var(), Dates);
    for (std::size_t iloc = 0; iloc < nlocs_; iloc++) {
      SortValues[iloc] = (Dates[iloc] - Dates[0]).toSeconds();
    }
  } else {
    get_db("MetaData", this->obs_sort_var(), SortValues);
  }

  // Construct a temporary structure to do the sorting, then transfer the results
  // to the data member recidx_.
  TmpRecIdxMap TmpRecIdx;
  for (size_t iloc = 0; iloc < nlocs_; iloc++) {
    TmpRecIdx[recnums_[iloc]].push_back(std::make_pair(SortValues[iloc], iloc));
  }

  for (TmpRecIdxIter irec = TmpRecIdx.begin(); irec != TmpRecIdx.end(); ++irec) {
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
    recidx_[irec->first].resize(irec->second.size());
    for (std::size_t iloc = 0; iloc < irec->second.size(); iloc++) {
      recidx_[irec->first][iloc] = irec->second[iloc].second;
    }
  }
}

// -----------------------------------------------------------------------------
/*!
 * \details This method will save the contents of the obs container into the
 *          given file. Currently, all variables in the obs container are written
 *          into the file. This may change in the future where we can select which
 *          variables we want saved.
 *
 * \param[in] file_name Path to output obs file.
 */
void ObsData::SaveToFile(const std::string & file_name, const std::size_t MaxFrameSize) {
///   // Open the file for output
///   std::unique_ptr<IodaIO> fileio
///     {ioda::IodaIOfactory::Create(file_name, "W", MaxFrameSize)};

///   // Add dimensions for nlocs and nvars
///   fileio->dim_insert("nlocs", nlocs_);
///   fileio->dim_insert("nvars", nvars_);

///   // Build the group, variable info container. This defines the variables
///   // that will be written into the output file.
///   std::size_t MaxVarSize = 0;
///   for (ObsSpaceContainer<int>::VarIter ivar = int_database_.var_iter_begin();
///                                        ivar != int_database_.var_iter_end(); ++ivar) {
///     std::string GroupName = int_database_.var_iter_gname(ivar);
///     std::string VarName = int_database_.var_iter_vname(ivar);
///     std::string GrpVarName = VarName + "@" + GroupName;
///     std::vector<std::size_t> VarShape = int_database_.var_iter_shape(ivar);
///     if (VarShape[0] > MaxVarSize) { MaxVarSize = VarShape[0]; }
///     fileio->grp_var_insert(GroupName, VarName, "int", VarShape, GrpVarName, "int");
///   }
///   for (ObsSpaceContainer<float>::VarIter ivar = float_database_.var_iter_begin();
///                                        ivar != float_database_.var_iter_end(); ++ivar) {
///     std::string GroupName = float_database_.var_iter_gname(ivar);
///     std::string VarName = float_database_.var_iter_vname(ivar);
///     std::string GrpVarName = VarName + "@" + GroupName;
///     std::vector<std::size_t> VarShape = float_database_.var_iter_shape(ivar);
///     if (VarShape[0] > MaxVarSize) { MaxVarSize = VarShape[0]; }
///     fileio->grp_var_insert(GroupName, VarName, "float", VarShape, GrpVarName, "float");
///   }
///   for (ObsSpaceContainer<std::string>::VarIter ivar = string_database_.var_iter_begin();
///                                        ivar != string_database_.var_iter_end(); ++ivar) {
///     std::string GroupName = string_database_.var_iter_gname(ivar);
///     std::string VarName = string_database_.var_iter_vname(ivar);
///     std::string GrpVarName = VarName + "@" + GroupName;
///     std::vector<std::size_t> VarShape = string_database_.var_iter_shape(ivar);
///     if (VarShape[0] > MaxVarSize) { MaxVarSize = VarShape[0]; }
///     std::vector<std::string> DbData(VarShape[0], "");
///     string_database_.LoadFromDb(GroupName, VarName, VarShape, DbData);
///     std::size_t MaxStringSize = FindMaxStringLength(DbData);
///     fileio->grp_var_insert(GroupName, VarName, "string", VarShape, GrpVarName, "string",
///                            MaxStringSize);
///   }
///   for (ObsSpaceContainer<util::DateTime>::VarIter ivar = datetime_database_.var_iter_begin();
///                                        ivar != datetime_database_.var_iter_end(); ++ivar) {
///     std::string GroupName = datetime_database_.var_iter_gname(ivar);
///     std::string VarName = datetime_database_.var_iter_vname(ivar);
///     std::string GrpVarName = VarName + "@" + GroupName;
///     std::vector<std::size_t> VarShape = datetime_database_.var_iter_shape(ivar);
///     if (VarShape[0] > MaxVarSize) { MaxVarSize = VarShape[0]; }
///     fileio->grp_var_insert(GroupName, VarName, "string", VarShape, GrpVarName, "string", 20);
///   }
///
///   // Build the frame info container
///   fileio->frame_info_init(MaxVarSize);
///
///   // For every frame, dump out the int, float, string variables.
///   for (IodaIO::FrameIter iframe = fileio->frame_begin();
///                          iframe != fileio->frame_end(); ++iframe) {
///     fileio->frame_data_init();
///     std::size_t FrameStart = fileio->frame_start(iframe);
///     std::size_t FrameSize = fileio->frame_size(iframe);
///
///     // Integer data
///     for (ObsSpaceContainer<int>::VarIter ivar = int_database_.var_iter_begin();
///                                          ivar != int_database_.var_iter_end(); ++ivar) {
///       std::string GroupName = int_database_.var_iter_gname(ivar);
///       std::string VarName = int_database_.var_iter_vname(ivar);
///       std::vector<std::size_t> VarShape = int_database_.var_iter_shape(ivar);
///
///       if (VarShape[0] > FrameStart) {
///         std::size_t Count = FrameSize;
///         if ((FrameStart + FrameSize) > VarShape[0]) { Count = VarShape[0] - FrameStart; }
///         std::vector<int> FrameData(Count, 0);
///         int_database_.LoadFromDb(GroupName, VarName, VarShape, FrameData, FrameStart, Count);
///         fileio->frame_int_put_data(GroupName, VarName, FrameData);
///       }
///     }
///
///     // Float data
///     for (ObsSpaceContainer<float>::VarIter ivar = float_database_.var_iter_begin();
///                                          ivar != float_database_.var_iter_end(); ++ivar) {
///       std::string GroupName = float_database_.var_iter_gname(ivar);
///       std::string VarName = float_database_.var_iter_vname(ivar);
///       std::vector<std::size_t> VarShape = float_database_.var_iter_shape(ivar);
///
///       if (VarShape[0] > FrameStart) {
///         std::size_t Count = FrameSize;
///         if ((FrameStart + FrameSize) > VarShape[0]) { Count = VarShape[0] - FrameStart; }
///         std::vector<float> FrameData(Count, 0.0);
///         float_database_.LoadFromDb(GroupName, VarName, VarShape, FrameData, FrameStart, Count);
///         fileio->frame_float_put_data(GroupName, VarName, FrameData);
///       }
///     }
///
///     // String data
///     for (ObsSpaceContainer<std::string>::VarIter ivar = string_database_.var_iter_begin();
///                                          ivar != string_database_.var_iter_end(); ++ivar) {
///       std::string GroupName = string_database_.var_iter_gname(ivar);
///       std::string VarName = string_database_.var_iter_vname(ivar);
///       std::vector<std::size_t> VarShape = string_database_.var_iter_shape(ivar);
///
///       if (VarShape[0] > FrameStart) {
///         std::size_t Count = FrameSize;
///         if ((FrameStart + FrameSize) > VarShape[0]) { Count = VarShape[0] - FrameStart; }
///         std::vector<std::string> FrameData(Count, "");
///         string_database_.LoadFromDb(GroupName, VarName, VarShape, FrameData,
///                                     FrameStart, Count);
///         fileio->frame_string_put_data(GroupName, VarName, FrameData);
///       }
///     }
///
///     for (ObsSpaceContainer<util::DateTime>::VarIter ivar = datetime_database_.var_iter_begin();
///                                        ivar != datetime_database_.var_iter_end(); ++ivar) {
///       std::string GroupName = datetime_database_.var_iter_gname(ivar);
///       std::string VarName = datetime_database_.var_iter_vname(ivar);
///       std::vector<std::size_t> VarShape = datetime_database_.var_iter_shape(ivar);
///
///       if (VarShape[0] > FrameStart) {
///         std::size_t Count = FrameSize;
///         if ((FrameStart + FrameSize) > VarShape[0]) { Count = VarShape[0] - FrameStart; }
///         util::DateTime TempDt("0000-01-01T00:00:00Z");
///         std::vector<util::DateTime> FrameData(Count, TempDt);
///         datetime_database_.LoadFromDb(GroupName, VarName, VarShape, FrameData,
///                                       FrameStart, Count);
///
///         // Convert the DateTime vector to a string vector, then save into the file.
///         std::vector<std::string> StringVector(FrameData.size(), "");
///         for (std::size_t i = 0; i < FrameData.size(); i++) {
///           StringVector[i] = FrameData[i].toString();
///         }
///         fileio->frame_string_put_data(GroupName, VarName, StringVector);
///       }
///     }
///
///     fileio->frame_write(iframe);
///   }
}

// -----------------------------------------------------------------------------
/*!
 * \details This sets a variable fill value. By default it uses the oops missing values
 *          according to type. For strings, there is a template specialization which
 *          uses a blank string.
 *
 * \param[out] FillValue Value (according to type) to use for a variable's fill value
 */
template<typename DataType>
void ObsData::GetFillValue(DataType & FillValue) const {
  FillValue = util::missingValue(FillValue);
}

template<>
void ObsData::GetFillValue<std::string>(std::string & FillValue) const {
  FillValue = std::string("_fill_");
}

// -----------------------------------------------------------------------------
/*!
 * \details This method applys the distribution index on data read from the input obs file.
 *          It is expected that when this method is called that the distribution index will
 *          have the process element and DA timing window effects accounted for.
 *
 * \param[in]  FullData     Vector holding data to be indexed
 * \param[in]  FullShape    Shape (dimension sizes) of FullData
 * \param[in]  Index        Index to be applied to FullData
 * \param[out] IndexedShape Shape (dimension sizes) of data after indexing
 */
template<typename VarType>
std::vector<VarType> ObsData::ApplyIndex(const std::vector<VarType> & FullData,
                              const std::vector<std::size_t> & FullShape,
                              const std::vector<std::size_t> & Index,
                              std::vector<std::size_t> & IndexedShape) const {
  std::vector<VarType> SelectedData;
  for (std::size_t i = 0; i < Index.size(); ++i) {
    std::size_t isrc = Index[i];
    SelectedData.push_back(FullData[isrc]);
  }
  IndexedShape = FullShape;
  IndexedShape[0] = SelectedData.size();
  return SelectedData;
}

// -----------------------------------------------------------------------------
/*!
 * \details This method will return the desired numeric data type for variables
 *          read from the input obs file. The rule for now is any variable
 *          in the group "PreQC" is to be an integer, and any variable that is
 *          a double is to be a float (single precision). For cases outside of this
 *          rule, the data type from the file is used.
 *
 * \param[in] GroupName   Name of obs container group
 * \param[in] FileVarType Name of the data type of the variable from the input obs file
 */
std::string ObsData::DesiredVarType(std::string & GroupName, std::string & FileVarType) {
  // By default, make the DbVarType equal to the FileVarType
  // Exceptions are:
  //   Force the group "PreQC" to an integer type.
  //   Force double to float.
  std::string DbVarType = FileVarType;

  if (GroupName == "PreQC") {
    DbVarType = "int";
  } else if (FileVarType == "double") {
    DbVarType = "float";
  }

  return DbVarType;
}

// -----------------------------------------------------------------------------
/*!
 * \details This method provides a means for printing Jo in
 *          an output stream. For now a dummy message is printed.
 */
void ObsData::printJo(const ObsVector & dy, const ObsVector & grad) {
  oops::Log::info() << "ObsData::printJo not implemented" << std::endl;
}
// -----------------------------------------------------------------------------
/*!
 * \details This method creates a private KDTree class member that can be used
 *          for searching for local obs to create an ObsSpace.
 */
void ObsData::createKDTree() {
  // Initialize KDTree class member
  kd_ = std::shared_ptr<ObsData::KDTree> ( new KDTree() );
  // Define lats,lons
  std::vector<float> lats(nlocs_);
  std::vector<float> lons(nlocs_);

  // Get latitudes and longitudes of all observations.
  this -> get_db("MetaData", "longitude", lons);
  this -> get_db("MetaData", "latitude", lats);

  // Define points list from lat/lon values
  typedef KDTree::PointType Point;
  std::vector<KDTree::Value> points;
  for (unsigned int i = 0; i < nlocs_; i++) {
    eckit::geometry::Point2 lonlat(lons[i], lats[i]);
    Point xyz = Point();
    // FIXME: get geometry from yaml, for now assume spherical Earth radius.
    atlas::util::Earth::convertSphericalToCartesian(lonlat, xyz);
    double index = static_cast<double>(i);
    KDTree::Value v(xyz, index);
    points.push_back(v);
  }

  // Create KDTree class member from points list.
  kd_->build(points.begin(), points.end());
}
// -----------------------------------------------------------------------------
/*!
 * \details This method returns the KDTree class member that can be used
 *          for searching for local obs when creating an ObsSpace.
 */
ObsData::KDTree & ObsData::getKDTree() {
  // Create the KDTree if it doesn't yet exist
  if (kd_ == NULL)
    createKDTree();

  return * kd_;
}
// -----------------------------------------------------------------------------

}  // namespace ioda
