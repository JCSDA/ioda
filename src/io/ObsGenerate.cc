/*
 * (C) Copyright 2017-2019 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#include "ioda/io/ObsGenerate.h"

#include <typeindex>
#include <typeinfo>

#include "ioda/core/IodaUtils.h"
#include "ioda/Layout.h"
#include "ioda/Misc/Dimensions.h"

#include "oops/util/abor1_cpp.h"
#include "oops/util/missingValues.h"
#include "oops/util/Duration.h"
#include "oops/util/Random.h"

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

            // Create the in-memory ObsGroup
            int numLocs = params.in_gen_rand_.numObs; // number of locations to generate
            newDims.push_back(
                std::make_shared<ioda::NewDimensionScale<int>>("nlocs", numLocs, numLocs, numLocs));
            obs_group_ = ObsGroup::generate(backend, newDims);

            // Fill in the ObsGroup with the generated data
            genDistRandom(params.in_gen_rand_, params.windowStart(),
                          params.windowEnd(), params.comm(), params.top_level_.simVars);

            // record maximum frame size
            max_frame_size_ = params.in_gen_rand_.maxFrameSize;
        } else if (params.in_type() == ObsIoTypes::GENERATOR_LIST) {
            oops::Log::trace() << "Constructing ObsGenerate: List method" << std::endl;

            // Create the in-memory ObsGroup
            int numLocs = params.in_gen_list_.lats.value().size();
            newDims.push_back(
                std::make_shared<ioda::NewDimensionScale<int>>("nlocs", numLocs, numLocs, numLocs));
            obs_group_ = ObsGroup::generate(backend, newDims);

            // Fill in the ObsGroup with the generated data
            genDistList(params.in_gen_list_, params.top_level_.simVars);

            // record maximum frame size
            max_frame_size_ = params.in_gen_list_.maxFrameSize;
        } else {
            ABORT("ObsGenerate: Unrecongnized ObsIoTypes value");
        }
    } else {
        ABORT("ObsGenerate: Unrecongnized ObsIoActions value");
    }

    // fill in the variable information
    for (auto & varName : listAllVars(obs_group_, std::string(""))) {
        if (varIsDimScale(obs_group_, varName)) {
            insertDimVarInfo(varName, varSize0(obs_group_, varName),
                varDtype(obs_group_, varName), varIsDist(obs_group_, varName));
        } else {
            insertVarInfo(varName, varSize0(obs_group_, varName),
                varDtype(obs_group_, varName), varIsDist(obs_group_, varName));
        }
    }

    // record the maximum variable size
    max_var_size_ = varSize0Max();
}

ObsGenerate::~ObsGenerate() {}

//-----------------------------------------------------------------------------------
void ObsGenerate::genDistRandom(const ObsGenerateRandomParameters & params,
                                const util::DateTime & winStart, const util::DateTime & winEnd,
                                const eckit::mpi::Comm & comm,
                                const std::vector<std::string> & simVarNames) {
    /// Grab the parameter values
    int numLocs = params.numObs;
    float latStart = params.latStart;
    float latEnd = params.latEnd;
    float lonStart = params.lonStart;
    float lonEnd = params.lonEnd;
    int ranSeed;
    if (params.ranSeed.value() != boost::none) {
        ranSeed = params.ranSeed.value().get();
    } else {
        ranSeed = std::time(0);  // based on the current date/time.
    }
    std::vector<float> obsErrors = params.obsErrors;
    ASSERT(obsErrors.size() == simVarNames.size());

    // Use the following formula to generate random lat, lon and time values.
    //
    //   val = val1 + (random_number_between_0_and_1 * (val2-val1))
    //
    // where val2 > val1.
    //
    // Create a list of random values between 0 and 1 to be used for generating
    // random lat, lon and time vaules.
    //
    // Use different seeds for lat and lon so that in the case where lat and lon ranges
    // are the same, you get a different sequences for lat compared to lon.
    //
    // Have rank 0 generate the full length random sequences, and then
    // broadcast these to the other ranks. This ensures that every rank
    // contains the same random sequences. If all ranks generated their
    // own sequences, which they could do, the sequences between ranks
    // would be different in the case where random_seed is not specified.
    std::vector<float> ranVals(numLocs, 0.0);
    std::vector<float> ranVals2(numLocs, 0.0);
    if (comm.rank() == 0) {
        util::UniformDistribution<float> ranUD(numLocs, 0.0, 1.0, ranSeed);
        util::UniformDistribution<float> ranUD2(numLocs, 0.0, 1.0, ranSeed+1);

        ranVals = ranUD.data();
        ranVals2 = ranUD2.data();
    }
    comm.broadcast(ranVals, 0);
    comm.broadcast(ranVals2, 0);

    // Form the ranges val2-val for lat, lon, time
    float latRange = latEnd - latStart;
    float lonRange = lonEnd - lonStart;
    util::Duration windowDuration(winEnd - winStart);
    float timeRange = static_cast<float>(windowDuration.toSeconds());

    // Create vectors for lat, lon, time, fill them with random values
    // inside their respective ranges, and put results into the obs container.
    std::vector<float> latVals(numLocs, 0.0);
    std::vector<float> lonVals(numLocs, 0.0);
    std::vector<std::string> dtStrings(numLocs, "");

    util::Duration durZero(0);
    util::Duration durOneSec(1);
    for (std::size_t ii = 0; ii < numLocs; ii++) {
        latVals[ii] = latStart + (ranVals[ii] * latRange);
        lonVals[ii] = lonStart + (ranVals2[ii] * lonRange);

        // Currently the filter for time stamps on obs values is:
        //
        //     windowStart < ObsTime <= windowEnd
        //
        // If we get a zero offsetDt, then change it to 1 second so that the observation
        // will remain inside the timing window.
        util::Duration offsetDt(static_cast<int64_t>(ranVals[ii] * timeRange));
        if (offsetDt == durZero) {
            offsetDt = durOneSec;
        }
        // convert result to ISO 8601 string
        util::DateTime dtVal = winStart + offsetDt;
        dtStrings[ii] = dtVal.toString();
    }

    // Transfer the generated values to the ObsGroup
    storeGenData(latVals, lonVals, dtStrings, simVarNames, obsErrors);
}

//-----------------------------------------------------------------------------------
void ObsGenerate::genDistList(const ObsGenerateListParameters & params,
                              const std::vector<std::string> & simVarNames) {
    // Grab the parameters
    std::vector<float> latVals = params.lats;
    std::vector<float> lonVals = params.lons;
    std::vector<std::string> dtStrings = params.datetimes;
    std::vector<float> obsErrors = params.obsErrors;
    ASSERT(obsErrors.size() == simVarNames.size());

    // Transfer the specified values to the ObsGroup
    storeGenData(latVals, lonVals, dtStrings, simVarNames, obsErrors);
}

//-----------------------------------------------------------------------------------
void ObsGenerate::storeGenData(const std::vector<float> & latVals,
                               const std::vector<float> & lonVals,
                               const std::vector<std::string> & dtStrings,
                               const std::vector<std::string> & obsVarNames,
                               const std::vector<float> & obsErrors) {
    // Generated data is a set of vectors for now.
    //     MetaData group
    //        latitude
    //        longitude
    //        datetime
    //
    //     ObsError group
    //        list of simulated variables in obsVarNames

    Variable nlocsVar = obs_group_.vars["nlocs"];

    float missingFloat = util::missingValue(missingFloat); // JEDI missing value mark
    std::string missingString("missing");

    ioda::VariableCreationParameters float_params;
    float_params.chunk = true;
    float_params.compressWithGZIP();
    float_params.setFillValue<float>(missingFloat);

    ioda::VariableCreationParameters string_params;
    float_params.chunk = true;
    float_params.compressWithGZIP();
    float_params.setFillValue<std::string>(missingString);

    std::string latName("latitude@MetaData");
    std::string lonName("longitude@MetaData");
    std::string dtName("datetime@MetaData");

    // Create, write and attach units attributes to the variables
    obs_group_.vars.createWithScales<float>(latName, { nlocsVar }, float_params)
        .write<float>(latVals)
        .atts.add<std::string>("units", std::string("degrees_east"));
    obs_group_.vars.createWithScales<float>(lonName, { nlocsVar }, float_params)
        .write<float>(lonVals)
        .atts.add<std::string>("units", std::string("degrees_north"));
    obs_group_.vars.createWithScales<std::string>(dtName, { nlocsVar }, string_params)
        .write<std::string>(dtStrings)
        .atts.add<std::string>("units", std::string("ISO 8601 format"));

    for (std::size_t i = 0; i < obsVarNames.size(); ++i) {
        std::string varName = obsVarNames[i] + std::string("@ObsError");
        std::vector<float> obsErrVals(latVals.size(), obsErrors[i]);
        obs_group_.vars.createWithScales<float>(varName, { nlocsVar }, float_params)
            .write<float>(obsErrVals)
            .atts.add<std::string>("units", std::string("estimated standard deviation"));
    }
}

//-----------------------------------------------------------------------------------
void ObsGenerate::print(std::ostream & os) const {
    os << "ObsGenerate: " << std::endl;
}

}  // namespace ioda
