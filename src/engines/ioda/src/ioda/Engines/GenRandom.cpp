/*
 * (C) Copyright 2022 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#include "ioda/Engines/GenRandom.h"

#include "ioda/Misc/Dimensions.h"
#include "ioda/Variables/VarUtils.h"

#include "oops/util/DateTime.h"
#include "oops/util/Logger.h"
#include "oops/util/missingValues.h"
#include "oops/util/Random.h"

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
    /// Grab the parameter values
    const std::vector<float> obsValues = params.obsValues;
    const std::vector<float> obsErrors = params.obsErrors;
    if (!obsErrors.empty()){
        ASSERT(obsErrors.size() == createParams_.obsVarNames.size());
    }
    if (!obsValues.empty()){
        ASSERT(obsValues.size() == createParams_.obsVarNames.size());
    }

    const size_t numLocs = params.numObs;
    const float latStart = params.latStart;
    const float latEnd = params.latEnd;
    const float lonStart = params.lonStart;
    const float lonEnd = params.lonEnd;
    int ranSeed;
    if (params.ranSeed.value() != boost::none) {
        ranSeed = params.ranSeed.value().get();
    } else {
        ranSeed = std::time(0);  // based on the current date/time.
    }

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
    // Have all ranks generate their own random sequences. This supports doing the
    // reader pool initialize step with only rank zero instantiating a GenRandom engine.
    // The initialize step is done this way so only rank zero opens an input file
    // (when the engine spec is for a file) which reduces much file IO.
    //
    // When a random seed is specified, then all ranks will get the same sequence. However,
    // when a random seed is not specified, then all ranks will get different sequences.
    // This doesn't really matter (except for possible confusion when debugging) since
    // these sequences get applied to variables that use Location selection. Location
    // selection is mutually exclusive across ranks, so the case where two ranks have
    // different values for the same location won't occur because they won't select
    // a common location.
    //
    // Use the reset flag (fifth argument, set to true) to make sure that
    // we get the same result (with the same seed value) regardless of how many
    // times the class is instantiated.
    std::vector<float> ranVals(numLocs, 0.0);
    std::vector<float> ranVals2(numLocs, 0.0);
    util::UniformDistribution<float> ranUD(numLocs, 0.0, 1.0, ranSeed, true);
    util::UniformDistribution<float> ranUD2(numLocs, 0.0, 1.0, ranSeed+1, true);
    ranVals = ranUD.data();
    ranVals2 = ranUD2.data();

    // Form the ranges val2-val for lat, lon, time
    float latRange = latEnd - latStart;
    float lonRange = lonEnd - lonStart;
    util::Duration windowDuration(createParams_.winEnd - createParams_.winStart);
    float timeRange = static_cast<float>(windowDuration.toSeconds());

    // Create vectors for lat, lon, time, fill them with random values
    // inside their respective ranges, and put results into the obs container.
    std::vector<float> latVals(numLocs, 0.0);
    std::vector<float> lonVals(numLocs, 0.0);
    std::vector<int64_t> dts(numLocs, 0.0);

    for (std::size_t ii = 0; ii < numLocs; ii++) {
        latVals[ii] = latStart + (ranVals[ii] * latRange);
        lonVals[ii] = lonStart + (ranVals2[ii] * lonRange);

        // Currently the filter for time stamps on obs values is:
        //
        //     windowStart < ObsTime <= windowEnd
        //
        int64_t offsetDt = static_cast<int64_t>(ranVals[ii] * timeRange);
        // If we get a zero offsetDt, then change it to 1 second so that the observation
        // will remain inside the timing window.
        if (offsetDt == 0) offsetDt = 1;
        dts[ii] = offsetDt;
    }

    const std::string epoch = std::string("seconds since ") + createParams_.winStart.toString();
    // Transfer the generated values to the ObsGroup
    storeGenData(latVals, lonVals, dts, epoch, createParams_.obsVarNames, obsValues,
                 obsErrors, obs_group_);
}

std::string GenRandom::fileName() const {
   return std::string("/tmp/generate.random.nc4");
}

void GenRandom::print(std::ostream & os) const {
  os << "generate from randomized locations";
}

}  // namespace Engines
}  // namespace ioda
