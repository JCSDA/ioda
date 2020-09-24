/*
 * (C) Copyright 2017-2019 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#include "oops/util/abor1_cpp.h"

#include "ioda/io/ObsFrameRead.h"
#include "ioda/io/ObsIoFactory.h"

namespace ioda {

//--------------------------- public functions ---------------------------------------
//------------------------------------------------------------------------------------
ObsFrameRead::ObsFrameRead(const ObsIoActions action, const ObsIoModes mode,
                           const ObsSpaceParameters & params) :
                               ObsFrame(action, mode, params) {
    // Create the ObsIo object
    if (params.in_type() == ObsIoTypes::OBS_FILE) {
        obs_io_ = ObsIoFactory::create(ObsIoActions::OPEN_FILE, ObsIoModes::READ_ONLY, params);
    } else if ((params.in_type() == ObsIoTypes::GENERATOR_RANDOM) ||
               (params.in_type() == ObsIoTypes::GENERATOR_LIST)) {
        obs_io_ = ObsIoFactory::create(ObsIoActions::CREATE_GENERATOR,
                                       ObsIoModes::READ_ONLY, params);
    }
}

ObsFrameRead::~ObsFrameRead() {}


//--------------------------- private functions --------------------------------------
}  // namespace ioda
