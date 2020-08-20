/*
 * (C) Copyright 2017-2019 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#include "ioda/io/ObsGenerate.h"

////////////////////////////////////////////////////////////////////////
// Implementation of ObsIo for a YAML generator
////////////////////////////////////////////////////////////////////////

namespace ioda {

//-----------------------------------------------------------------------------------
ObsGenerate::ObsGenerate(const ObsIoActions action, const ObsIoModes mode,
                         const ObsIoParameters & params) : ObsIo(action, mode, params) {
    oops::Log::trace() << "Constructing ObsGenerate" << std::endl;
}

ObsGenerate::~ObsGenerate() {}

//-----------------------------------------------------------------------------------
void ObsGenerate::print(std::ostream & os) const {
    os << "ObsGenerate: " << std::endl;
}

}  // namespace ioda
