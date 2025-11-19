/*
 * (C) Copyright 2025 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include "ioda/writer/obsWriteEmptyFile.hpp"

#include <netcdf>

#include "eckit/exception/Exceptions.h"

#include "ioda/ObsDataIoParameters.h"

namespace ioda {
namespace writer {

// -----------------------------------------------------------------------------
// Simply open up the output file (from the dataOutParams) and insert
// the Location dimension with zero locations. For now, must write a
// netCDF/hdf5 file.
void obsWriteEmptyFile(const ioda::ObsDataOutParameters & dataOutParams,
                       const std::string & locDimName) {
  const std::string fileType = dataOutParams.engine.value().engineParameters.value().type;
  const std::string fileName = dataOutParams.engine.value().engineParameters.value().fileName;
  if (fileType != "H5File") {
    const std::string errMsg = std::string("Unrecognized file type: ") + fileType +
      std::string(", must use 'H5File' for now");
    throw eckit::UserError(errMsg, Here());
  }

  // Calling addDim with only one argument (dimension name) will create
  // the dimension with unlimited size and set that size to zero. Exactly
  // what we want.
  netCDF::NcFile outFile(fileName, netCDF::NcFile::replace);
  outFile.addDim(locDimName);
  outFile.close();
}

}  // namespace writer
}  // namespace ioda
