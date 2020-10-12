/*
 * (C) Copyright 2017-2019 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#include "oops/util/abor1_cpp.h"

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
bool ObsIo::isVarDimByNlocs(const std::string varName) {
    bool isDimByNlocs = false;
    auto ivar = dims_attached_to_vars_.find(varName);
    if (ivar != dims_attached_to_vars_.end()) {
        if (ivar->second[0] == "nlocs") {
            isDimByNlocs = true;
        }
    }
    return isDimByNlocs;
}

//------------------------------------------------------------------------------------
void ObsIo::resetVarLists() {
    getVarLists(obs_group_, var_list_, dim_var_list_);
}

//------------------------------------------------------------------------------------
void ObsIo::resetVarDimMap() {
    dims_attached_to_vars_ = genDimsAttachedToVars(obs_group_.vars, var_list_, dim_var_list_);
}

//------------------------ protected functions ---------------------------------------
void ObsIo::print(std::ostream & os) const {}

}  // namespace ioda
