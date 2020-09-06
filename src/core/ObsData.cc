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

#include "oops/parallel/mpi/mpi.h"
#include "oops/util/abor1_cpp.h"
#include "oops/util/DateTime.h"
#include "oops/util/Duration.h"
#include "oops/util/Logger.h"
#include "oops/util/Random.h"
#include "oops/util/stringFunctions.h"

#include "ioda/distribution/DistributionFactory.h"
#include "ioda/io/ObsIoFactory.h"
#include "ioda/Misc/Dimensions.h"
#include "ioda/Variables/Variable.h"

#include "atlas/util/Earth.h"

namespace ioda {

// ----------------------------- public functions ------------------------------
// -----------------------------------------------------------------------------
/*!
 * \details Config based constructor for an ObsData object. This constructor will read
 *          in from the obs file and transfer the variables into the obs container. Obs
 *          falling outside the DA timing window, specified by bgn and end, will be
 *          discarded before storing them in the obs container.
 *
 * \param[in] config ECKIT configuration segment holding obs types specs
 * \param[in] bgn    DateTime object holding the start of the DA timing window
 * \param[in] end    DateTime object holding the end of the DA timing window
 */
ObsData::ObsData(const eckit::Configuration & config, const eckit::mpi::Comm & comm,
                 const util::DateTime & bgn, const util::DateTime & end)
    : config_(config), winbgn_(bgn), winend_(end), commMPI_(comm),
      gnlocs_(0), nlocs_(0), nvars_(0), nrecs_(0), obsvars_(),
      next_rec_num_(0), obs_group_(), obs_params_(bgn, end, comm)
{
    oops::Log::trace() << "ObsData::ObsData config  = " << config << std::endl;
    obs_params_.deserialize(config);

    obsname_ = obs_params_.top_level_.obsSpaceName;
    obsvars_ = oops::Variables(config, "simulated variables");
    oops::Log::info() << this->obsname() << " vars: " << obsvars_ << std::endl;

    // Create the MPI distribution object
    std::unique_ptr<DistributionFactory> distFactory;
    dist_.reset(distFactory->createDistribution(this->comm(), this->distname()));

    // Open the source (ObsIo) of the data for initializing the obs_group_ (ObsGroup)
    std::shared_ptr<ObsIo> obsIo;
    if (obs_params_.in_type() == ObsIoTypes::OBS_FILE) {
      obsIo = ObsIoFactory::create(ObsIoActions::OPEN_FILE, ObsIoModes::READ_ONLY, obs_params_);
    } else if ((obs_params_.in_type() == ObsIoTypes::GENERATOR_RANDOM) ||
               (obs_params_.in_type() == ObsIoTypes::GENERATOR_LIST)) {
      obsIo = ObsIoFactory::create(ObsIoActions::CREATE_GENERATOR, ObsIoModes::READ_ONLY,
          obs_params_);
    } else {
      // Error - must have one of obsdatain or generate
      std::string ErrorMsg =
        "ObsData::ObsData: Must use one of 'obsdatain' or 'generate' in the YAML configuration.";
      ABORT(ErrorMsg);
    }

    createObsGroupFromObsIo(obsIo);
    initFromObsSource(obsIo);

    gnlocs_ = obsIo->nlocs();

    if (this->obs_sort_var() != "") {
      BuildSortedObsGroups();
    }

    oops::Log::trace() << "ObsData::ObsData contructed name = " << obsname() << std::endl;
}

// -----------------------------------------------------------------------------
std::size_t ObsData::nlocs() const {
    return obs_group_.vars.open("nlocs").getDimensions().dimsCur[0];
}

// -----------------------------------------------------------------------------
std::size_t ObsData::nrecs() const {
    // TODO(srh) placeholder until distribution is working
    return obs_group_.vars.open("nlocs").getDimensions().dimsCur[0];
}

// -----------------------------------------------------------------------------
std::size_t ObsData::nvars() const {
    Group g = obs_group_.open("ObsValue");
    return g.vars.list().size();
}



// -----------------------------------------------------------------------------
/*!
 * \details Destructor for an ObsData object. This destructor will clean up the ObsData
 *          object and optionally write out the contents of the obs container into
 *          the output file. The save-to-file operation is invoked when an output obs
 *          file is specified in the ECKIT configuration segment associated with the
 *          ObsData object.
 */
ObsData::~ObsData() {
  oops::Log::trace() << "ObsData::ObsData destructor begin" << std::endl;
  if (fileout_.size() != 0) {
    oops::Log::info() << obsname() << ": save database to " << fileout_ << std::endl;
    // SaveToFile(fileout_, out_max_frame_size_);
  } else {
    oops::Log::info() << obsname() << " :  no output" << std::endl;
  }
  oops::Log::trace() << "ObsData::ObsData destructor end" << std::endl;
}

// -----------------------------------------------------------------------------
/*!
 * \brief transfer data from the obs container to vdata
 *
 * \details The following four get_db methods are the same except for the data type
 *          of the data being transferred (integer, float, double, DateTime). The
 *          caller needs to allocate the memory that the vdata parameter points to.
 *
 * \param[in]  group Name of container group (ObsValue, ObsError, MetaData, etc.)
 * \param[in]  name  Name of container variable
 * \param[out] vdata Vector where container data is being transferred to
 */

void ObsData::get_db(const std::string & group, const std::string & name,
                      std::vector<int> & vdata) const {
  LoadFromDb<int>(group, name, { vdata.size() }, vdata);
}

void ObsData::get_db(const std::string & group, const std::string & name,
                      std::vector<float> & vdata) const {
  LoadFromDb<float>(group, name, { vdata.size() }, vdata);
}

void ObsData::get_db(const std::string & group, const std::string & name,
                      std::vector<double> & vdata) const {
  // load the float values from the database and convert to double
  std::vector<float> FloatData(vdata.size(), 0.0);
  LoadFromDb<float>(group, name, { vdata.size() }, FloatData);
  ConvertVarType<float, double>(FloatData, vdata);
}

void ObsData::get_db(const std::string & group, const std::string & name,
                      std::vector<std::string> & vdata) const {
  LoadFromDb<std::string>(group, name, { vdata.size() }, vdata);
}

void ObsData::get_db(const std::string & group, const std::string & name,
                      std::vector<util::DateTime> & vdata) const {
  // datetime_database_.LoadFromDb(group, name, { vdata.size() }, vdata);
}

// -----------------------------------------------------------------------------
/*!
 * \brief transfer data from vdata to the obs container
 *
 * \details The following four put_db methods are the same except for the data type
 *          of the data being transferred (integer, float, double, DateTime). The
 *          caller needs to allocate and assign the memory that the vdata parameter
 *          points to.
 *
 * \param[in]  group Name of container group (ObsValue, ObsError, MetaData, etc.)
 * \param[in]  name  Name of container variable
 * \param[out] vdata Vector where container data is being transferred from
 */

void ObsData::put_db(const std::string & group, const std::string & name,
                      const std::vector<int> & vdata) {
  StoreToDb<int>(group, name, { vdata.size() }, vdata);
}

void ObsData::put_db(const std::string & group, const std::string & name,
                      const std::vector<float> & vdata) {
  StoreToDb<float>(group, name, { vdata.size() }, vdata);
}

void ObsData::put_db(const std::string & group, const std::string & name,
                      const std::vector<double> & vdata) {
  // convert to float, then load into the database
  std::vector<float> FloatData(vdata.size());
  ConvertVarType<double, float>(vdata, FloatData);
  StoreToDb<float>(group, name, { vdata.size() }, FloatData);
}

void ObsData::put_db(const std::string & group, const std::string & name,
                      const std::vector<std::string> & vdata) {
  StoreToDb<std::string>(group, name, { vdata.size() }, vdata);
}

void ObsData::put_db(const std::string & group, const std::string & name,
                      const std::vector<util::DateTime> & vdata) {
  // StoreToDb<util::DateTime>(group, name, { vdata.size() }, vdata);
}

// -----------------------------------------------------------------------------
/*!
 * \details This method checks for the existence of the group, name combination
 *          in the obs container. If the combination exists, "true" is returned,
 *          otherwise "false" is returned.
 */

bool ObsData::has(const std::string & group, const std::string & name) const {
  std::string groupVar = group + "/" + name;
  return obs_group_.vars.exists(groupVar);
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the data type of the variable stored in the obs
 *          container.
 */

ObsDtype ObsData::dtype(const std::string & group, const std::string & name) const {
  // Set the type to None if there is no type from the backend
  std::string groupVar = group + "/" + name;
  ObsDtype VarType = ObsDtype::None;
  if (has(group, name)) {
    Variable var = obs_group_.vars.open(groupVar);
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
/*!
 * \details This method returns a reference to the record number vector
 *          data member. This is for read only access.
 */
const std::vector<std::size_t> & ObsData::recnum() const {
  return recnums_;
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns a reference to the index vector
 *          data member. This is for read only access.
 */
const std::vector<std::size_t> & ObsData::index() const {
  return indx_;
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
void ObsData::createObsGroupFromObsIo(const std::shared_ptr<ObsIo> & obsIo) {
    // Create the dimension specs for obs_group_
    NewDimensionScales_t newDims;
    for (auto & dimName : listDimVars(obsIo->obs_group_)) {
        Variable var = obsIo->obs_group_.vars.open(dimName);
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
//  Engines::BackendNames backendName = Engines::BackendNames::ObsStore;
//  Engines::BackendCreationParameters backendParams;
//  Group backend = constructBackend(backendName, backendParams);
    Engines::BackendNames backendName = Engines::BackendNames::Hdf5File;
    Engines::BackendCreationParameters backendParams;
    backendParams.fileName = "test_obsspace.hdf5";
    backendParams.action = Engines::BackendFileActions::Create;
    backendParams.createMode = Engines::BackendCreateModes::Truncate_If_Exists;
    Group backend = constructBackend(backendName, backendParams);

    // Create the ObsGroup and attach the backend.
    obs_group_ = ObsGroup::generate(backend, newDims);
}

// -----------------------------------------------------------------------------
void ObsData::initFromObsSource(const std::shared_ptr<ObsIo> & obsIo) {
    // Walk through the frames and copy the data to the obs_group_ storage
    std::vector<std::string> varList = listVars(obsIo->obs_group_);
    int iframe = 1;
    for (obsIo->frameInit(); obsIo->frameAvailable(); obsIo->frameNext()) {
        Dimensions_t frameStart = obsIo->frameStart();
        oops::Log::debug() << "ObsData::initFromObsSource: Frame number: "
            << iframe << std::endl
            << "    frameStart: " << frameStart << std::endl;

        bool resizedNlocs = false;
        for (auto & varName : varList) {
            Variable var = obsIo->obs_group_.vars.open(varName);
            Dimensions_t frameCount = obsIo->frameCount(var);
            if (frameCount > 0) {
                oops::Log::debug() << "ObsData::initFromObsSource: Variable: " << varName
                    << ", frameCount: " << frameCount << std::endl;

                // resize nlocs in anticipation of storing variables along nlocs
                // TODO(srh) need to do distribution, timing window filtering before
                // this and get an adjusted frame count for use downstream.
                if (!resizedNlocs) {
                    resizedNlocs = resizeNlocs(obsIo, varName, frameCount, (iframe > 1));
                }

                // Transfer the variable to the in-memory storage
                if (var.isA<int>()) {
                    std::vector<int> varValues;
                    readObsSource<int>(obsIo, varName, varValues);
                    if (iframe == 1) { createVarFromObsSource<int>(obsIo, varName); }
                    storeVar<int>(varName, varValues, frameStart, frameCount);
                } else if (var.isA<float>()) {
                    std::vector<float> varValues;
                    readObsSource<float>(obsIo, varName, varValues);
                    if (iframe == 1) { createVarFromObsSource<float>(obsIo, varName); }
                    storeVar<float>(varName, varValues, frameStart, frameCount);
                } else if (var.isA<std::string>()) {
                    std::vector<std::string> varValues;
                    readObsSource<std::string>(obsIo, varName, varValues);
                    if (iframe == 1) { createVarFromObsSource<std::string>(obsIo, varName); }
                    storeVar<std::string>(varName, varValues, frameStart, frameCount);
                } else {
                    if ((iframe == 1) && (this->comm().rank() == 0)) {
                        oops::Log::warning() << "ObsData::initFromObsSource: "
                            << "Skipping read due to an unexpected data type for variable: "
                            << varName << std::endl;
                    }
                }
            }
        }

        iframe++;
    }
}

// -----------------------------------------------------------------------------
bool ObsData::resizeNlocs(const std::shared_ptr<ObsIo> & obsIo, const std::string & varName,
                          const Dimensions_t nlocsSize, const bool append) {
    bool resized;
    Variable sourceVar = obsIo->obs_group_.vars.open(varName);
    Variable sourceNlocsVar = obsIo->obs_group_.vars.open("nlocs");
    if (sourceVar.isDimensionScaleAttached(0, sourceNlocsVar)) { 
        Variable nlocsVar = obs_group_.vars.open("nlocs");
        Dimensions_t nlocsResize;
        if (append) {
            nlocsResize = nlocsVar.getDimensions().dimsCur[0] + nlocsSize;
        } else {
            nlocsResize = nlocsSize;
        }
        obs_group_.resize(
            { std::pair<Variable, Dimensions_t>(nlocsVar, nlocsResize) });
        resized= true;
    } else {
        resized= false;
    }
    return resized;
}

// -----------------------------------------------------------------------------
std::vector<Variable> ObsData::setVarDimsFromObsSource(const std::shared_ptr<ObsIo> & obsIo,
                                                       const std::string & varName) {
    // Get a list of dimension variables from the source (obsIo)
    std::map<std::string, Variable> srcDimVars;
    for (auto & dimVarName : listDimVars(obsIo->obs_group_)) {
        Variable dimVar = obsIo->obs_group_.vars.open(dimVarName);
        srcDimVars.insert(std::pair<std::string, Variable>(dimVarName, dimVar));
    }

    // find which dimensions are attached to the source variable
    Variable sourceVar = obsIo->obs_group_.vars.open(varName);
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
/*!
 * \details This method will initialize the obs container from the input obs file.
 *          All the variables from the input file will be read in and loaded into
 *          the obs container. Obs that fall outside the DA timing window will be
 *          filtered out before loading into the container. This method will also
 *          apply obs distribution across multiple process elements. For these reasons,
 *          the number of locations in the obs container may be smaller than the
 *          number of locations in the input obs file.
 *
 * \param[in] filename Path to input obs file
 */
/// void ObsData::InitFromFile(const std::string & filename, const std::size_t MaxFrameSize) {
///   oops::Log::trace() << "ObsData::InitFromFile opening file: " << filename << std::endl;

///   // Open the file for reading and record nlocs and nvars from the file.
///   std::unique_ptr<IodaIO> fileio {ioda::IodaIOfactory::Create(filename, "r", MaxFrameSize)};
///   gnlocs_ = fileio->nlocs();

///   // Walk through the frames and select the records according to the MPI distribution
///   // and if the records fall inside the DA timing window. nvars_ for ObsData is the
///   // number of variables with the GroupName ObsValue. Since we can be reading in
///   // multiple frames, only check for the ObsValue group on the first frame.
///   nvars_ = 0;
///   bool FirstFrame = true;
///   fileio->frame_initialize();
///   for (IodaIO::FrameIter iframe = fileio->frame_begin();
///                        iframe != fileio->frame_end(); ++iframe) {
///     std::size_t FrameStart = fileio->frame_start(iframe);
///     std::size_t FrameSize = fileio->frame_size(iframe);

///     // Fill in the current frame from the file
///     fileio->frame_read(iframe);

///     // Calculate the corresponding segments of indx_ and recnums_ vectors for this
///     // frame. Use these segments to select the rows from the frame before storing in
///     // the obs space container.
/// //    std::vector<std::size_t> FrameIndex = GenFrameIndexRecNums(fileio, FrameStart, FrameSize);

///     // Integer variables
///     for (IodaIO::FrameIntIter idata = fileio->frame_int_begin();
///                               idata != fileio->frame_int_end(); ++idata) {
///       std::string GroupName = fileio->frame_int_get_gname(idata);
///       if (FirstFrame && (GroupName == "ObsValue")) { nvars_++; }
///       std::string VarName = fileio->frame_int_get_vname(idata);
///       std::vector<std::size_t> VarShape = fileio->var_shape(GroupName, VarName);
///       std::vector<int> FrameData;
///       fileio->frame_int_get_data(GroupName, VarName, FrameData);
///       std::vector<std::size_t> FrameShape = VarShape;
///       FrameShape[0] = FrameData.size();
///       if (VarShape[0] == gnlocs_) {
///         std::vector<std::size_t> IndexedShape;
///         std::vector<int> SelectedData =
///              ApplyIndex(FrameData, VarShape, FrameIndex, FrameShape);
///         StoreToDb<int>(GroupName, VarName, FrameShape, SelectedData, true);
///       } else {
///         StoreToDb<int>(GroupName, VarName, FrameShape, FrameData, true);
///       }
///     }

///     // Float variables
///     for (IodaIO::FrameFloatIter idata = fileio->frame_float_begin();
///                                 idata != fileio->frame_float_end(); ++idata) {
///       std::string GroupName = fileio->frame_float_get_gname(idata);
///       if (FirstFrame && (GroupName == "ObsValue")) { nvars_++; }
///       std::string VarName = fileio->frame_float_get_vname(idata);
///       std::vector<std::size_t> VarShape = fileio->var_shape(GroupName, VarName);
///       std::vector<float> FrameData;
///       fileio->frame_float_get_data(GroupName, VarName, FrameData);
///       std::vector<std::size_t> FrameShape = VarShape;
///       FrameShape[0] = FrameData.size();
///       if (VarShape[0] == gnlocs_) {
///         std::vector<std::size_t> IndexedShape;
///         std::vector<float> SelectedData =
///              ApplyIndex(FrameData, VarShape, FrameIndex, FrameShape);
///         StoreToDb<float>(GroupName, VarName, FrameShape, SelectedData, true);
///       } else {
///         StoreToDb<float>(GroupName, VarName, FrameShape, FrameData, true);
///       }
///     }

///     // String variables
///     for (IodaIO::FrameStringIter idata = fileio->frame_string_begin();
///                                  idata != fileio->frame_string_end(); ++idata) {
///       std::string GroupName = fileio->frame_string_get_gname(idata);
///       if (FirstFrame && (GroupName == "ObsValue")) { nvars_++; }
///       std::string VarName = fileio->frame_string_get_vname(idata);
///       std::vector<std::size_t> VarShape = fileio->var_shape(GroupName, VarName);
///       std::vector<std::string> FrameData;
///       fileio->frame_string_get_data(GroupName, VarName, FrameData);
///       std::vector<std::size_t> FrameShape = VarShape;
///       FrameShape[0] = FrameData.size();
///       if (VarShape[0] == gnlocs_) {
///         std::vector<std::size_t> IndexedShape;
///         std::vector<std::string> SelectedData =
///              ApplyIndex(FrameData, VarShape, FrameIndex, FrameShape);
///         StoreToDb<std::string>(GroupName, VarName, FrameShape, SelectedData, true);
///       } else {
///         StoreToDb<std::string>(GroupName, VarName, FrameShape, FrameData, true);
///       }
///     }
///     FirstFrame = false;
///   }
///   fileio->frame_finalize();

///   // Record whether any problems occurred when reading the file.
///   file_unexpected_dtypes_ = fileio->unexpected_data_types();
///   file_excess_dims_ = fileio->excess_dims();
///   oops::Log::trace() << "ObsData::InitFromFile opening file ends " << std::endl;
/// }

// -----------------------------------------------------------------------------
/*!
 * \details This method will transfer data into the in-memory storage.
 *
 * \param[in] GroupName Name of group from top level (ObsValue, ObsError, etc)
 * \param[in] VarName Name of variable that goes under the group
 * \param[in] VarShape Sizes of variable dimensions
 * \param[in] VarData Variable data values
 * \param[in] Append If true then append the variable to the existing variable
 */
template<typename VarType>
void ObsData::StoreToDb(const std::string & GroupName, const std::string & VarName,
                        const std::vector<std::size_t> & VarShape,
                        const std::vector<VarType> & VarData, bool Append) {
  // Need to copy VarShape into a dimension spec
  std::vector<Dimensions_t> VarDims(VarShape.begin(), VarShape.end());

  // Create the group if it doesn't exist
  Group grp;
  if (!obs_group_.exists(GroupName)) {
    grp = obs_group_.create(GroupName);
  } else {
    grp = obs_group_.open(GroupName);
  }

  // Create the variable if it doesn't exist
  Variable var;
  if (!grp.vars.exists(VarName)) {
    // Need to set fill value to the appropriate missing value
    VarType FillValue;
    GetFillValue<VarType>(FillValue);
    VariableCreationParameters VarParams;
    VarParams.setFillValue<VarType>(FillValue);

    // Create and write the variable. Since it is new, the Append control
    // doesn't matter. Regardless of its setting, you want to simply write
    // the variable into the space that was just created.
    var = grp.vars.create<VarType>(VarName, VarDims, VarDims, VarParams);
    var.write(VarData);
  } else {
    var = grp.vars.open(VarName);
    if (Append) {
      // Need to add the existing old and new dimensions to get the
      // dimensions for resizing. Then write the new data into the new
      // space that was opened up.
      std::vector<Dimensions_t> CurDims = var.getDimensions().dimsCur;
      std::vector<Dimensions_t> NewDims = CurDims;
      std::vector<Dimensions_t> Counts(CurDims.size());
      for (std::size_t i = 0; i < NewDims.size(); ++i) {
        NewDims[i] += VarDims[i];
        Counts[i] = VarDims[i];
      }
      var.resize(NewDims);

      // Create a selection object for the ObsStore ("file") side. Use
      // the default ALL for the memory side.
      var.write(VarData, Selection::all,
                Selection().select({ SelectionOperator::SET, CurDims, Counts }));

    } else {
      var.write(VarData);
    }
  }
}

// -----------------------------------------------------------------------------
/*!
 * \details This method will transfer data from the in-memory storage.
 *
 * \param[in]  GroupName Name of group from top level (ObsValue, ObsError, etc)
 * \param[in]  VarName Name of variable that goes under the group
 * \param[in]  VarShape Sizes of variable dimensions
 * \param[out] VarData Variable data values
 */
template<typename VarType>
void ObsData::LoadFromDb(const std::string & GroupName, const std::string & VarName,
                  const std::vector<std::size_t> & VarShape, std::vector<VarType> & VarData,
                  const std::size_t Start, const std::size_t Count) const {
  // Need to copy VarShape into a dimension spec
  std::vector<Dimensions_t> VarDims(VarShape.size());
  for (std::size_t i = 0; i < VarShape.size(); ++i) {
    VarDims[i] = VarShape[i];
  }

  // Open the group and variable
  Group grp = obs_group_.open(GroupName);
  Variable var = grp.vars.open(VarName);

  // Read the variable
  Selection MemSelect = Selection::all;
  Selection FileSelect;
  if (Count > 0) {
    std::vector<Dimensions_t> Starts(1, Start);
    std::vector<Dimensions_t> Counts(1, Count);
    FileSelect.select({ SelectionOperator::SET, Starts, Counts });
  } else {
    FileSelect = Selection::all;
  }
  var.read(VarData, MemSelect, FileSelect);
}

/// // -----------------------------------------------------------------------------
/// /*!
///  * \details This method generates an list of indices with their corresponding
///  *          record numbers, where the indices denote which locations are to be
///  *          read into this process element.
///  *
///  * \param[in] FileIO File id (pointer to IodaIO object)
///  * \param[in] FrameStart Row number at beginning of frame.
///  * \param[out] FrameIndex Vector of indices indicating rows belonging to this process element
///  * \param[out] FrameRecNums Vector containing record numbers corresponding to FrameIndex
///  */
/// std::vector<std::size_t> ObsData::GenFrameIndexRecNums(const std::unique_ptr<IodaIO> & FileIO,
///                                  const std::size_t FrameStart, const std::size_t FrameSize) {
///   // It's possible that the total number of locations (gnlocs_) is smaller than
///   // another dimension (eg, nchans or nvars for a hyperspectral instrument). If that
///   // is the case, we don't want to read past the end of the datetime or obs group
///   // variable which are dimensioned by nlocs.
///   std::size_t LocSize = FrameSize;
///   if ((FrameStart + FrameSize) > gnlocs_) { LocSize = gnlocs_ - FrameStart; }

///   // Apply the timing window if we are reading from a file. Need to filter out locations
///   // that are outside the timing window before generating record numbers. This is because
///   // we are generating record numbers on the fly since we want to get to the point where
///   // we can do the MPI distribution without knowing how many obs (and records) we are going
///   // to encounter.
///   //
///   // Create two vectors as the timing windows are checked, one for location indices the
///   // other for frame indices. Location indices are relative to FrameStart, and frame
///   // indices are relative to this frame (start at zero).
///   //
///   // If we are not reading from a file, then load up the locations and frame indices
///   // with all locations in the frame.
///   std::vector<std::size_t> LocIndex;
///   std::vector<std::size_t> FrameIndex;
///   if (FileIO != nullptr) {
///     // Grab the datetime strings for checking the timing window
///     std::string DtGroupName = "MetaData";
///     std::string DtVarName = "datetime";
///     std::vector<std::string> DtStrings;
///     FileIO->frame_string_get_data(DtGroupName, DtVarName, DtStrings);

///     // Convert the datetime strings to DateTime objects
///     std::vector<util::DateTime> ObsDtimes;
///     for (std::size_t i = 0; i < DtStrings.size(); ++i) {
///       util::DateTime ObsDt(DtStrings[i]);
///       ObsDtimes.push_back(ObsDt);
///     }

///     // Keep all locations that fall inside the timing window
///     for (std::size_t i = 0; i < LocSize; ++i) {
///       if (InsideTimingWindow(ObsDtimes[i])) {
///         LocIndex.push_back(FrameStart + i);
///         FrameIndex.push_back(i);
///       }
///     }
///     LocSize = LocIndex.size();  // in case any locations were rejected
///   } else {
///     // Not reading from file, keep all locations.
///     LocIndex.assign(LocSize, 0);
///     std::iota(LocIndex.begin(), LocIndex.end(), FrameStart);

///     FrameIndex.assign(LocSize, 0);
///     std::iota(FrameIndex.begin(), FrameIndex.end(), 0);
///   }

///   // Generate record numbers for this frame
///   std::vector<std::size_t> Records(LocSize);
///   if ((obs_group_variable_.empty()) || (FileIO == nullptr)) {
///     // Grouping is not specified, so use the location indices as the record indicators.
///     // Using the obs grouping object will make the record numbering go sequentially
///     // from 0 to nrecs_ - 1.
///     for (std::size_t i = 0; i < LocSize; ++i) {
///       int RecValue = LocIndex[i];
///       if (!int_obs_grouping_.has(RecValue)) {
///         int_obs_grouping_.insert(RecValue, next_rec_num_);
///         next_rec_num_++;
///         nrecs_ = next_rec_num_;
///       }
///       Records[i] = int_obs_grouping_.at(RecValue);
///     }
///   } else {
///     // Group according to the data in obs_group_variable_
///     std::string GroupName = "MetaData";
///     std::string VarName = obs_group_variable_;
///     std::string VarType = FileIO->var_dtype(GroupName, VarName);

///     if (VarType == "int") {
///       std::vector<int> GroupVar;
///       FileIO->frame_int_get_data(GroupName, VarName, GroupVar);
///       for (std::size_t i = 0; i < LocSize; ++i) {
///         int RecValue = GroupVar[FrameIndex[i]];
///         if (!int_obs_grouping_.has(RecValue)) {
///           int_obs_grouping_.insert(RecValue, next_rec_num_);
///           next_rec_num_++;
///           nrecs_ = next_rec_num_;
///         }
///         Records[i] = int_obs_grouping_.at(RecValue);
///       }
///     } else if (VarType == "float") {
///       std::vector<float> GroupVar;
///       FileIO->frame_float_get_data(GroupName, VarName, GroupVar);
///       for (std::size_t i = 0; i < LocSize; ++i) {
///         float RecValue = GroupVar[FrameIndex[i]];
///         if (!float_obs_grouping_.has(RecValue)) {
///           float_obs_grouping_.insert(RecValue, next_rec_num_);
///           next_rec_num_++;
///         }
///         Records[i] = float_obs_grouping_.at(RecValue);
///       }
///     } else if (VarType == "string") {
///       std::vector<std::string> GroupVar;
///       FileIO->frame_string_get_data(GroupName, VarName, GroupVar);
///       for (std::size_t i = 0; i < LocSize; ++i) {
///         std::string RecValue = GroupVar[FrameIndex[i]];
///         if (!string_obs_grouping_.has(RecValue)) {
///           string_obs_grouping_.insert(RecValue, next_rec_num_);
///           next_rec_num_++;
///           nrecs_ = next_rec_num_;
///         }
///         Records[i] = string_obs_grouping_.at(RecValue);
///       }
///     }
///   }

///   // Generate the index and recnums for this frame. We are done with FrameIndex
///   // so it can be reused here.
///   FrameIndex.clear();
///   std::set<std::size_t> UniqueRecNums;
///   for (std::size_t i = 0; i < LocSize; ++i) {
///     std::size_t RowNum = LocIndex[i];
///     std::size_t RecNum = Records[i];
///     if (dist_->isMyRecord(RecNum)) {
///       indx_.push_back(RowNum);
///       recnums_.push_back(RecNum);
///       unique_rec_nums_.insert(RecNum);
///       FrameIndex.push_back(RowNum - FrameStart);
///     }
///   }

///   nlocs_ += FrameIndex.size();
///   return FrameIndex;
/// }

// -----------------------------------------------------------------------------
/*!
 * \details This method will return true/false according to whether the
 *          observation datetime (ObsDt) is inside the DA timing window.
 *
 * \param[in] ObsDt Observation date time object
 */
bool ObsData::InsideTimingWindow(const util::DateTime & ObsDt) {
  return ((ObsDt > winbgn_) && (ObsDt <= winend_));
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
