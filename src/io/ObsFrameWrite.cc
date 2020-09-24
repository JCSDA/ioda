/*
 * (C) Copyright 2017-2019 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#include "oops/util/abor1_cpp.h"

#include "ioda/io/ObsFrameWrite.h"
#include "ioda/io/ObsIoFactory.h"

namespace ioda {

//--------------------------- public functions ---------------------------------------
//------------------------------------------------------------------------------------
ObsFrameWrite::ObsFrameWrite(const ObsIoActions action, const ObsIoModes mode,
                             const ObsSpaceParameters & params) :
                                 ObsFrame(action, mode, params) {
    // Create the ObsIo object
    obs_io_ = ObsIoFactory::create(ObsIoActions::CREATE_FILE, ObsIoModes::CLOBBER, params);
    max_frame_size_ = params.top_level_.obsOutFile.value()->maxFrameSize;
}

ObsFrameWrite::~ObsFrameWrite() {}

//------------------------------------------------------------------------------------
void ObsFrameWrite::frameInit() {
    frame_start_ = 0;
    max_var_size_ = obs_io_->maxVarSize();
}

//------------------------------------------------------------------------------------
void ObsFrameWrite::frameNext() {
    frame_start_ += max_frame_size_;
}

//------------------------------------------------------------------------------------
bool ObsFrameWrite::frameAvailable() {
    return (frame_start_ < max_var_size_);
}

//------------------------------------------------------------------------------------
Dimensions_t ObsFrameWrite::frameStart() {
    return frame_start_;
}

//------------------------------------------------------------------------------------
Dimensions_t ObsFrameWrite::frameCount(const Variable & var) {
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
void ObsFrameWrite::createFrameSelection(const Variable & var, Selection & feSelect,
                                         Selection & beSelect) {
    // Form the selection objects for this frame. The frontend selection will
    // simply be the current size (according to the variable) of the frame going from
    // 0 to size-1.
    //
    // The backend selection will start at the frame start value (instead of zero) and
    // be determined by the size of the frame for the given variable.

    // Grab the variable dimensions and use this as a template for the selection operators.
    std::vector<Dimensions_t> varDims = var.getDimensions().dimsCur;
    Dimensions_t frameStart = this->frameStart();
    Dimensions_t frameCount = this->frameCount(var);

    // Substitute the frameCount for the first dimension size of the variable.
    varDims[0] = frameCount;

    // For the frontend, treat the data as a vector with a total size given by the product
    // of the dimension sizes considering the possible adjustment of the fisrt
    // dimension (varDims). Use hyperslab style selection since we will be consolidating
    // the selected locations into a contiguous series.
    Dimensions_t numElements = std::accumulate(
        varDims.begin(), varDims.end(), 1, std::multiplies<Dimensions_t>());
    std::vector<Dimensions_t> feStarts(1, 0);
    std::vector<Dimensions_t> feCounts(1, numElements);
    feSelect.extent(feCounts).select({ SelectionOperator::SET, feStarts, feCounts });

    // For the backend, use a hyperslab style.
    std::vector<Dimensions_t> beStarts(varDims.size(), 0);
    beStarts[0] = frameStart;
    std::vector<Dimensions_t> beCounts = varDims;
    beSelect.select({ SelectionOperator::SET, beStarts, beCounts });
}

//--------------------------- private functions --------------------------------------
//-----------------------------------------------------------------------------------
void ObsFrameWrite::print(std::ostream & os) const {
    os << "ObsFrameWrite: " << std::endl;
}

}  // namespace ioda
