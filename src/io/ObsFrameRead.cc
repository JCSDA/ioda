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
#include "ioda/io/ObsIoFactory.h"

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
    // Create the ObsIo object
    obs_io_ = ObsIoFactory::create(ObsIoModes::READ, params);

    // Find out what datetime representation exists in the input
    // Precedence is epoch first, then string, then offset.
    use_epoch_datetime_ = obs_io_->vars().exists("MetaData/dateTime");
    use_string_datetime_ = obs_io_->vars().exists("MetaData/datetime");
    use_offset_datetime_ = obs_io_->vars().exists("MetaData/time");
    if (use_epoch_datetime_) {
      use_string_datetime_ = false;
      use_offset_datetime_ = false;
    } else if (use_string_datetime_) {
      use_offset_datetime_ = false;
    }

    // Check to see if required metadata variables exist
    bool haveRequiredMetadata =
        use_epoch_datetime_ || use_string_datetime_ || use_offset_datetime_;
    haveRequiredMetadata = haveRequiredMetadata && obs_io_->vars().exists("MetaData/latitude");
    haveRequiredMetadata = haveRequiredMetadata && obs_io_->vars().exists("MetaData/longitude");
    if (!haveRequiredMetadata) {
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

    const auto & distParams = params.top_level_.distribution.value().params.value();
    distname_ = distParams.name;

    // Create an MPI distribution
    each_process_reads_separate_obs_ = obs_io_->eachProcessGeneratesSeparateObs();
    if (each_process_reads_separate_obs_) {
      if ("Halo" == distname_) {
         dist_ = DistributionFactory::create(params.comm(), distParams);
      } else {
        // On each process the obs_io_ object will produce a separate series of observations,
        // so we need to use a non-overlapping distribution. The RoundRobin will do.
        dist_ = DistributionFactory::create(params.comm(), EmptyDistributionParameters());
      }
    } else {
      dist_ = DistributionFactory::create(params.comm(), distParams);
    }

    max_frame_size_ = params.top_level_.obsIoInParameters().maxFrameSize;
    oops::Log::debug() << "ObsFrameRead: maximum frame size: " << max_frame_size_ << std::endl;
}

ObsFrameRead::~ObsFrameRead() {}

//------------------------------------------------------------------------------------
void ObsFrameRead::frameInit(Has_Attributes & destAttrs) {
    // reset counters, etc.
    frame_start_ = 0;
    if (each_process_reads_separate_obs_) {
      // Ensure record numbers assigned on different processes don't overlap
      next_rec_num_ = params_.comm().rank();
      rec_num_increment_ = params_.comm().size();
    } else {
      next_rec_num_ = 0;
      rec_num_increment_ = 1;
    }
    unique_rec_nums_.clear();
    // It's important to grab maximum var size from obs_io_ since it is being used to
    // determine when there are no more frames from obs_io_.
    max_var_size_ = obs_io_->maxVarSize();
    nlocs_ = 0;
    adjusted_nlocs_frame_start_ = 0;
    gnlocs_ = 0;
    nrecs_ = 0;

    // create an ObsGroup based frame with an in-memory backend
    createFrameFromObsGroup(obs_io_->varList(), obs_io_->dimVarList(), obs_io_->varDimMap());

    // copy the global attributes
    copyAttributes(obs_io_->atts(), destAttrs);

    // Collect variable and dimension information for downstream use. Don't use the
    // max_var_size_ from obs_frame_ since it is artificially cropped to the max_frame_size_.
    // max_var_size_ is being used to determine when there are no more frames left from
    // obs_io_ so it needs to be set from obs_io_.
    Dimensions_t dummyMaxVarSize;
    VarUtils::collectVarDimInfo(obs_frame_, var_list_, dim_var_list_,
                                dims_attached_to_vars_, dummyMaxVarSize);
}

//------------------------------------------------------------------------------------
void ObsFrameRead::frameNext() {
    frame_start_ += max_frame_size_;
    adjusted_nlocs_frame_start_ += adjusted_nlocs_frame_count_;
}

//------------------------------------------------------------------------------------
bool ObsFrameRead::frameAvailable() {
    bool haveAnotherFrame = (frame_start_ < max_var_size_);
    // If there is another frame, then read it into obs_frame_
    if (haveAnotherFrame) {
        // Resize along the nlocs dimension
        Variable nlocsVar = obs_frame_.vars.open("nlocs");
        obs_frame_.resize(
            { std::pair<Variable, Dimensions_t>(nlocsVar, frameCount("nlocs")) });

        // Transfer all variable data
        Dimensions_t frameStart = this->frameStart();
        for (auto & varNameObject : obs_io_->varList()) {
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

                forAnySupportedVariableType(
                      destVar,
                      [&](auto typeDiscriminator) {
                          typedef decltype(typeDiscriminator) T;
                          std::vector<T> varValues;
                          sourceVar.read<T>(varValues, memBufferSelect, obsIoSelect);
                          destVar.write<T>(varValues, memBufferSelect, obsFrameSelect);
                      },
                      ThrowIfVariableIsOfUnsupportedType(varName));
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
      if (obs_io_->eachProcessGeneratesSeparateObs()) {
        // sum up global location counts on all PEs
        params_.comm().allReduceInPlace(gnlocs_, eckit::mpi::sum());
        params_.comm().allReduceInPlace(gnlocs_outside_timewindow_, eckit::mpi::sum());
      }
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
    // We need to query the full size of the variable from obs_io_, but we may
    // have created MetaData/dateTime from MetaData/datetime inside the frame.
    // If we are asking for MetaData/dateTime, but we used MetaData/datetime from
    // obs_io_, then substitue in MetaData/datetime for this check.
    std::string useVarName = varName;
    if (use_string_datetime_ && (varName == "MetaData/dateTime")) {
        useVarName = "MetaData/datetime";
    } else if (use_offset_datetime_ && (varName == "MetaData/dateTime")) {
        useVarName = "MetaData/time";
    }
    Variable var = obs_io_->vars().open(useVarName);
    Dimensions_t  fCount;
    if (var.isDimensionScale()) {
        fCount = basicFrameCount(var);
    } else {
        if (obs_io_->isVarDimByNlocs(useVarName)) {
            fCount = adjusted_nlocs_frame_count_;
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
    if (obs_io_->applyLocationsCheck()) {
        genFrameLocationsWithQcheck(locIndex, frameIndex);
    } else {
        genFrameLocationsAll(locIndex, frameIndex);
    }

    // Generate record numbers for this frame. Consider obs grouping.
    std::vector<Dimensions_t> records;
    const std::vector<std::string> &obsGroupVarList = obs_io_->obsGroupingVars();
    if (obsGroupVarList.empty()) {
        genRecordNumbersAll(locIndex, records);
    } else {
        genRecordNumbersGrouping(obsGroupVarList, frameIndex, records);
    }

    // Apply the MPI distribution to the records
    applyMpiDistribution(dist, locIndex, records);

    // New frame count is the number of entries in the frame_loc_index_ vector
    // This will be handed to callers through the frameCount function for all
    // variables with nlocs as their first dimension.
    adjusted_nlocs_frame_count_ = frame_loc_index_.size();
}

//------------------------------------------------------------------------------------
void ObsFrameRead::genFrameLocationsAll(std::vector<Dimensions_t> & locIndex,
                                        std::vector<Dimensions_t> & frameIndex) {
    Dimensions_t locSize = this->frameCount("nlocs");
    gnlocs_ += locSize;

    locIndex.resize(locSize);
    std::iota(locIndex.begin(), locIndex.end(), this->frameStart());

    frameIndex.resize(locSize);
    std::iota(frameIndex.begin(), frameIndex.end(), 0);
}

//------------------------------------------------------------------------------------
void ObsFrameRead::genFrameLocationsWithQcheck(std::vector<Dimensions_t> & locIndex,
                                               std::vector<Dimensions_t> & frameIndex) {
    Dimensions_t frameCount = this->frameCount("nlocs");
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
        if (!obs_io_->isVarDimByNlocs(varName)) {
            std::string ErrMsg =
                std::string("ERROR: ObsFrameRead::genRecordNumbersGrouping: ") +
                std::string("obs grouping variable (") + obsGroupVarName +
                std::string(") must have 'nlocs' as first dimension");
            Exception(ErrMsg.c_str(), ioda_Here());
        }

        // Form selection objects to grab the current frame values
        Dimensions_t frameCount = this->frameCount("nlocs");

        std::vector<Dimensions_t> varShape = groupVar.getDimensions().dimsCur;
        Selection memSelect = createMemSelection(varShape, frameCount);
        Selection frameSelect = createEntireFrameSelection(varShape, frameCount);

        forAnySupportedVariableType(
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
              ThrowIfVariableIsOfUnsupportedType(varName));
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
    Dimensions_t frameCount = this->frameCount("nlocs");

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

    /// If "save obs distribution" is set to true in a previous run,
    /// global location indices and record numbers have stored
    /// in the MetaData/saved_index and MetaData/saved_record_number variables,
    /// along with all other variables in separate files.
    ///
    /// When the "obsdatain.read obs from separate file" option is set,
    /// each process reads a separate input file generated previously,
    /// to use the stored index and record_number.

    std::vector<int> saved_index(locSize, 0);
    std::vector<int> saved_record_number(locSize, 0);

    if (each_process_reads_separate_obs_ && ("Halo" == distname_)) {
      // Assume that saved_index and saved_record_number variables are shaped the same
      if (!obs_frame_.vars.exists("MetaData/saved_record_number")) {
        throw eckit::UserError("MetaData/saved_record_number not found in observations file",
                                Here());
      }
      Variable locVar = obs_frame_.vars.open("MetaData/saved_record_number");
      varShape = locVar.getDimensions().dimsCur;
      Selection locMemSelect = createMemSelection(varShape, frameCount);
      Selection locFrameSelect = createEntireFrameSelection(varShape, frameCount);
      locVar.read(saved_record_number, locMemSelect, locFrameSelect);
      saved_record_number.resize(frameCount);

      if (!obs_frame_.vars.exists("MetaData/saved_index")) {
        throw eckit::UserError("MetaData/saved_index not found in observations file", Here());
      }
      locVar = obs_frame_.vars.open("MetaData/saved_index");
      locVar.read(saved_index, locMemSelect, locFrameSelect);
      saved_index.resize(frameCount);
    }

    // Generate the index and recnums for this frame.
    const std::size_t commSize = params_.comm().size();
    const std::size_t commRank = params_.comm().rank();
    std::size_t rowNum = 0;
    std::size_t recNum = 0;
    std::size_t frameIndex = 0;
    std::size_t globalLocIndex = 0;
    frame_loc_index_.clear();
    for (std::size_t i = 0; i < locSize; ++i) {
        if (each_process_reads_separate_obs_ && ("Halo" == distname_)) {
          rowNum = saved_index[i];
          recNum = saved_record_number[i];
          frameIndex = i;
        } else {
          rowNum = locIndex[i];
          recNum = records[i];
          // The current frame storage always starts at zero so frameIndex
          // needs to be the offset from the ObsIo frame start.
          frameIndex = rowNum - frameStart;
        }

        eckit::geometry::Point2 point(lons[frameIndex], lats[frameIndex]);

        std::size_t globalLocIndex = rowNum;
        if (each_process_reads_separate_obs_ && ("Halo" != distname_)) {
          // Each process reads a different set of observations. Make sure all of them are assigned
          // different global location indices
          globalLocIndex = rowNum * commSize + commRank;
        }
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
