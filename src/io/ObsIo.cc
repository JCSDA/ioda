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
ObsIo::ObsIo(const ObsIoActions action, const ObsIoModes mode,
             const ObsSpaceParameters & params) :
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
