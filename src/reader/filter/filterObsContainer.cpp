/*
 * (C) Copyright 2025 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include <cmath>

#include "ioda/reader/filter/filterObsContainer.hpp"

#include "ioda/containers/IFrame.h"

#include "oops/util/Logger.h"
#include "oops/util/missingValues.h"
#include "oops/util/TimeWindow.h"

namespace ioda {
namespace reader {

//---------------------------------------------------------------------
void filterObsContainer(const util::TimeWindow & timeWindow,
                        std::unique_ptr<osdf::IFrame> & osdfCont) {
    // First need to check that we have all the required variables
    // which are latitude, longitude and dateTime.
    const std::string dateTimeColName = "MetaData/dateTime";
    const std::string latColName = "MetaData/latitude";
    const std::string lonColName = "MetaData/longitude";

    bool haveRequiredVars = true;
    std::vector<std::string> missingVars(0);
    if (!osdfCont->hasColumn(dateTimeColName)) {
        haveRequiredVars = false;
        missingVars.push_back(dateTimeColName);
    }
    if (!osdfCont->hasColumn(latColName)) {
        haveRequiredVars = false;
        missingVars.push_back(latColName);
    }
    if (!osdfCont->hasColumn(lonColName)) {
        haveRequiredVars = false;
        missingVars.push_back(lonColName);
    }
    if (!haveRequiredVars) {
        std::string errMsg =
            std::string("filterObsContainer: Missing required variables:\n");
        for (std::size_t i = 0; i < missingVars.size(); ++i) {
            errMsg += std::string("    ") + missingVars[i] + std::string("\n");
        }
        throw eckit::BadValue(errMsg, Here());
    }

    // If we made it to here, we have the required variables to do the filtering.
    // Read in the dateTime values and do the window check (built into the
    // TimeWindow class).
    // TODO(srh) We don't have a way to store the epoch in an OSDF yet, so
    // for now use a default of Jan 1, 1970, 0Z.
    std::vector<int64_t> dateTimeVals;
    osdfCont->getColumn(dateTimeColName, dateTimeVals);
    util::DateTime epochDt("1970-01-01T00:00:00Z");
    timeWindow.setEpoch(epochDt);
    std::vector<bool> filterMask = timeWindow.createTimeMask(dateTimeVals);

    // Add missing date/time and lat/lon values to the filter mask.
    std::vector<float> latVals, lonVals;
    osdfCont->getColumn(latColName, latVals);
    osdfCont->getColumn(lonColName, lonVals);
    const int64_t int64MissingVal = util::missingValue<int64_t>();
    const float floatMissingVal = util::missingValue<float>();
    for (std::size_t i = 0; i < filterMask.size(); ++i) {
        if (filterMask[i]) {
            if ((dateTimeVals[i] == int64MissingVal) ||
                (latVals[i] == floatMissingVal) ||
                (lonVals[i] == floatMissingVal)) {
                filterMask[i] = false;
            }
        }
    }

    // Remove all masked rows
    osdfCont->removeRows(filterMask);
}

}  // namespace reader
}  // namespace ioda
