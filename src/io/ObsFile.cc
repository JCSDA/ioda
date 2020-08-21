/*
 * (C) Copyright 2017-2019 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#include "ioda/io/ObsFile.h"

#include "oops/util/abor1_cpp.h"

////////////////////////////////////////////////////////////////////////
// Implementation of ObsIo for a file
////////////////////////////////////////////////////////////////////////

namespace ioda {

//--------------------------------------------------------------------------------
ObsFile::ObsFile(const ObsIoActions action, const ObsIoModes mode,
                 const ObsIoParameters & params) : ObsIo(action, mode, params) {
    std::string fileName;

    if (action == ObsIoActions::OPEN_FILE) {
        fileName = params.params_in_file_.fileName;
        oops::Log::trace() << "Constructing ObsFile: Opening file for read: "
                           << fileName << std::endl;
    } else if (action == ObsIoActions::CREATE_FILE) {
        fileName = params.params_out_file_.fileName;
        oops::Log::trace() << "Constructing ObsFile: Creating file for write: "
                           << fileName << std::endl;
    } else {
        ABORT("ObsFile: Unrecongnized ObsIoActions value");
    }
}

ObsFile::~ObsFile() {}

//-----------------------------------------------------------------------------------
void ObsFile::print(std::ostream & os) const {
    os << "ObsFile: " << std::endl;
}

}  // namespace ioda
