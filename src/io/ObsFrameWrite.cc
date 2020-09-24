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

//--------------------------- private functions --------------------------------------
//-----------------------------------------------------------------------------------
void ObsFrameWrite::print(std::ostream & os) const {
    os << "ObsFrameWrite: " << std::endl;
}

}  // namespace ioda
