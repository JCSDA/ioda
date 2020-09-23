/*
 * (C) Copyright 2017-2019 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#include "oops/util/abor1_cpp.h"

#include "ioda/io/ObsFrameRead.h"

namespace ioda {

//--------------------------- public functions ---------------------------------------
//------------------------------------------------------------------------------------
ObsFrameRead::ObsFrameRead(const ObsIoActions action, const ObsIoModes mode,
                           const ObsSpaceParameters & params) :
                               ObsFrame(action, mode, params) {
}

ObsFrameRead::~ObsFrameRead() {}


//--------------------------- private functions --------------------------------------
}  // namespace ioda
