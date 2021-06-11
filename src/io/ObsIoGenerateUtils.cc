/*
 * (C) Copyright 2017-2019 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#include "ioda/io/ObsIoGenerateUtils.h"

#include "ioda/Misc/Dimensions.h"
#include "ioda/ObsGroup.h"
#include "ioda/Variables/Variable.h"

#include "oops/util/missingValues.h"

namespace ioda {

void storeGenData(const std::vector<float> & latVals,
                  const std::vector<float> & lonVals,
                  const std::vector<std::string> & dtStrings,
                  const std::vector<std::string> & obsVarNames,
                  const std::vector<float> & obsErrors,
                  ObsGroup &obsGroup) {
    // Generated data is a set of vectors for now.
    //     MetaData group
    //        latitude
    //        longitude
    //        datetime
    //
    //     ObsError group
    //        list of simulated variables in obsVarNames

    Variable nlocsVar = obsGroup.vars["nlocs"];

    float missingFloat = util::missingValue(missingFloat);
    std::string missingString("missing");

    ioda::VariableCreationParameters float_params;
    float_params.chunk = true;
    float_params.compressWithGZIP();
    float_params.setFillValue<float>(missingFloat);

    ioda::VariableCreationParameters string_params;
    float_params.chunk = true;
    float_params.compressWithGZIP();
    float_params.setFillValue<std::string>(missingString);

    std::string latName("MetaData/latitude");
    std::string lonName("MetaData/longitude");
    std::string dtName("MetaData/datetime");

    // Create, write and attach units attributes to the variables
    obsGroup.vars.createWithScales<float>(latName, { nlocsVar }, float_params)
        .write<float>(latVals)
        .atts.add<std::string>("units", std::string("degrees_east"));
    obsGroup.vars.createWithScales<float>(lonName, { nlocsVar }, float_params)
        .write<float>(lonVals)
        .atts.add<std::string>("units", std::string("degrees_north"));
    obsGroup.vars.createWithScales<std::string>(dtName, { nlocsVar }, string_params)
        .write<std::string>(dtStrings)
        .atts.add<std::string>("units", std::string("ISO 8601 format"));

    for (std::size_t i = 0; i < obsVarNames.size(); ++i) {
        std::string varName = std::string("ObsError/") + obsVarNames[i];
        std::vector<float> obsErrVals(latVals.size(), obsErrors[i]);
        obsGroup.vars.createWithScales<float>(varName, { nlocsVar }, float_params)
            .write<float>(obsErrVals);
    }
}

}  // namespace ioda
