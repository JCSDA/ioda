/*
 * (C) Copyright 2017-2019 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#include "oops/util/abor1_cpp.h"

#include "ioda/core/IodaUtils.h"
#include "ioda/io/ObsFrameRead.h"
#include "ioda/io/ObsIoFactory.h"

namespace ioda {

//--------------------------- public functions ---------------------------------------
//------------------------------------------------------------------------------------
ObsFrameRead::ObsFrameRead(const ObsIoActions action, const ObsIoModes mode,
                           const ObsSpaceParameters & params) :
                               ObsFrame(action, mode, params) {
    // Create the ObsIo object
    if (params.in_type() == ObsIoTypes::OBS_FILE) {
        obs_io_ = ObsIoFactory::create(ObsIoActions::OPEN_FILE, ObsIoModes::READ_ONLY, params);
        max_frame_size_ = params.top_level_.obsInFile.value()->maxFrameSize;
    } else if ((params.in_type() == ObsIoTypes::GENERATOR_RANDOM) ||
               (params.in_type() == ObsIoTypes::GENERATOR_LIST)) {
        obs_io_ = ObsIoFactory::create(ObsIoActions::CREATE_GENERATOR,
                                       ObsIoModes::READ_ONLY, params);
        max_frame_size_ = params.top_level_.obsGenerate.value()->maxFrameSize;
    }
}

ObsFrameRead::~ObsFrameRead() {}

//------------------------------------------------------------------------------------
void ObsFrameRead::frameInit() {
    frame_start_ = 0;
    max_var_size_ = obs_io_->maxVarSize();
    nlocs_ = frameCount(obs_io_->vars().open("nlocs"));
    adjusted_nlocs_frame_start_ = 0;
}

//------------------------------------------------------------------------------------
void ObsFrameRead::frameNext() {
    frame_start_ += max_frame_size_;
    nlocs_ += frameCount(obs_io_->vars().open("nlocs"));
    adjusted_nlocs_frame_start_ += adjusted_nlocs_frame_count_;
}

//------------------------------------------------------------------------------------
bool ObsFrameRead::frameAvailable() {
    return (frame_start_ < max_var_size_);
}

//------------------------------------------------------------------------------------
Dimensions_t ObsFrameRead::frameStart() {
    return frame_start_;
}

//------------------------------------------------------------------------------------
Dimensions_t ObsFrameRead::frameCount(const Variable & var) {
    Dimensions_t  fCount;
    if (var.isDimensionScaleAttached(0, obs_io_->vars().open("nlocs"))) {
        fCount = adjusted_nlocs_frame_count_;
    } else {
        fCount = frameCount(var);
    }
    return fCount;
}

//--------------------------- private functions --------------------------------------
//-----------------------------------------------------------------------------------
void ObsFrameRead::print(std::ostream & os) const {
    os << "ObsFrameRead: " << std::endl;
}

// -----------------------------------------------------------------------------
void ObsFrameRead::genFrameIndexRecNums(std::shared_ptr<Distribution> & dist) {
    // Generate location indices relative to the obs source (locIndex) and relative
    // to the current frame (frameIndex).
    std::vector<Dimensions_t> locIndex;
    std::vector<Dimensions_t> frameIndex;

    // Apply the timing window. Need to filter out locations that are outside the
    // timing window before generating record numbers. This is because
    // we are generating record numbers on the fly since we want to get to the point where
    // we can do the MPI distribution without knowing how many obs (and records) we are going
    // to encounter. Only apply the timing window in the case of reading from a file.
    if (params_.in_type() == ObsIoTypes::OBS_FILE) {
        genFrameLocationsTimeWindow(locIndex, frameIndex);
    } else {
        genFrameLocationsAll(locIndex, frameIndex);
    }

    // Generate record numbers for this frame. Consider obs grouping.
    std::vector<Dimensions_t> records;
    std::string obsGroupVarName =
        params_.top_level_.obsInFile.value()->obsGrouping.value().obsGroupVar;
    if ((params_.in_type() != ObsIoTypes::OBS_FILE) || (obsGroupVarName.empty())) {
         genRecordNumbersAll(locIndex, records);
    } else {
        genRecordNumbersGrouping(obsGroupVarName, frameIndex, records);
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
    Variable nlocsVar = obs_io_->vars().open("nlocs");
    Dimensions_t locSize = this->frameCount(nlocsVar);

    locIndex.assign(locSize, 0);
    std::iota(locIndex.begin(), locIndex.end(), this->frameStart());

    frameIndex.assign(locSize, 0);
    std::iota(frameIndex.begin(), frameIndex.end(), 0);
}

//------------------------------------------------------------------------------------
void ObsFrameRead::genFrameLocationsTimeWindow(std::vector<Dimensions_t> & locIndex,
                                               std::vector<Dimensions_t> & frameIndex) {
    Variable nlocsVar = obs_io_->vars().open("nlocs");
    Dimensions_t locSize = this->frameCount(nlocsVar);

    // TODO(srh) prefer datetime@Metadata once fixed length strings are available
    std::string dtVarName = std::string("time@MetaData");
    if (!this->obs_io_->vars().exists(dtVarName)) {
        std::string ErrMsg =
            std::string("ERROR: ObsFrameRead::genFrameLocationsTimeWindow:: date time information ") +
            std::string("does not exist, cannot perform time window filtering");
        ABORT(ErrMsg);
    }

    Variable dtVar = this->obs_io_->vars().open(dtVarName);
    std::vector<Dimensions_t> counts(1, locSize);
    std::vector<Dimensions_t> feStarts(1, 0);
    std::vector<Dimensions_t> beStarts(1, this->frameStart());
    std::vector<float> dtValues;
    dtVar.read<float>(dtValues,
        Selection().extent(counts)
            .select({ SelectionOperator::SET, feStarts, counts }),
        Selection()
            .select({ SelectionOperator::SET, beStarts, counts }));
    dtValues.resize(locSize);

    // convert ref, offset time to datetime objects
    int refDtime;
    this->obs_io_->atts().open("date_time").read<int>(refDtime);
    std::vector<util::DateTime> dtimeVals = convertRefOffsetToDtime(refDtime, dtValues);

    // Keep all locations that fall inside the timing window
    for (std::size_t i = 0; i < locSize; ++i) {
      if (this->insideTimingWindow(dtimeVals[i])) {
        locIndex.push_back(this->frameStart() + i);
        frameIndex.push_back(i);
      }
    }
}

//------------------------------------------------------------------------------------
void ObsFrameRead::genRecordNumbersAll(const std::vector<Dimensions_t> & locIndex,
                                       std::vector<Dimensions_t> & records) {
    Dimensions_t locSize = locIndex.size();
    records.assign(locSize, 0);
    for (std::size_t i = 0; i < locSize; ++i) {
        int recValue = locIndex[i];
        if (int_obs_grouping_.find(recValue) == int_obs_grouping_.end()) {
            int_obs_grouping_.insert(
                std::pair<int, std::size_t>(recValue, next_rec_num_));
            next_rec_num_++;
        }
        records[i] = int_obs_grouping_.at(recValue);
    }
}

//------------------------------------------------------------------------------------
void ObsFrameRead::genRecordNumbersGrouping(const std::string & obsGroupVarName,
                                            const std::vector<Dimensions_t> & frameIndex,
                                     std::vector<Dimensions_t> & records) {
    // Group according to the data in obs_group_variable_
    std::string varName = obsGroupVarName + std::string("@MetaData");
    Variable groupVar = this->obs_io_->vars().open(varName);
    Variable nlocsVar = this->obs_io_->vars().open("nlocs");
    if (!groupVar.isDimensionScaleAttached(0, nlocsVar)) {
        std::string ErrMsg =
            std::string("ERROR: ObsFrameRead::genRecordNumbersGrouping: obs grouping variable (") +
            obsGroupVarName + std::string(") must have 'nlocs' as first dimension");
    }

    Dimensions_t frameStart = this->frameStart();
    Dimensions_t frameCount = this->frameCount(nlocsVar);

    std::vector<Dimensions_t> counts(1, frameCount);
    std::vector<Dimensions_t> feStarts(1, 0);
    std::vector<Dimensions_t> beStarts(1, frameStart);

    Dimensions_t locSize = frameIndex.size();
    records.assign(locSize, 0);
    if (groupVar.isA<int>()) {
        std::vector<int> groupVarVals;
        groupVar.read<int>(groupVarVals,
            Selection().extent(counts)
                .select({ SelectionOperator::SET, feStarts, counts }),
            Selection()
                .select({ SelectionOperator::SET, beStarts, counts }));
        groupVarVals.resize(frameCount);
        for (std::size_t i = 0; i < locSize; ++i) {
            int recValue = groupVarVals[frameIndex[i]];
            if (int_obs_grouping_.find(recValue) == int_obs_grouping_.end()) {
                int_obs_grouping_.insert(
                    std::pair<int, std::size_t>(recValue, next_rec_num_));
                next_rec_num_++;
            }
            records[i] = int_obs_grouping_.at(recValue);
        }
    } else if (groupVar.isA<float>()) {
        std::vector<float> groupVarVals;
        groupVar.read<float>(groupVarVals,
            Selection().extent(counts)
                .select({ SelectionOperator::SET, feStarts, counts }),
            Selection()
                .select({ SelectionOperator::SET, beStarts, counts }));
        groupVarVals.resize(frameCount);
        for (std::size_t i = 0; i < locSize; ++i) {
            float recValue = groupVarVals[frameIndex[i]];
            if (float_obs_grouping_.find(recValue) == float_obs_grouping_.end()) {
                float_obs_grouping_.insert(
                    std::pair<float, std::size_t>(recValue, next_rec_num_));
                next_rec_num_++;
            }
            records[i] = float_obs_grouping_.at(recValue);
        }
    } else if (groupVar.isA<std::string>()) {
        std::vector<std::string> groupVarVals;
        groupVar.read<std::string>(groupVarVals,
            Selection().extent(counts)
                .select({ SelectionOperator::SET, feStarts, counts }),
            Selection()
                .select({ SelectionOperator::SET, beStarts, counts }));
        groupVarVals.resize(frameCount);
        for (std::size_t i = 0; i < locSize; ++i) {
            std::string recValue = groupVarVals[frameIndex[i]];
            if (string_obs_grouping_.find(recValue) == string_obs_grouping_.end()) {
                string_obs_grouping_.insert(
                    std::pair<std::string, std::size_t>(recValue, next_rec_num_));
                next_rec_num_++;
            }
            records[i] = string_obs_grouping_.at(recValue);
        }
    }
}

//------------------------------------------------------------------------------------
void ObsFrameRead::applyMpiDistribution(const std::shared_ptr<Distribution> & dist,
                                        const std::vector<Dimensions_t> & locIndex,
                                        const std::vector<Dimensions_t> & records) {
    // Generate the index and recnums for this frame.
    Dimensions_t locSize = locIndex.size();
    frame_loc_index_.clear();
    for (std::size_t i = 0; i < locSize; ++i) {
        std::size_t rowNum = locIndex[i];
        std::size_t recNum = records[i];
        if (dist->isMyRecord(recNum)) {
            indx_.push_back(rowNum);
            recnums_.push_back(recNum);
            unique_rec_nums_.insert(recNum);
            frame_loc_index_.push_back(rowNum);
        }
    }
}

// -----------------------------------------------------------------------------
bool ObsFrameRead::insideTimingWindow(const util::DateTime & obsDt) {
    return ((obsDt > params_.windowStart()) && (obsDt <= params_.windowEnd()));
}

}  // namespace ioda
