/*
 * (C) Copyright 2017-2019 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#include "oops/util/abor1_cpp.h"

#include "ioda/core/IodaUtils.h"
#include "ioda/io/ObsFrame.h"
#include "ioda/Variables/Variable.h"

namespace ioda {

//--------------------------- public functions ---------------------------------------
//------------------------------------------------------------------------------------
ObsFrame::ObsFrame(const ObsIoActions action, const ObsIoModes mode,
                   const ObsSpaceParameters & params) :
                       action_(action), mode_(mode), params_(params) {
    oops::Log::trace() << "Constructing ObsFrame" << std::endl;
}

ObsFrame::~ObsFrame() {}

//------------------------------------------------------------------------------------
Dimensions_t ObsFrame::frameCount(const Variable & var) {
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

}  // namespace ioda
