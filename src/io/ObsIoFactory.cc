/*
 * (C) Copyright 2017 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#include "ioda/io/ObsIoFactory.h"

#include <string>

#include "oops/util/abor1_cpp.h"
#include "oops/util/Logger.h"

#include "ioda/io/ObsFile.h"
#include "ioda/io/ObsGenerate.h"

namespace ioda {

//-------------------------------------------------------------------------------------
std::shared_ptr<ObsIo> ObsIoFactory::create(const ObsIoActions action, const ObsIoModes mode,
                                            const ObsSpaceParameters & params) {
    std::shared_ptr<ObsIo> obsIo;
    if ((action == ObsIoActions::CREATE_FILE) || (action == ObsIoActions::OPEN_FILE)) {
        // Instantiate an ObsFile object
        obsIo = std::make_shared<ObsFile>(action, mode, params);
    } else if (action == ObsIoActions::CREATE_GENERATOR) {
        // Instantiate an ObsGenerate object
        obsIo = std::make_shared<ObsGenerate>(action, mode, params);
    } else {
        ABORT("ObsIoFactory::create: Unrecognized action");
    }
    return obsIo;
}

}  // namespace ioda
