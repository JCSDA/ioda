/*
 * (C) Copyright 2022 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include <iomanip>
#include <sstream>

#include "ioda/Io/IoPoolUtils.h"

namespace ioda {

// -----------------------------------------------------------------------------
std::string uniquifyFileName(const std::string & fileName, std::size_t rankNum,
                             int timeRankNum) {
    // Attach the rank number to the output file name to avoid collisions when running
    // with multiple MPI tasks.
    std::string uniqueFileName = fileName;

    // Find the right-most dot in the file name, and use that to pick off the file name
    // and file extension.
    std::size_t found = uniqueFileName.find_last_of(".");
    if (found == std::string::npos)
      found = uniqueFileName.length();

    // Get the process rank number and format it
    std::ostringstream ss;
    ss << "_" << std::setw(4) << std::setfill('0') << rankNum;
    if (timeRankNum >= 0) ss << "_" << timeRankNum;

    // Construct the output file name
    return uniqueFileName.insert(found, ss.str());
}

}  // namespace ioda
