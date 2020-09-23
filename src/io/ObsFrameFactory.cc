/*
 * (C) Copyright 2017 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#include "ioda/io/ObsFrameFactory.h"

#include <string>

#include "oops/util/abor1_cpp.h"
#include "oops/util/Logger.h"

#include "ioda/io/ObsFrameRead.h"
#include "ioda/io/ObsFrameWrite.h"

namespace ioda {

//-------------------------------------------------------------------------------------
std::shared_ptr<ObsFrame> ObsFrameFactory::create(
                                             const ObsIoActions action, const ObsIoModes mode,
                                             const ObsSpaceParameters & params) {
    std::shared_ptr<ObsFrame> obsFrame;
    if ((action == ObsIoActions::CREATE_GENERATOR) || (action == ObsIoActions::OPEN_FILE)) {
        // Instantiate an ObsFrameRead object
        obsFrame = std::make_shared<ObsFrameRead>(action, mode, params);
    } else if (action == ObsIoActions::CREATE_FILE) {
        // Instantiate an ObsFrameWrite object
        obsFrame = std::make_shared<ObsFrameWrite>(action, mode, params);
    } else {
        ABORT("ObsFrameFactory::create: Unrecognized action");
    }
    return obsFrame;
}

}  // namespace ioda
