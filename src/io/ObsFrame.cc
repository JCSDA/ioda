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

}  // namespace ioda
