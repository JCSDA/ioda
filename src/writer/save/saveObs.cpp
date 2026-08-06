/*
 * (C) Copyright 2026 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/writer/save/saveObs.hpp"

#include "eckit/exception/Exceptions.h"
#include "eckit/mpi/Comm.h"

#include "ioda/containers/IFrame.h"
#include "ioda/containers/FrameMetadata.h"
#include "ioda/ObsDataIoParameters.h"
#include "ioda/obsIoPool/ObsIoPool.hpp"
#include "ioda/writer/save/saveObsToNetcdf.hpp"
#include "ioda/writer/save/saveObsToOdb.hpp"

#include "oops/util/Logger.h"

namespace ioda {
namespace writer {

//--------------------------------------------------------------------------------
// Declarations for private functions
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
// Public functions
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
void saveObs(const ObsDataOutParameters & dataOutParams,
             const std::unique_ptr<ObsIoPool::ObsIoPool> & obsIoPool,
             const eckit::mpi::Comm & commAll,
             const std::string & obsName,
             std::unique_ptr<osdf::IFrame> & srcOsdf,
             osdf::FrameMetadata & osdfMetadata) {
  oops::Log::trace() << "writer::saveObs start" << std::endl;
  // todo(SRH): For now, we are only supporting HDF5 and ODB file types. We will
  // eventually want to support BUFR.
  const std::string fileType =
    dataOutParams.engine.value().engineParameters.value().type.value();

  if (fileType == "H5File") {
    // Collectively save the OSDF to NetCDF across the io pool ranks.
    // It is assumed that all of the obs have been collected onto the
    // io pool ranks prior to calling this function.
    if (obsIoPool->inIoPool()) {
      saveOsdfToNetcdf(dataOutParams, obsIoPool->commPool(), srcOsdf, osdfMetadata);
    }
  } else {
    // Collectively save the OSDF to ODB across the io pool ranks.
    // It is assumed that all of the obs have been collected onto the
    // io pool ranks prior to calling this function.
    ASSERT(fileType == "ODB");
    if (obsIoPool->inIoPool()) {
      saveOsdfToOdb(dataOutParams, obsIoPool->commPool(), srcOsdf, osdfMetadata);
    }
  }

  // Issue the summary message describing where this obs space wrote its data. The file
  // name reported here is the one from the configuration, before the io pool rank number
  // gets appended for multiple file output.
  oops::Log::info() << obsName << ": save database to "
                    << dataOutParams.engine.value().engineParameters.value().fileName.value()
                    << " (io pool size: " << obsIoPool->poolSize() << ")" << std::endl;
  oops::Log::trace() << "writer::saveObs end" << std::endl;
}

//--------------------------------------------------------------------------------
// Private functions
//--------------------------------------------------------------------------------


}  // namespace writer
}  // namespace ioda
