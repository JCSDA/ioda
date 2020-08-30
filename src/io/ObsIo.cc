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
Dimensions_t ObsIo::varSize0(const std::string & varName) {
    auto ivar = var_info_.find(varName);
    if (ivar == var_info_.end()) {
        std::string ErrMsg = "ObsIo::varSize: Variable does not exist: " + varName;
        ABORT(ErrMsg);
    }
    return ivar->second.size0_;
}

//------------------------------------------------------------------------------------
std::type_index ObsIo::varDtype(const std::string & varName) {
    auto ivar = var_info_.find(varName);
    if (ivar == var_info_.end()) {
        std::string ErrMsg = "ObsIo::varDtype: Variable does not exist: " + varName;
        ABORT(ErrMsg);
    }
    return ivar->second.dtype_;
}

//------------------------------------------------------------------------------------
bool ObsIo::varIsDist(const std::string & varName) {
    auto ivar = var_info_.find(varName);
    if (ivar == var_info_.end()) {
        std::string ErrMsg = "ObsIo::varIsDist: Variable does not exist: " + varName;
        ABORT(ErrMsg);
    }
    return ivar->second.is_dist_;
}

//------------------------------------------------------------------------------------
std::vector<std::string> ObsIo::listDimVars() {
    std::vector<std::string> varList;
    for (auto ivar = dim_var_info_.begin(); ivar != dim_var_info_.end(); ++ivar) {
        varList.push_back(ivar->first);
    }
    return varList;
}

//------------------------------------------------------------------------------------
std::vector<std::string> ObsIo::listVars() {
    std::vector<std::string> varList;
    for (auto ivar = var_info_.begin(); ivar != var_info_.end(); ++ivar) {
        varList.push_back(ivar->first);
    }
    return varList;
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
    Dimensions_t vsize = varSize0(varName);
    if ((frame_start_ + max_frame_size_) > vsize) {
        count = vsize - frame_start_;
        if (count < 0) { count = 0; }
    } else {
        count = max_frame_size_;
    }
    return count;
}

//------------------------ protected functions ---------------------------------------
//------------------------------------------------------------------------------------
Dimensions_t ObsIo::getVarSize0(const std::string & varName) {
    Dimensions varDims = obs_group_.vars.open(varName).getDimensions();
    return varDims.dimsCur[0];
}

//------------------------------------------------------------------------------------
std::type_index ObsIo::getVarDtype(const std::string & varName) {
    Variable var = obs_group_.vars.open(varName);
    std::type_index varType(typeid(std::string));
    if (var.isA<int>()) {
        varType = typeid(int);
    } else if (var.isA<float>()) {
        varType = typeid(float);
    }
    return varType;
}

//------------------------------------------------------------------------------------
bool ObsIo::getVarIsDist(const std::string & varName) {
    bool isDist;
    Variable var = obs_group_.vars.open(varName);
    Variable nlocsVar = obs_group_.vars.open("nlocs");
    if (var.isDimensionScale()) {
        isDist = false;
    } else {
        isDist = var.isDimensionScaleAttached(0, nlocsVar);
    }
    return isDist;
}

//------------------------------------------------------------------------------------
bool ObsIo::getVarIsDimScale(const std::string & varName) {
    Variable var = obs_group_.vars.open(varName);
    return var.isDimensionScale();
}

//------------------------------------------------------------------------------------
Dimensions_t ObsIo::getVarSizeMax() {
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
