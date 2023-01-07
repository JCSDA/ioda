/*
 * (C) Copyright 2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <string>

#include "ioda/Engines/HH.h"
#include "ioda/Engines/ODC.h"
#include "ioda/Group.h"
#include "ioda/ObsGroup.h"
#include "ioda/defs.h"
#include "eckit/filesystem/PathName.h"

#include "oops/util/missingValues.h"

int main(int argc, char** argv) {
  try {
    using namespace ioda;

    // Assume that unit tests will either set maxNumberChannels or
    // timeWindowStart and timeWindowExtendedLowerBound.
    // In other words all three optional arguments are never set together.
    if (argc != 5 && argc != 6 && argc != 7) {
      std::cerr << "Usage: " << argv[0]
                << " subtype_str "
                   "filename mappingFile queryFile [maxNumberChannels]"
                << " [timeWindowStart] [timeWindowExtendedLowerBound]"
                << std::endl;
      return 1;
    }

    Engines::ODC::ODC_Parameters odcparams;
    const std::string subtype_str = argv[1];
    odcparams.filename    = argv[2];
    odcparams.mappingFile = argv[3];
    odcparams.queryFile   = argv[4];
    if (argc == 6) {
      odcparams.maxNumberChannels = std::stoi(argv[5]);
    } else {
      odcparams.maxNumberChannels = 0;
    }

    if (argc == 7) {
      odcparams.timeWindowStart = util::DateTime(argv[5]);
      odcparams.timeWindowExtendedLowerBound = util::DateTime(argv[6]);
    } else {
      const util::DateTime missingDate = util::missingValue(missingDate);
      odcparams.timeWindowStart = missingDate;
      odcparams.timeWindowExtendedLowerBound = missingDate;
    }

    auto f = Engines::HH::createFile("testoutput/test-" + subtype_str + ".hdf",
               Engines::BackendCreateModes::Truncate_If_Exists);
    auto og = Engines::ODC::openFile(odcparams, f);
  } catch (const std::exception& e) {
    ioda::unwind_exception_stack(e);
    return 1;
  }
  return 0;
}
