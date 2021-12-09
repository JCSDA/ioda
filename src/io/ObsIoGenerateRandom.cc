/*
 * (C) Copyright 2017-2019 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#include "ioda/io/ObsIoGenerateRandom.h"

#include "ioda/Engines/Factory.h"
#include "ioda/io/ObsIoGenerateUtils.h"
#include "ioda/Misc/Dimensions.h"

#include "oops/util/missingValues.h"
#include "oops/util/Random.h"

namespace ioda {

static ObsIoMaker<ObsIoGenerateRandom> maker("GenerateRandom");

//----------------------------- public functions ------------------------------------
//-----------------------------------------------------------------------------------
ObsIoGenerateRandom::ObsIoGenerateRandom(const Parameters_ &ioParams,
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

    oops::Log::trace() << "Constructing ObsIoGenerateRandom: Random method" << std::endl;

    // Create the in-memory ObsGroup
    Dimensions_t numLocs = ioParams.random.numObs;
    newDims.push_back(
                      ioda::NewDimensionScale<int>("nlocs", numLocs, numLocs, numLocs));
    obs_group_ = ObsGroup::generate(backend, newDims);

    // Fill in the ObsGroup with the generated data
    genDistRandom(ioParams.random,
                  obsSpaceParams.windowStart(), obsSpaceParams.windowEnd(), obsSpaceParams.comm(),
                  ioParams.obsErrors, obsSimVarsVal->variables());

    // record counts useful for an obs source
    nlocs_ = obs_group_.vars.open("nlocs").getDimensions().dimsCur[0];

    // Collect variable and dimension infomation for downstream use
    collectVarDimInfo(obs_group_, var_list_, dim_var_list_, dims_attached_to_vars_,
                      max_var_size_);

    // record variables by which observations should be grouped into records
    obs_grouping_vars_ = ioParams.obsGrouping.value().obsGroupVars;
}

ObsIoGenerateRandom::~ObsIoGenerateRandom() {}

//----------------------------- public functions -------------------------------

//----------------------------- private functions -----------------------------------
//-----------------------------------------------------------------------------------
void ObsIoGenerateRandom::genDistRandom(const EmbeddedObsGenerateRandomParameters & params,
                                        const util::DateTime & winStart,
                                        const util::DateTime & winEnd,
                                        const eckit::mpi::Comm & comm,
                                        const std::vector<float> & obsErrors,
                                        const std::vector<std::string> & simVarNames) {
    ASSERT(obsErrors.size() == simVarNames.size());

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
    std::vector<int64_t> dts(numLocs, 0.0);

    util::Duration durZero(0);
    util::Duration durOneSec(1);
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

    std::string epoch = std::string("seconds since ") + winStart.toString();
    // Transfer the generated values to the ObsGroup
    storeGenData(latVals, lonVals, dts, epoch, simVarNames, obsErrors, obs_group_);
}

//-----------------------------------------------------------------------------------
void ObsIoGenerateRandom::print(std::ostream & os) const {
    os << "ObsIoGenerateRandom: " << std::endl;
}

}  // namespace ioda
