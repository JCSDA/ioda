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

//--------------------------- public functions ---------------------------------------
//------------------------------------------------------------------------------------
ObsIo::ObsIo(const ObsIoActions action, const ObsIoModes mode, const ObsIoParameters & params) :
                 action_(action), mode_(mode), params_(params) {
    oops::Log::trace() << "Constructing ObsIo" << std::endl;
}

ObsIo::~ObsIo() {}

//------------------------------------------------------------------------------------
std::size_t ObsIo::numVars() {
    return var_info_.size();
}

//------------------------------------------------------------------------------------
void ObsIo::insertVarInfo(const std::string & varName, const Dimensions_t varSize0,
                          const std::type_index & varDtype, const bool varIsDist) { 
    var_info_.insert(std::make_pair(varName, VarInfoRec(varSize0, varDtype, varIsDist)));
}

//------------------------------------------------------------------------------------
void ObsIo::insertDimVarInfo(const std::string & varName, const Dimensions_t varSize0,
                             const std::type_index & varDtype, const bool varIsDist) { 
    dim_var_info_.insert(std::make_pair(varName, VarInfoRec(varSize0, varDtype, varIsDist)));
}

//------------------------------------------------------------------------------------
void ObsIo::frameInit() {
    frame_start_ = 0;
}

//------------------------------------------------------------------------------------
void ObsIo::frameNext() {
    frame_start_ += max_frame_size_;
}

//------------------------------------------------------------------------------------
bool ObsIo::frameAvailable() {
    return (frame_start_ < max_var_size_);
}

//------------------------------------------------------------------------------------
int ObsIo::frameStart() {
    return frame_start_;
}

//------------------------------------------------------------------------------------
int ObsIo::frameCount(const std::string & varName) {
    int count;
    Dimensions_t vsize = varSize0(obs_group_, varName);
    if ((frame_start_ + max_frame_size_) > vsize) {
        count = vsize - frame_start_;
        if (count < 0) { count = 0; }
    } else {
        count = max_frame_size_;
    }
    return count;
}

//------------------------------------------------------------------------------------
void ObsIo::createFrameSelection(const std::string & varName, Selection & feSelect,
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
    Variable var = obs_group_.vars.open(varName);
    std::vector<Dimensions_t> varShape = var.getDimensions().dimsCur;

    std::vector<Dimensions_t> beStarts(varShape.size(), 0);
    beStarts[0] = frameStart();
    std::vector<Dimensions_t> beCounts = varShape;
    beCounts[0] = frameCount(varName);

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

//------------------------ protected functions ---------------------------------------
//------------------------------------------------------------------------------------
Dimensions_t ObsIo::varSize0Max() {
    Dimensions_t varSizeMax = 0;
    for (auto ivar = var_info_.begin(); ivar != var_info_.end(); ++ivar) {
        if (varSizeMax < ivar->second.size0_) {
            varSizeMax = ivar->second.size0_;
        }
    }
    return varSizeMax;
}

//------------------------------------------------------------------------------------
void ObsIo::print(std::ostream & os) const {}

}  // namespace ioda
