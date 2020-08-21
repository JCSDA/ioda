/*
 * (C) Copyright 2017-2019 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#include "ioda/io/ObsGenerate.h"

#include "oops/util/abor1_cpp.h"

////////////////////////////////////////////////////////////////////////
// Implementation of ObsIo for a YAML generator
////////////////////////////////////////////////////////////////////////

namespace ioda {

//-----------------------------------------------------------------------------------
ObsGenerate::ObsGenerate(const ObsIoActions action, const ObsIoModes mode,
                         const ObsIoParameters & params) : ObsIo(action, mode, params) {
    if (action == ObsIoActions::CREATE_GENERATOR) {
        if (params.in_type() == ObsIoTypes::GENERATOR_RANDOM) {
            oops::Log::trace() << "Constructing ObsGenerate: Random method" << std::endl;
        } else if (params.in_type() == ObsIoTypes::GENERATOR_LIST) {
            oops::Log::trace() << "Constructing ObsGenerate: List method" << std::endl;
        } else {
            ABORT("ObsGenerate: Unrecongnized ObsIoTypes value");
        }
    } else {
        ABORT("ObsGenerate: Unrecongnized ObsIoActions value");
    }
}

ObsGenerate::~ObsGenerate() {}

//-----------------------------------------------------------------------------------
void ObsGenerate::print(std::ostream & os) const {
    os << "ObsGenerate: " << std::endl;
}

}  // namespace ioda
