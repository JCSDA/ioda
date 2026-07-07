/*
 * (C) Copyright 2022 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#include "oops/util/Logger.h"

#include "ioda/Engines/GenList.h"

namespace ioda {
namespace Engines {

//---------------------------------------------------------------------
// GenList
//---------------------------------------------------------------------

static ReaderMaker<GenList> maker("GenList");

// Parameters

// Classes

//-------------------- public functions --------------------------------
GenList::GenList(const Parameters_ & params, const ReaderCreationParameters & createParams)
                     : ReaderBase(createParams) {
    oops::Log::trace() << "ioda::Engines::GenList start constructor" << std::endl;
    // Create a backend backed by memory, and fill with results from the list style generator
    Engines::BackendNames backendName = BackendNames::ObsStore;
    Engines::BackendCreationParameters backendParams;
    Group backend = constructBackend(backendName, backendParams);

    // Create the in-memory ObsGroup
    NewDimensionScales_t newDims;
    Dimensions_t numLocs = params.lats.value().size();
    newDims.push_back(ioda::NewDimensionScale<int>("Location", numLocs, numLocs, numLocs));
    obs_group_ = ObsGroup::generate(backend, newDims);

    // Fill in the ObsGroup with the generated data
    genDistList(params);

    oops::Log::trace() << "ioda::Engines::GenList end constructor" << std::endl;
}

//-------------------- private functions -------------------------------
void GenList::genDistList(const GenList::Parameters_ & params) {
    // Generate the data (validation and value selection are shared with the OSDF reader)
    const GeneratedObsData data = generateObsList(params, createParams_.obsVarNames);

    // Transfer the specified values to the ObsGroup
    storeGenData(data.latVals, data.lonVals, data.vcoordType, data.vcoordVals, data.dts,
                 data.epoch, data.obsVarNames, data.obsValues, data.obsErrors, obs_group_);
}

std::string GenList::fileName() const {
   return std::string("/tmp/generate.list.nc4");
}

void GenList::print(std::ostream & os) const {
  os << "generate from listed locations";
}

}  // namespace Engines
}  // namespace ioda
