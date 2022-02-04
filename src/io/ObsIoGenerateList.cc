/*
 * (C) Copyright 2017-2019 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#include "ioda/io/ObsIoGenerateList.h"

#include "ioda/Engines/Factory.h"
#include "ioda/io/ObsIoGenerateUtils.h"
#include "ioda/Misc/Dimensions.h"
#include "ioda/Variables/VarUtils.h"

#include "oops/util/missingValues.h"

namespace ioda {

static ObsIoMaker<ObsIoGenerateList> maker("GenerateList");

//----------------------------- public functions ------------------------------------
//-----------------------------------------------------------------------------------
ObsIoGenerateList::ObsIoGenerateList(const Parameters_ &ioParams,
                                     const ObsSpaceParameters & obsSpaceParams) :
  ObsIo() {
    // Create an in-memory backend and attach it to an in-memory ObsGroup
    Engines::BackendNames backendName;
    Engines::BackendCreationParameters backendParams;

    backendName = Engines::BackendNames::ObsStore;
    Group backend = constructBackend(backendName, backendParams);

    NewDimensionScales_t newDims;
    const boost::optional<oops::Variables> & obsSimVarsVal =
      obsSpaceParams.top_level_.simVars.value();

    oops::Log::trace() << "Constructing ObsIoGenerateList: List method" << std::endl;

    // Create the in-memory ObsGroup
    Dimensions_t numLocs =
      ioParams.list.lats.value().size();
    newDims.push_back(
                      ioda::NewDimensionScale<int>("nlocs", numLocs, numLocs, numLocs));
    obs_group_ = ObsGroup::generate(backend, newDims);

    // Fill in the ObsGroup with the generated data
    genDistList(ioParams.list, ioParams.obsErrors, obsSimVarsVal->variables());

    // record counts useful for an obs source
    nlocs_ = obs_group_.vars.open("nlocs").getDimensions().dimsCur[0];

    // Collect variable and dimension infomation for downstream use
    VarUtils::collectVarDimInfo(obs_group_, var_list_, dim_var_list_, dims_attached_to_vars_,
                                max_var_size_);

    // record variables by which observations should be grouped into records
    obs_grouping_vars_ = ioParams.obsGrouping.value().obsGroupVars;
}

ObsIoGenerateList::~ObsIoGenerateList() {}

//----------------------------- public functions -------------------------------

//----------------------------- private functions -----------------------------------
//-----------------------------------------------------------------------------------
void ObsIoGenerateList::genDistList(const EmbeddedObsGenerateListParameters & params,
                                    const std::vector<float> & obsErrors,
                                    const std::vector<std::string> & simVarNames) {
    ASSERT(obsErrors.size() == simVarNames.size());

    // Grab the parameters
    const std::vector<float> latVals = params.lats;
    const std::vector<float> lonVals = params.lons;
    const std::vector<int64_t> dts = params.dateTimes;
    const std::string epoch = params.epoch.value();

    // Transfer the specified values to the ObsGroup
    storeGenData(latVals, lonVals, dts, epoch, simVarNames, obsErrors, obs_group_);
}

//-----------------------------------------------------------------------------------
void ObsIoGenerateList::print(std::ostream & os) const {
    os << "ObsIoGenerateList: " << std::endl;
}

}  // namespace ioda
