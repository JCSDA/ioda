/*
 * (C) Copyright 2017-2019 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#include "ioda/io/ObsGenerate.h"

#include "ioda/Layout.h"

#include "oops/util/abor1_cpp.h"

////////////////////////////////////////////////////////////////////////
// Implementation of ObsIo for a YAML generator
////////////////////////////////////////////////////////////////////////

namespace ioda {

//-----------------------------------------------------------------------------------
ObsGenerate::ObsGenerate(const ObsIoActions action, const ObsIoModes mode,
                         const ObsIoParameters & params) : ObsIo(action, mode, params) {
    // Create an in-memory backend and attach it to an in-memory ObsGroup
    Engines::BackendNames backendName;
    Engines::BackendCreationParameters backendParams;

    backendName = Engines::BackendNames::ObsStore;
    Group backend = constructBackend(backendName, backendParams);

    NewDimensionScales_t newDims;
    if (action == ObsIoActions::CREATE_GENERATOR) {
        if (params.in_type() == ObsIoTypes::GENERATOR_RANDOM) {
            oops::Log::trace() << "Constructing ObsGenerate: Random method" << std::endl;

            // Grab the parameters
            int numLocs = params.params_in_gen_rand_.numObs;
            float latStart = params.params_in_gen_rand_.latStart;
            float latEnd = params.params_in_gen_rand_.latEnd;
            float lonStart = params.params_in_gen_rand_.lonStart;
            float lonEnd = params.params_in_gen_rand_.lonEnd;
            std::vector<float> obsErrors;
            if (params.params_in_gen_rand_.obsErrors.value() != boost::none) {
                obsErrors = params.params_in_gen_rand_.obsErrors.value().get();
            }

            // Create the in-memory ObsGroup
            newDims.push_back(
                std::make_shared<ioda::NewDimensionScale<int>>("nlocs", numLocs, numLocs, numLocs));
            obs_group_ = ObsGroup::generate(backend, newDims);

        } else if (params.in_type() == ObsIoTypes::GENERATOR_LIST) {
            oops::Log::trace() << "Constructing ObsGenerate: List method" << std::endl;

            // Grab the parameters
            std::vector<float> latVals = params.params_in_gen_list_.lats;
            std::vector<float> lonVals = params.params_in_gen_list_.lons;
            std::vector<std::string> dtVals = params.params_in_gen_list_.datetimes;
            std::vector<float> obsErrors;
            if (params.params_in_gen_rand_.obsErrors.value() != boost::none) {
                obsErrors = params.params_in_gen_rand_.obsErrors.value().get();
            }

            // Create the in-memory ObsGroup
            int numLocs = latVals.size();
            newDims.push_back(
                std::make_shared<ioda::NewDimensionScale<int>>("nlocs", numLocs, numLocs, numLocs));
            obs_group_ = ObsGroup::generate(backend, newDims);

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
