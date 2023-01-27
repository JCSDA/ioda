/*
 * (C) Copyright 2017-2019 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#include <algorithm>
#include <cmath>

#include "oops/util/Logger.h"

#include "ioda/distribution/DistributionFactory.h"
#include "ioda/Exception.h"
#include "ioda/Copying.h"
#include "ioda/io/ObsFrameRead.h"
#include "ioda/Variables/VarUtils.h"

namespace ioda {

namespace detail {
  using std::to_string;

  /// An overload of to_string() taking a string and returning the same string.
  std::string to_string(std::string s) {
    return s;
  }
}  // namespace detail

//--------------------------- public functions ---------------------------------------
//------------------------------------------------------------------------------------
ObsFrameRead::ObsFrameRead(const ObsSpaceParameters & params) :
    ObsFrame(params) {
    // Create the backend engine object. Use the "simulated variables" spec from
    // the YAML (params.top_level_.simVars) since that is the required spec, thus
    // the only list guaranteed to be available at this time (ie, before reading
    // the obs input and constructing the ObsSpace).
    Engines::ReaderCreationParameters createParams(
        params.windowStart(), params.windowEnd(),
        params.comm(), params.timeComm(),
        params.top_level_.simVars.value().variables(), false);
    obs_data_in_ = Engines::ReaderFactory::create(
        params.top_level_.obsDataIn.value().engine.value().engineParameters, createParams);

    ObsGroup og = obs_data_in_->getObsGroup();

    // record number of locations from backend
    backend_nlocs_ = og.vars.open("Location").getDimensions().dimsCur[0];
    if (backend_nlocs_ == 0) {
      oops::Log::warning() << "WARNING: Input file " << obs_data_in_->fileName()
                           << " contains zero observations" << std::endl;
    }

    // Find out what datetime representation exists in the input
    // Precedence is epoch first, then string, then offset.
    use_epoch_datetime_ = og.vars.exists("MetaData/dateTime");
    use_string_datetime_ = og.vars.exists("MetaData/datetime");
    use_offset_datetime_ = og.vars.exists("MetaData/time");
    if (use_epoch_datetime_) {
      use_string_datetime_ = false;
      use_offset_datetime_ = false;
    } else if (use_string_datetime_) {
      use_offset_datetime_ = false;
    }

    // Check to see if required metadata variables exist
    bool haveRequiredMetadata =
        use_epoch_datetime_ || use_string_datetime_ || use_offset_datetime_;
    haveRequiredMetadata = haveRequiredMetadata && og.vars.exists("MetaData/latitude");
    haveRequiredMetadata = haveRequiredMetadata && og.vars.exists("MetaData/longitude");

    // Only do this check if there are more than zero obs in the file (backend_nlocs_ > 0)
    // When a file does contain zero obs, we want to allow for an "empty" file with
    // no variables. This makes it easier for r2d2 to provide a valid "empty" file when there
    // are no obs available.
    if ((backend_nlocs_ > 0) && (!haveRequiredMetadata)) {
      std::string errorMsg =
          std::string("\nOne or more of the following metadata variables are missing ") +
          std::string("from the input obs data source:\n") +
          std::string("    MetaData/dateTime (preferred) or MetaData/datetime or MetaData/time\n") +
          std::string("    MetaData/latitude\n") +
          std::string("    MetaData/longitude\n");
      throw Exception(errorMsg.c_str(), ioda_Here());
    }

    if (use_string_datetime_) {
      oops::Log::info() << "WARNING: string style datetime will cause performance degredation "
                        << "and will eventually be deprecated." << std::endl
                        << "WARNING: Please update your datetime data to the epoch style "
                        << "representation using the new variable: MetaData/dateTime."
                        << std::endl;
    }

    if (use_offset_datetime_) {
      oops::Log::info() << "WARNING: the reference/offset style datetime will be deprecated soon."
                        << std::endl
                        << "WARNING: Please update your datetime data to the epoch style "
                        << "representation using the new variable: MetaData/dateTime."
                        << std::endl;
    }

    // Collect information from the backend which will help with frame initialization
    // and frame looping. Note the call to collectVarDimInfo will cache variable
    // and dimension information from the backend since doing these on the fly is
    // very slow with the HDF5 backend.
    VarUtils::collectVarDimInfo(og, backend_var_list_, backend_dim_var_list_,
                                backend_dims_attached_to_vars_, backend_max_var_size_);

    // record variables by which observations should be grouped into records
    obs_grouping_vars_ = params.top_level_.obsDataIn.value().obsGrouping.value().obsGroupVars;

    // Create an MPI distribution
    const auto & distParams = params.top_level_.distribution.value().params.value();
    distname_ = distParams.name;
    dist_ = DistributionFactory::create(params.comm(), distParams);

    max_frame_size_ = params.top_level_.obsDataIn.value().maxFrameSize;
    oops::Log::debug() << "ObsFrameRead: maximum frame size: " << max_frame_size_ << std::endl;
}

ObsFrameRead::~ObsFrameRead() {}

//------------------------------------------------------------------------------------
void ObsFrameRead::frameInit(Has_Attributes & destAttrs) {
    // reset counters, etc.
    frame_start_ = 0;
    next_rec_num_ = 0;
    rec_num_increment_ = 1;
    unique_rec_nums_.clear();
    // It's important to grab maximum var size from the backend since it is being used to
    // determine when there are no more frames from the backend.
    max_var_size_ = backend_max_var_size_;
    nlocs_ = 0;
    adjusted_location_frame_start_ = 0;
    gnlocs_ = 0;
    nrecs_ = 0;

    // create an ObsGroup based frame with an in-memory backend
    createFrameFromObsGroup(backend_var_list_, backend_dim_var_list_,
                            backend_dims_attached_to_vars_);

    // copy the global attributes
    copyAttributes(obs_data_in_->getObsGroup().atts, destAttrs);

    // Collect variable and dimension information for downstream use. Don't use the
    // max_var_size_ from obs_frame_ since it is artificially cropped to the max_frame_size_.
    // max_var_size_ is being used to determine when there are no more frames left from
    // the backend so it needs to be set from the backend.
    Dimensions_t dummyMaxVarSize;
    VarUtils::collectVarDimInfo(obs_frame_, var_list_, dim_var_list_,
                                dims_attached_to_vars_, dummyMaxVarSize);
}

//------------------------------------------------------------------------------------
void ObsFrameRead::frameNext() {
    frame_start_ += max_frame_size_;
    adjusted_location_frame_start_ += adjusted_location_frame_count_;
}

//------------------------------------------------------------------------------------
bool ObsFrameRead::frameAvailable() {
    bool haveAnotherFrame = (frame_start_ < max_var_size_);
    // If there is another frame, then read it into obs_frame_
    if (haveAnotherFrame) {
        // Resize along the Location dimension
        Variable LocationVar = obs_frame_.vars.open("Location");
        obs_frame_.resize(
            { std::pair<Variable, Dimensions_t>(LocationVar, frameCount("Location")) });

        // Transfer all variable data
        Dimensions_t frameStart = this->frameStart();
        for (auto & varNameObject : backend_var_list_) {
            std::string varName = varNameObject.name;
            Variable sourceVar = varNameObject.var;
            Dimensions_t frameCount = this->basicFrameCount(sourceVar);
            if (frameCount > 0) {
                // Transfer the variable data for this frame. Do this in two steps:
                //    ObsIo --> memory buffer --> frame storage

                // Selection objects for transfer;
                std::vector<Dimensions_t> varShape = sourceVar.getDimensions().dimsCur;
                Selection obsIoSelect = createObsIoSelection(varShape, frameStart, frameCount);
                Selection memBufferSelect = createMemSelection(varShape, frameCount);
                Selection obsFrameSelect = createEntireFrameSelection(varShape, frameCount);

                // Transfer the data
                Variable destVar = obs_frame_.vars.open(varName);

                VarUtils::forAnySupportedVariableType(
                      destVar,
                      [&](auto typeDiscriminator) {
                          typedef decltype(typeDiscriminator) T;
                          std::vector<T> varValues;
                          sourceVar.read<T>(varValues, memBufferSelect, obsIoSelect);
                          destVar.write<T>(varValues, memBufferSelect, obsFrameSelect);
                      },
                      VarUtils::ThrowIfVariableIsOfUnsupportedType(varName));
            }
        }

        // If using the string or offset datetimes, convert those to epoch datetimes
        if (use_string_datetime_) {
          // Read in string datetimes and convert to time offsets. Use the window
          // start time as the epoch.
          std::vector<std::string> dtStrings;
          Variable stringDtVar = obs_frame_.vars.open("MetaData/datetime");
          stringDtVar.read<std::string>(dtStrings);
          std::vector<int64_t> timeOffsets = convertDtStringsToTimeOffsets(
              params_.windowStart(), dtStrings);

          // Transfer the epoch datetime to the new variable.
          Variable epochDtVar = obs_frame_.vars.open("MetaData/dateTime");
          epochDtVar.write<int64_t>(timeOffsets);
        } else if (use_offset_datetime_) {
          // Use the date_time global attribute as the epoch. This means that
          // we just need to convert the float offset times in hours to an
          // int64_t offset in seconds.
          std::vector<float> dtTimeOffsets;
          Variable offsetDtVar = obs_frame_.vars.open("MetaData/time");
          offsetDtVar.read<float>(dtTimeOffsets);

          std::vector<int64_t> timeOffsets(dtTimeOffsets.size());
          for (std::size_t i = 0; i < dtTimeOffsets.size(); ++i) {
            timeOffsets[i] = static_cast<int64_t>(lround(dtTimeOffsets[i] * 3600.0));
          }

          // Transfer the epoch datetime to the new variable.
          Variable epochDtVar = obs_frame_.vars.open("MetaData/dateTime");
          epochDtVar.write<int64_t>(timeOffsets);
        }

        // generate the frame index and record numbers for this frame
        genFrameIndexRecNums(dist_);

        // clear the selection caches
        known_frame_selections_.clear();
        known_mem_selections_.clear();
    } else {
      // assign each record to the patch of a unique PE
      dist_->computePatchLocs();
    }
    return (haveAnotherFrame);
}

//------------------------------------------------------------------------------------
Dimensions_t ObsFrameRead::frameStart() {
    return frame_start_;
}

//------------------------------------------------------------------------------------
Dimensions_t ObsFrameRead::frameCount(const std::string & varName) {
    // We need to query the full size of the variable from the backend, but we may
    // have created MetaData/dateTime from MetaData/datetime inside the frame.
    // If we are asking for MetaData/dateTime, but we used MetaData/datetime from
    // the backend, then substitue in MetaData/datetime for this check.
    std::string useVarName = varName;
    if (use_string_datetime_ && (varName == "MetaData/dateTime")) {
        useVarName = "MetaData/datetime";
    } else if (use_offset_datetime_ && (varName == "MetaData/dateTime")) {
        useVarName = "MetaData/time";
    }
    Variable var = obs_data_in_->getObsGroup().vars.open(useVarName);
    Dimensions_t  fCount;
    if (var.isDimensionScale()) {
        fCount = basicFrameCount(var);
    } else {
        if (isVarDimByLocation_Impl(useVarName, backend_dims_attached_to_vars_)) {
            fCount = adjusted_location_frame_count_;
        } else {
            fCount = basicFrameCount(var);
        }
    }
    return fCount;
}

//-----------------------------------------------------------------------------------
bool ObsFrameRead::readFrameVar(const std::string & varName, std::vector<int> & varData) {
    return readFrameVarHelper<int>(varName, varData);
}
bool ObsFrameRead::readFrameVar(const std::string & varName, std::vector<int64_t> & varData) {
    return readFrameVarHelper<int64_t>(varName, varData);
}
bool ObsFrameRead::readFrameVar(const std::string & varName, std::vector<float> & varData) {
    return readFrameVarHelper<float>(varName, varData);
}
bool ObsFrameRead::readFrameVar(const std::string & varName,
                                std::vector<std::string> & varData) {
    return readFrameVarHelper<std::string>(varName, varData);
}
bool ObsFrameRead::readFrameVar(const std::string & varName, std::vector<char> & varData) {
    return readFrameVarHelper<char>(varName, varData);
}

//--------------------------- private functions --------------------------------------
//-----------------------------------------------------------------------------------
void ObsFrameRead::print(std::ostream & os) const {
    os << "ObsFrameRead: " << std::endl;
}

//------------------------------------------------------------------------------------
Dimensions_t ObsFrameRead::basicFrameCount(const Variable & var) {
    Dimensions_t count;
    Dimensions_t varSize0 = var.getDimensions().dimsCur[0];
    if ((frame_start_ + max_frame_size_) > varSize0) {
        count = varSize0 - frame_start_;
        if (count < 0) { count = 0; }
    } else {
        count = max_frame_size_;
    }
    return count;
}

//------------------------------------------------------------------------------------
Selection ObsFrameRead::createIndexedFrameSelection(const std::vector<Dimensions_t> & varShape) {
    // frame_loc_index_ contains the indices for the first dimension. Subsequent
    // dimensions, we want to select all indices.
    Selection indexedFrameSelect;
    indexedFrameSelect.extent(varShape)
        .select({ SelectionOperator::SET, 0, frame_loc_index_ });
    for (std::size_t i = 1; i < varShape.size(); ++i) {
        std::vector<Dimensions_t> dimIndex(varShape[i]);
        std::iota(dimIndex.begin(), dimIndex.end(), 0);
        indexedFrameSelect.extent(varShape).select({ SelectionOperator::AND, i, dimIndex });
    }
    return indexedFrameSelect;
}

// -----------------------------------------------------------------------------
void ObsFrameRead::genFrameIndexRecNums(std::shared_ptr<Distribution> & dist) {
    // Generate location indices relative to the obs source (locIndex) and relative
    // to the current frame (frameIndex).
    std::vector<Dimensions_t> locIndex;
    std::vector<Dimensions_t> frameIndex;

    // Apply locations checking. Need to filter out locations that are outside
    // the timing window or that contain missing values before generating record
    // numbers. This is because we are generating record numbers on the fly
    // since we want to get to the point where we can do the MPI distribution
    // without knowing how many obs (and records) we are going to encounter.
    if (obs_data_in_->applyLocationsCheck()) {
        genFrameLocationsWithQcheck(locIndex, frameIndex);
    } else {
        genFrameLocationsAll(locIndex, frameIndex);
    }

    // Generate record numbers for this frame. Consider obs grouping.
    std::vector<Dimensions_t> records;
    const std::vector<std::string> & obsGroupVarList = obs_grouping_vars_;
    if (obsGroupVarList.empty()) {
        genRecordNumbersAll(locIndex, records);
    } else {
        genRecordNumbersGrouping(obsGroupVarList, frameIndex, records);
    }

    // Apply the MPI distribution to the records
    applyMpiDistribution(dist, locIndex, records);

    // New frame count is the number of entries in the frame_loc_index_ vector
    // This will be handed to callers through the frameCount function for all
    // variables with Location as their first dimension.
    adjusted_location_frame_count_ = frame_loc_index_.size();
}

//------------------------------------------------------------------------------------
void ObsFrameRead::genFrameLocationsAll(std::vector<Dimensions_t> & locIndex,
                                        std::vector<Dimensions_t> & frameIndex) {
    Dimensions_t locSize = this->frameCount("Location");
    gnlocs_ += locSize;

    locIndex.resize(locSize);
    std::iota(locIndex.begin(), locIndex.end(), this->frameStart());

    frameIndex.resize(locSize);
    std::iota(frameIndex.begin(), frameIndex.end(), 0);
}

//------------------------------------------------------------------------------------
void ObsFrameRead::genFrameLocationsWithQcheck(std::vector<Dimensions_t> & locIndex,
                                               std::vector<Dimensions_t> & frameIndex) {
    Dimensions_t frameCount = this->frameCount("Location");
    Dimensions_t frameStart = this->frameStart();

    // Reader code will have thrown an exception before getting here if datetime information
    // is mising from the input obs source. Also the epoch style datetime values have
    // been generated by now so we can assume that the variable "MetaData/dateTime" exists
    // along with the epoch style datetime values.

    // Build the selection objects
    Variable dtVar = obs_frame_.vars.open("MetaData/dateTime");
    std::vector<Dimensions_t> varShape = dtVar.getDimensions().dimsCur;
    Selection memSelect = createMemSelection(varShape, frameCount);
    Selection frameSelect = createEntireFrameSelection(varShape, frameCount);

    // convert ref, offset time to datetime objects
    std::vector<int64_t> timeOffsets;
    dtVar.read<int64_t>(timeOffsets);
    util::DateTime epochDt = getEpochAsDtime(dtVar);
    std::vector<util::DateTime> dtimeVals = convertEpochDtToDtime(epochDt, timeOffsets);

    // Need to check the latitude and longitude values too.
    std::vector<float> lats;
    Variable latVar = obs_frame_.vars.open("MetaData/latitude");
    latVar.read<float>(lats, memSelect, frameSelect);
    detail::FillValueData_t latFvData = latVar.getFillValue();
    float latFillValue = detail::getFillValue<float>(latFvData);

    std::vector<float> lons;
    Variable lonVar = obs_frame_.vars.open("MetaData/longitude");
    lonVar.read<float>(lons, memSelect, frameSelect);
    detail::FillValueData_t lonFvData = lonVar.getFillValue();
    float lonFillValue = detail::getFillValue<float>(lonFvData);

    // Keep all locations that fall inside the timing window. Note iloc will be set
    // to the number of locations stored in the output vectors after exiting the
    // following for loop.
    locIndex.resize(frameCount);
    frameIndex.resize(frameCount);
    std::size_t iloc = 0;
    for (std::size_t i = 0; i < frameCount; ++i) {
      // Check the timing window first since having a location outside the timing
      // window likely occurs more than having issues with the lat and lon values.
      bool keepThisLocation = this->insideTimingWindow(dtimeVals[i]);
      if (!keepThisLocation) {
        // Keep a count of how many obs were rejected due to being outside
        // the timing window
        gnlocs_outside_timewindow_++;
      }

      // Check the latitude value if we made it through the timing window check.
      // Reject the obs if the latitude value is the fill value (missing data)
      if (keepThisLocation) {
        if (lats[i] == latFillValue) {
           keepThisLocation = false;
        }
      }

      // Check the longitude value if we made it through the prior checks.
      // Reject the obs if the longitude value is the fill value (missing data)
      if (keepThisLocation) {
        if (lons[i] == lonFillValue) {
           keepThisLocation = false;
        }
      }

      // Obs has passed all of the quality checks so add it to the list of records
      if (keepThisLocation) {
        locIndex[iloc] = frameStart + i;
        frameIndex[iloc] = i;
        iloc++;
      }
    }
    locIndex.resize(iloc);
    frameIndex.resize(iloc);
    gnlocs_ += iloc;
}

//------------------------------------------------------------------------------------
void ObsFrameRead::genRecordNumbersAll(const std::vector<Dimensions_t> & locIndex,
                                       std::vector<Dimensions_t> & records) {
    // No obs grouping. Assign each location to a separate record.
    Dimensions_t locSize = locIndex.size();
    records.assign(locSize, 0);
    for (std::size_t i = 0; i < locSize; ++i) {
        records[i] = next_rec_num_;
        next_rec_num_ += rec_num_increment_;
    }
}

//------------------------------------------------------------------------------------
void ObsFrameRead::genRecordNumbersGrouping(const std::vector<std::string> & obsGroupVarList,
                                            const std::vector<Dimensions_t> & frameIndex,
                                     std::vector<Dimensions_t> & records) {
    // Form the selection objects for reading the variables

    // Applying obs grouping. First convert all of the group variable data values for this
    // frame into string key values. This is done in one call to minimize accessing the
    // frame data for the grouping variables.
    std::size_t locSize = frameIndex.size();
    records.assign(locSize, 0);
    std::vector<std::string> obsGroupingKeys(locSize);
    buildObsGroupingKeys(obsGroupVarList, frameIndex, obsGroupingKeys);

    for (std::size_t i = 0; i < locSize; ++i) {
      if (obs_grouping_.find(obsGroupingKeys[i]) == obs_grouping_.end()) {
        // key is not present in the map
        // assign current record number to the current key, and move to the next record number
        obs_grouping_.insert(
            std::pair<std::string, std::size_t>(obsGroupingKeys[i], next_rec_num_));
        next_rec_num_ += rec_num_increment_;
      }
      records[i] = obs_grouping_.at(obsGroupingKeys[i]);
    }
}

//------------------------------------------------------------------------------------
void ObsFrameRead::buildObsGroupingKeys(const std::vector<std::string> & obsGroupVarList,
                                        const std::vector<Dimensions_t> & frameIndex,
                                        std::vector<std::string> & groupingKeys) {
    // Walk though each variable and construct the segments of the key values (strings)
    // Append the segments as each variable is encountered.
    for (std::size_t i = 0; i < obsGroupVarList.size(); ++i) {
        // Retrieve the variable values from the obs frame and convert
        // those values to strings. Then append those "value" strings from each
        // variable to form the grouping keys.
        std::string obsGroupVarName = obsGroupVarList[i];
        std::string varName = std::string("MetaData/") + obsGroupVarName;
        Variable groupVar = obs_frame_.vars.open(varName);
        if (!isVarDimByLocation_Impl(varName, backend_dims_attached_to_vars_)) {
            std::string ErrMsg =
                std::string("ERROR: ObsFrameRead::genRecordNumbersGrouping: ") +
                std::string("obs grouping variable (") + obsGroupVarName +
                std::string(") must have 'Location' as first dimension");
            Exception(ErrMsg.c_str(), ioda_Here());
        }

        // Form selection objects to grab the current frame values
        Dimensions_t frameCount = this->frameCount("Location");

        std::vector<Dimensions_t> varShape = groupVar.getDimensions().dimsCur;
        Selection memSelect = createMemSelection(varShape, frameCount);
        Selection frameSelect = createEntireFrameSelection(varShape, frameCount);

        VarUtils::forAnySupportedVariableType(
              groupVar,
              [&](auto typeDiscriminator) {
                  typedef decltype(typeDiscriminator) T;
                  std::vector<T> groupVarValues;
                  groupVar.read<T>(groupVarValues, memSelect, frameSelect);
                  groupVarValues.resize(frameCount);
                  std::string keySegment;
                  for (std::size_t j = 0; j < frameIndex.size(); ++j) {
                      keySegment = detail::to_string(groupVarValues[frameIndex[j]]);
                      if (i == 0) {
                          groupingKeys[j] = keySegment;
                      } else {
                          groupingKeys[j] += ":";
                          groupingKeys[j] += keySegment;
                      }
                  }
              },
              VarUtils::ThrowIfVariableIsOfUnsupportedType(varName));
    }
}

//------------------------------------------------------------------------------------
void ObsFrameRead::applyMpiDistribution(const std::shared_ptr<Distribution> & dist,
                                        const std::vector<Dimensions_t> & locIndex,
                                        const std::vector<Dimensions_t> & records) {
    // Read lat/lon for this frame
    Dimensions_t locSize = locIndex.size();
    std::vector<float> lats(locSize, 0);
    std::vector<float> lons(locSize, 0);
    Dimensions_t frameStart = this->frameStart();
    // Form selection objects to grab the current frame values
    Dimensions_t frameCount = this->frameCount("Location");

    // Assume that lat and lon variables are shaped the same
    if (!obs_frame_.vars.exists("MetaData/longitude")) {
        throw eckit::UserError("MetaData/longitude not found in observations file", Here());
    }
    Variable latLonVar = obs_frame_.vars.open("MetaData/longitude");
    std::vector<Dimensions_t> varShape = latLonVar.getDimensions().dimsCur;
    Selection memSelect = createMemSelection(varShape, frameCount);
    Selection frameSelect = createEntireFrameSelection(varShape, frameCount);

    latLonVar.read(lons, memSelect, frameSelect);
    lons.resize(frameCount);

    if (!obs_frame_.vars.exists("MetaData/latitude")) {
        throw eckit::UserError("MetaData/latitude not found in observations file", Here());
    }
    latLonVar = obs_frame_.vars.open("MetaData/latitude");
    latLonVar.read(lats, memSelect, frameSelect);
    lats.resize(frameCount);

    // Generate the index and recnums for this frame.
    const std::size_t commSize = params_.comm().size();
    const std::size_t commRank = params_.comm().rank();
    std::size_t rowNum = 0;
    std::size_t recNum = 0;
    std::size_t frameIndex = 0;
    std::size_t globalLocIndex = 0;
    frame_loc_index_.clear();
    for (std::size_t i = 0; i < locSize; ++i) {
        rowNum = locIndex[i];
        recNum = records[i];
        // The current frame storage always starts at zero so frameIndex
        // needs to be the offset from the ObsIo frame start.
        frameIndex = rowNum - frameStart;

        eckit::geometry::Point2 point(lons[frameIndex], lats[frameIndex]);

        std::size_t globalLocIndex = rowNum;
        dist_->assignRecord(recNum, globalLocIndex, point);

        if (dist->isMyRecord(recNum)) {
            indx_.push_back(globalLocIndex);
            recnums_.push_back(recNum);
            unique_rec_nums_.insert(recNum);
            frame_loc_index_.push_back(frameIndex);
            nlocs_++;
        }
    }
    nrecs_ = unique_rec_nums_.size();
}

// -----------------------------------------------------------------------------
bool ObsFrameRead::insideTimingWindow(const util::DateTime & obsDt) {
    return ((obsDt > params_.windowStart()) && (obsDt <= params_.windowEnd()));
}

}  // namespace ioda
