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
std::shared_ptr<ioda::ObsIo> ObsIoFactory::create(const eckit::LocalConfiguration & config) { 
    if (config.has("obsdatain")) {
        // Open a backend file for reading
        oops::Log::info() << "ObsIoFactory::Create: file" << std::endl;
    } else if (config.has("generate")) {
        // Open an in-memory backend
        oops::Log::info() << "ObsIoFactory::Create: in-memory" << std::endl;
    } else {
        oops::Log::error() << "ObsIoFactory::Create: Unrecognized configuration action: "
                           << config << std::endl;
        ABORT("IodaIO::Create: Unrecognized configuration");
    }
}

}  // namespace ioda
