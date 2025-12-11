/*
 * (C) Copyright 2025 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include <algorithm>
#include <cmath>

#include "eckit/mpi/Comm.h"

#include "ioda/reader/filter/filterObs.hpp"

#include "ioda/containers/FrameMetadata.h"
#include "ioda/containers/IFrame.h"
#include "ioda/core/ObsSourceStats.h"

#include "oops/util/Logger.h"
#include "oops/util/missingValues.h"
#include "oops/util/TimeWindow.h"

namespace ioda {
namespace reader {

//---------------------------------------------------------------------
void filterObs(const util::TimeWindow & timeWindow,
               const eckit::mpi::Comm & commAll,
               ObsSourceStats & obsSourceStats,
               std::unique_ptr<osdf::IFrame> & osdfCont,
               osdf::FrameMetadata & osdfMetadata) {
  // Want to treat an empty input file (sourceNlocs == 0) as a special case
  // in the obs space. sourceNlocs == 0 when there are zero rows in all osdf
  // containers across all MPI ranks. Figure that out first, and if so,
  // fill in the obsSourceStats struct accordingly and skip the filtering steps.
  commAll.allReduce(osdfCont->numRows(), obsSourceStats.sourceNlocs, eckit::mpi::sum());

  if (obsSourceStats.sourceNlocs == 0) {
    // All ranks have zero rows in their osdfCont containers.
    // Fill in the obsSourceStats struct
    obsSourceStats.nlocs = 0;
    obsSourceStats.gNlocs = 0;
    obsSourceStats.gNlocsOutsideTimewindow = 0;
    obsSourceStats.gNlocsRejectQc = 0;
    obsSourceStats.locIndices.resize(0);
  } else {
    // Check that we have all the required variables
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
    std::vector<int64_t> dateTimeVals;
    osdfCont->getColumn(dateTimeColName, dateTimeVals);
    util::DateTime epochDt(osdfMetadata.getDateTimeEpoch());
    timeWindow.setEpoch(epochDt);
    std::vector<bool> filterMask = timeWindow.createTimeMask(dateTimeVals);
    const std::size_t locsOutsideTimewindow =
      std::count(filterMask.begin(), filterMask.end(), false);

    // Add missing date/time and lat/lon values to the filter mask.
    std::size_t locsRejectQc = 0;
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
                ++locsRejectQc;
            }
        }
    }

    // Remove all masked rows. Keep count of locations (row) both
    // before and after the row removal.
    osdfCont->removeRows(filterMask);
    const std::size_t localNlocs = osdfCont->numRows();

    // Fill in the obsSourceStats struct
    obsSourceStats.nlocs = localNlocs;
    commAll.allReduce(localNlocs, obsSourceStats.gNlocs, eckit::mpi::sum());
    commAll.allReduce(locsOutsideTimewindow, obsSourceStats.gNlocsOutsideTimewindow,
                      eckit::mpi::sum());
    commAll.allReduce(locsRejectQc, obsSourceStats.gNlocsRejectQc, eckit::mpi::sum());

    // Record the source location indices that were kept, these are
    // the indices in the filterMask vector which contain a true value.
    obsSourceStats.locIndices.resize(localNlocs);
    std::size_t iloc = 0;
    for (std::size_t i = 0; i < filterMask.size(); ++i) {
      if (filterMask[i]) {
        obsSourceStats.locIndices[iloc] = i;
        ++iloc;
      }
    }
  }
}

}  // namespace reader
}  // namespace ioda
