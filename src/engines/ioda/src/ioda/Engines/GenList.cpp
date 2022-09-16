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
GenList::GenList(const Parameters_ & params, const util::DateTime & winStart,
                 const util::DateTime & winEnd, const eckit::mpi::Comm & comm,
                 const eckit::mpi::Comm & timeComm, const std::vector<std::string> & obsVarNames)
                     : ReaderBase(winStart, winEnd, comm, timeComm, obsVarNames) {
    oops::Log::trace() << "ioda::Engines::GenList start constructor" << std::endl;
    // Create a backend backed by memory, and fill with results from the list style generator
    Engines::BackendNames backendName = BackendNames::ObsStore;
    Engines::BackendCreationParameters backendParams;
    Group backend = constructBackend(backendName, backendParams);

    // Create the in-memory ObsGroup
    NewDimensionScales_t newDims;
    Dimensions_t numLocs = params.lats.value().size();
    newDims.push_back(ioda::NewDimensionScale<int>("nlocs", numLocs, numLocs, numLocs));
    obs_group_ = ObsGroup::generate(backend, newDims);

    // Fill in the ObsGroup with the generated data
    genDistList(params);

    oops::Log::trace() << "ioda::Engines::GenList end constructor" << std::endl;
}

//-------------------- private functions -------------------------------
void GenList::genDistList(const GenList::Parameters_ & params) {
    // Grab the parameters
    const std::vector<float> obsErrors = params.obsErrors;
    ASSERT(obsErrors.size() == obsVarNames_.size());

    const std::vector<float> latVals = params.lats;
    const std::vector<float> lonVals = params.lons;
    const std::vector<int64_t> dts = params.dateTimes;
    const std::string epoch = params.epoch.value();

    // Transfer the specified values to the ObsGroup
    storeGenData(latVals, lonVals, dts, epoch, obsVarNames_, obsErrors, obs_group_);
}

void GenList::print(std::ostream & os) const {
  os << "generate from listed locations";
}

}  // namespace Engines
}  // namespace ioda
