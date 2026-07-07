/*
 * (C) Copyright 2022 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#include "ioda/Engines/GenRandom.h"

#include "ioda/Misc/Dimensions.h"

#include "oops/util/Logger.h"

namespace ioda {
namespace Engines {

//---------------------------------------------------------------------
// GenRandom
//---------------------------------------------------------------------

static ReaderMaker<GenRandom> maker("GenRandom");

// Parameters

// Classes

//------------------------ public functions ----------------------------------
GenRandom::GenRandom(const Parameters_ & params, const ReaderCreationParameters & createParams)
                         : ReaderBase(createParams) {
    oops::Log::trace() << "ioda::Engines::GenRandom start constructor" << std::endl;
    // Create a backend backed by memory and fill with results from a random style generator
    Engines::BackendNames backendName = BackendNames::ObsStore;
    Engines::BackendCreationParameters backendParams;
    Group backend = constructBackend(backendName, backendParams);

    // Create the in-memory ObsGroup
    NewDimensionScales_t newDims;
    Dimensions_t numLocs = params.numObs;
    newDims.push_back(ioda::NewDimensionScale<int>("Location", numLocs, numLocs, numLocs));
    obs_group_ = ObsGroup::generate(backend, newDims);

   // Fill in the ObsGroup with the generated data
    genDistRandom(params);

    oops::Log::trace() << "ioda::Engines::GenRandom end constructor" << std::endl;
}

//------------------------ private functions ----------------------------------
void GenRandom::genDistRandom(const GenRandom::Parameters_ & params) {
    // Generate the data (validation, random generation and value selection are shared with
    // the OSDF reader)
    const GeneratedObsData data =
        generateObsRandom(params, createParams_.obsVarNames, createParams_.timeWindow);

    // Transfer the generated values to the ObsGroup
    storeGenData(data.latVals, data.lonVals, data.vcoordType, data.vcoordVals, data.dts,
                 data.epoch, data.obsVarNames, data.obsValues, data.obsErrors, obs_group_);
}

std::string GenRandom::fileName() const {
   return std::string("/tmp/generate.random.nc4");
}

void GenRandom::print(std::ostream & os) const {
  os << "generate from randomized locations";
}

}  // namespace Engines
}  // namespace ioda
