/*
 * (C) Copyright 2017-2019 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/core/ObsData.h"

#include <algorithm>
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
                 const util::DateTime & bgn, const util::DateTime & end,
                 const eckit::mpi::Comm & timeComm)
                     : config_(config), winbgn_(bgn), winend_(end), commMPI_(comm),
                       gnlocs_(0), nlocs_(0), nrecs_(0), obsvars_(),
                       obs_group_(), obs_params_(bgn, end, comm, timeComm)
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
    if (obs_params_.out_type() == ObsIoTypes::OBS_FILE) {
        std::string fileName = obs_params_.top_level_.obsOutFile.value()->fileName;
        oops::Log::info() << obsname() << ": save database to " << fileName << std::endl;
        saveToFile(true);  // argument set to true will write in old format
    } else {
        oops::Log::info() << obsname() << " :  no output" << std::endl;
    }
    oops::Log::trace() << "ObsData::ObsData destructor" << std::endl;
}

// -----------------------------------------------------------------------------
std::size_t ObsData::nvars() const {
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
    std::vector<std::string> dtStrings;
    Variable var = obs_group_.vars.open(fullVarName(group, name));
    var.read<std::string>(dtStrings);
    vdata = convertDtStringsToDtime(dtStrings);
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
  std::vector<std::string> dtStrings(vdata.size(), "");
  for (std::size_t i = 0; i < vdata.size(); ++i) {
    dtStrings[i] = vdata[i].toString();
  }
  Variable var = openCreateVar<std::string>(fullVarName(group, name));
  var.write<std::string>(dtStrings);
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
    std::vector<std::string> dimVarList = obsFrame->ioDimVarList();

    // Form a map containing a list of attached dims for each variable
    VarDimMap dimsAttachedToVars = genDimsAttachedToVars(obsFrame->vars(), varList, dimVarList);

    // Create variables in obs_group_ based on those in the obs source
    createVariables(obsFrame->vars(), obs_group_.vars, dimsAttachedToVars);

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
            if (obsFrame->ioIsVarDimByNlocs(varName)) {
                beFrameStart = obsFrame->adjNlocsFrameStart();
            } else {
                beFrameStart = frameStart;
            }
            Dimensions_t frameCount = obsFrame->frameCount(varName);
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

    nlocs_ = obs_group_.vars.open("nlocs").getDimensions().dimsCur[0];
    nrecs_ = obsFrame->frameNumRecs();
    indx_ = obsFrame->index();
    recnums_ = obsFrame->recnums();

    // TODO(SRH) Eliminate this temporary fix. Some files do not have ISO 8601 date time
    // strings. Instead they have a reference date time and time offset. If there is no
    // datetime variable in the obs_group_ after reading in the file, then create one
    // from the reference/offset time values.
    std::string dtVarName = fullVarName("MetaData", "datetime");
    if (!obs_group_.vars.exists(dtVarName)) {
        int refDtime;
        obsFrame->atts().open("date_time").read<int>(refDtime);

        std::vector<float> timeOffset;
        Variable timeVar = obs_group_.vars.open(fullVarName("MetaData", "time"));
        timeVar.read<float>(timeOffset);

        std::vector<util::DateTime> dtVals = convertRefOffsetToDtime(refDtime, timeOffset);
        std::vector<std::string> dtStrings(dtVals.size(), "");
        for (std::size_t i = 0; i < dtVals.size(); ++i) {
            dtStrings[i] = dtVals[i].toString();
        }

        VariableCreationParameters params;
        params.chunk = true;
        params.compressWithGZIP();
        params.setFillValue<std::string>(this->getFillValue<std::string>());
        std::vector<Variable> dimVars(1, obs_group_.vars.open("nlocs"));
        obs_group_.vars
            .createWithScales<std::string>(dtVarName, dimVars, params)
            .write<std::string>(dtStrings);
    }

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
void ObsData::createVariables(const Has_Variables & srcVarContainer,
                              Has_Variables & destVarContainer,
                              const VarDimMap & dimsAttachedToVars,
                              const bool useOldFormat) {
    // Walk through map to get list of variables to create along with
    // their dimensions. Use the srcVarContainer to get the var data type.
    for (auto & ivar : dimsAttachedToVars) {
        std::string varName = ivar.first;
        std::vector<std::string> varDimNames = ivar.second;

        VariableCreationParameters params;
        params.chunk = true;
        params.compressWithGZIP();

        // Create a vector with dimension scale vector from destination container
        std::vector<Variable> varDims;
        for (auto & dimVarName : varDimNames) {
            varDims.push_back(destVarContainer.open(dimVarName));
        }

        Variable srcVar = srcVarContainer.open(varName);
        std::string destVarName;
        if (useOldFormat) {
            destVarName = convertNewVnameToOldVname(varName);
        } else {
            destVarName = varName;
        }
        if (srcVar.isA<int>()) {
            params.setFillValue<int>(this->getFillValue<int>());
            destVarContainer.createWithScales<int>(destVarName, varDims, params);
        } else if (srcVar.isA<float>()) {
            params.setFillValue<float>(this->getFillValue<float>());
            destVarContainer.createWithScales<float>(destVarName, varDims, params);
        } else if (srcVar.isA<std::string>()) {
            params.setFillValue<std::string>(this->getFillValue<std::string>());
            // If we have a new style string variable (stored in file as vector of strings),
            // then use varDims as is. If we have an old style string variable (stored in
            // file as a 2D string array), the target variable will be a 1D vector of
            // strings, so strip of the extra dimension. In either case, varDims
            // should contain just one element.
            varDims.resize(1);
            destVarContainer.createWithScales<std::string>(destVarName, varDims, params);
        } else {
            if (this->comm().rank() == 0) {
                oops::Log::warning() << "WARNING: ObsData::createVariables: "
                    << "Skipping variable due to an unexpected data type for variable: "
                    << varName << std::endl;
            }
        }
    }
}

// -----------------------------------------------------------------------------
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
void ObsData::saveToFile(const bool useOldFormat) {
    // Form lists of regular and dimension scale variables
    std::vector<std::string> varList = listVars(obs_group_);
    std::vector<std::string> dimVarList = listDimVars(obs_group_);

    // Record dimension scale variables for the output file creation.
    for (auto & dimName : dimVarList) {
        Dimensions_t dimSize = obs_group_.vars.open(dimName).getDimensions().dimsCur[0];
        Dimensions_t dimMaxSize = dimSize;
        if (dimName == "nlocs") {
            dimMaxSize = Unlimited;
        }
        obs_params_.setDimScale(dimName, dimSize, dimMaxSize, dimSize);
    }

    // Record the maximum variable size
    Dimensions_t maxVarSize = 0;
    for (auto & varName : varList) {
        Variable var = obs_group_.vars.open(varName);
        Dimensions_t varSize0 = var.getDimensions().dimsCur[0];
        if (varSize0 > maxVarSize) {
            maxVarSize = varSize0;
        }
    }
    obs_params_.setMaxVarSize(maxVarSize);

    // Open the file for output
    std::shared_ptr<ObsFrame> obsFrame =
        ObsFrameFactory::create(ObsIoActions::CREATE_FILE, ObsIoModes::CLOBBER, obs_params_);

    // Create a map showing list of dimension scale variables attached to each variable
    // in the obs_group_. Then create the variables.
    VarDimMap dimsAttachedToVars = genDimsAttachedToVars(obs_group_.vars, varList, dimVarList);
    createVariables(obs_group_.vars, obsFrame->vars(), dimsAttachedToVars, useOldFormat);

    // Iterate through the frames and variables moving data from the database into
    // the file.
    int iframe = 0;
    for (obsFrame->frameInit(); obsFrame->frameAvailable(); obsFrame->frameNext()) {
        Dimensions_t frameStart = obsFrame->frameStart();
        for (auto & varName : varList) {
            // form the destination (ObsFrame) variable name
            std::string destVarName;
            if (useOldFormat) {
                destVarName = convertNewVnameToOldVname(varName);
            } else {
                destVarName = varName;
            }

            // open the destination variable and get the associate count
            Variable destVar = obsFrame->vars().open(destVarName);
            Dimensions_t frameCount = obsFrame->frameCount(destVarName);

            // transfer data if we haven't gone past the end of the variable yet
            if (frameCount > 0) {
                // Form the hyperslab selection for this frame
                ioda::Selection frontendSelect;
                ioda::Selection backendSelect;
                obsFrame->createFrameSelection(destVarName, frontendSelect, backendSelect);

                // transfer the data
                Variable srcVar = obs_group_.vars.open(varName);
                if (srcVar.isA<int>()) {
                    std::vector<int> varValues;
                    srcVar.read<int>(varValues, frontendSelect, backendSelect);
                    destVar.write<int>(varValues, frontendSelect, backendSelect);
                } else if (srcVar.isA<float>()) {
                    std::vector<float> varValues;
                    srcVar.read<float>(varValues, frontendSelect, backendSelect);
                    destVar.write<float>(varValues, frontendSelect, backendSelect);
                } else if (srcVar.isA<std::string>()) {
                    std::vector<std::string> varValues;
                    srcVar.read<std::string>(varValues, frontendSelect, backendSelect);
                    destVar.write<std::string>(varValues, frontendSelect, backendSelect);
                }
            }
        }
    }
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
