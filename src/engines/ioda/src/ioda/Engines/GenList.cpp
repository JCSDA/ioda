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
    // Grab the parameters
    const std::vector<float> obsVals = params.obsValues;
    const std::vector<float> obsErrors = params.obsErrors;

    if (!obsErrors.empty()){
        ASSERT(obsErrors.size() == createParams_.obsVarNames.size());
    }
    if (!obsVals.empty()){
        ASSERT(obsVals.size() == createParams_.obsVarNames.size());
    }

    const std::vector<float> latVals = params.lats;
    const std::vector<float> lonVals = params.lons;
    const std::vector<int64_t> dts = params.dateTimes;
    const std::string epoch = params.epoch.value();

    std::string vcoordType = "Undefined";
    std::vector<float> vcoordVals;
    if ( params.vcoordType.value() != boost::none ) {
        vcoordType = params.vcoordType.value().get(); 

	if ( vcoordType != "pressure" && vcoordType != "height" ){
	    throw Exception("Invalid vertical coordinate type, " + vcoordType + ", for GenList. Valid values are 'pressure' or 'height'.", ioda_Here());
	}

        if ( params.vcoordVals.value() != boost::none ) {
	    vcoordVals = params.vcoordVals.value().get();
	} else {
  	    throw Exception("If vert coord type specified in GenList then vert coords must also be specified.", ioda_Here());
	}
    }

    // Transfer the specified values to the ObsGroup
    storeGenData(latVals, lonVals, vcoordType, vcoordVals, dts, epoch, 
                 createParams_.obsVarNames, obsVals, obsErrors, obs_group_);
}

std::string GenList::fileName() const {
   return std::string("/tmp/generate.list.nc4");
}

void GenList::print(std::ostream & os) const {
  os << "generate from listed locations";
}

}  // namespace Engines
}  // namespace ioda
