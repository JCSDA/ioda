/*
 * (C) Copyright 2017-2019 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#include "oops/util/abor1_cpp.h"

#include "ioda/core/IodaUtils.h"
#include "ioda/io/ObsIo.h"
#include "ioda/Variables/Variable.h"

////////////////////////////////////////////////////////////////////////
// Implementation of ObsIo base class
////////////////////////////////////////////////////////////////////////

namespace ioda {

//------------------------------------------------------------------------------------
// ObsFrame functions
//------------------------------------------------------------------------------------

//--------------------------- public functions ---------------------------------------
//------------------------------------------------------------------------------------
void ObsFrame::frameInit(const Dimensions_t maxVarSize, const Dimensions_t maxFrameSize) {
    start_ = 0;
    max_var_size_ = maxVarSize;
    max_size_ = maxFrameSize;
}

//------------------------------------------------------------------------------------
void ObsFrame::frameNext() {
    start_ += max_size_;
}

//------------------------------------------------------------------------------------
bool ObsFrame::frameAvailable() {
    return (start_ < max_var_size_);
}

//------------------------------------------------------------------------------------
Dimensions_t ObsFrame::frameStart() {
    return start_;
}

//------------------------------------------------------------------------------------
Dimensions_t ObsFrame::frameCount(const Variable & var) {
    Dimensions_t count;
    Dimensions_t varSize0 = var.getDimensions().dimsCur[0];
    if ((start_ + max_size_) > varSize0) {
        count = varSize0 - start_;
        if (count < 0) { count = 0; }
    } else {
        count = max_size_;
    }
    return count;
}

//------------------------------------------------------------------------------------
void ObsFrame::createFrameSelection(const Variable & var, Selection & feSelect,
                                    Selection & beSelect) {
    // Form the hyperslab style selection for this frame. The frontend selection will
    // simply be the current size (according to the variable) of the frame going from
    // 0 to size-1. The backend selection will be the same except starting at the frame
    // start instead of zero. The argument var is the Variable associated with the backend.
    //
    // Create the selection object for the backend first. Grab the variable shape
    // (vector of dimension sizes). Make the backend starts and counts for the
    // selection objects match the number of dimensions of the variable, but limit the
    // count for the first dimension to the associate frame count for the variable.
    std::vector<Dimensions_t> varShape = var.getDimensions().dimsCur;

    std::vector<Dimensions_t> beStarts(varShape.size(), 0);
    beStarts[0] = frameStart();
    std::vector<Dimensions_t> beCounts = varShape;
    beCounts[0] = frameCount(var);

    // Get the number of elements for the frontend selection after the counts for the backend
    // selection have been adjusted for the frame size.
    Dimensions_t numElements = std::accumulate(
        beCounts.begin(), beCounts.end(), 1, std::multiplies<Dimensions_t>());
    std::vector<Dimensions_t> feStarts(1, 0);
    std::vector<Dimensions_t> feCounts(1, numElements);

    // Create the selection objects
    feSelect.extent(feCounts)
        .select({ SelectionOperator::SET, feStarts, feCounts });
    beSelect.select({ SelectionOperator::SET, beStarts, beCounts });
}

//------------------------------------------------------------------------------------
// ObsIo functions
//------------------------------------------------------------------------------------

//--------------------------- public functions ---------------------------------------
//------------------------------------------------------------------------------------
ObsIo::ObsIo(const ObsIoActions action, const ObsIoModes mode, const ObsIoParameters & params) :
                 action_(action), mode_(mode), params_(params), obs_frame_() {
    oops::Log::trace() << "Constructing ObsIo" << std::endl;
}

ObsIo::~ObsIo() {}

//------------------------------------------------------------------------------------
void ObsIo::frameInit() {
    obs_frame_.frameInit(max_var_size_, max_frame_size_);
    nlocs_ = obs_frame_.frameCount(obs_group_.vars.open("nlocs"));
}

//------------------------------------------------------------------------------------
void ObsIo::frameNext() {
    obs_frame_.frameNext();
    nlocs_ += obs_frame_.frameCount(obs_group_.vars.open("nlocs"));
}

//------------------------------------------------------------------------------------
bool ObsIo::frameAvailable() {
    return obs_frame_.frameAvailable();
}

//------------------------------------------------------------------------------------
Dimensions_t ObsIo::frameStart() {
    return obs_frame_.frameStart();
}

//------------------------------------------------------------------------------------
Dimensions_t ObsIo::frameCount(const Variable & var) {
    return obs_frame_.frameCount(var);
}

//------------------------------------------------------------------------------------
void ObsIo::createFrameSelection(const Variable & var, Selection & feSelect,
                                 Selection & beSelect) {
    obs_frame_.createFrameSelection(var, feSelect, beSelect);
}

//------------------------ protected functions ---------------------------------------
//------------------------------------------------------------------------------------
void ObsIo::print(std::ostream & os) const {}

}  // namespace ioda
