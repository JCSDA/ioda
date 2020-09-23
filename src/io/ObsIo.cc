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
                 action_(action), mode_(mode), params_(params) {
    oops::Log::trace() << "Constructing ObsIo" << std::endl;
}

//------------------------------------------------------------------------------------
ObsIo::~ObsIo() {}

//------------------------------------------------------------------------------------
Variable ObsIo::openVar(const std::string & varName) const {
    return obs_group_.vars.open(varName);
}

//------------------------------------------------------------------------------------
void ObsIo::resetVarList() {
    var_list_ = listVars(obs_group_);
}

//------------------------------------------------------------------------------------
void ObsIo::resetDimVarList() {
    dim_var_list_ = listDimVars(obs_group_);
}

//------------------------ protected functions ---------------------------------------
void ObsIo::print(std::ostream & os) const {}

}  // namespace ioda
