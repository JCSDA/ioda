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
#include "ioda/ioPool/IoPoolParameters.h"
#include "ioda/obsIoPool/ObsIoPool.hpp"
#include "ioda/writer/save/saveObsToNetcdf.hpp"

#include "oops/mpi/mpi.h"
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
             const IoPool::IoPoolParameters & ioPoolParams,
             const eckit::mpi::Comm & commAll,
             std::unique_ptr<osdf::IFrame> & srcOsdf,
             osdf::FrameMetadata & osdfMetadata) {
  oops::Log::trace() << "writer::saveObs start" << std::endl;
  // todo(SRH): For now, we are only supporting HDF5 file types. We will
  // eventually want to support ODB.
  const std::string fileType =
    dataOutParams.engine.value().engineParameters.value().type.value();

  // Form the io pool.
  std::unique_ptr<ObsIoPool::ObsIoPool> obsIoPool =
    std::make_unique<ObsIoPool::ObsIoPool>(ioPoolParams, commAll);
  if (fileType == "H5File") {
    // Collectively save the OSDF to NetCDF across the io pool ranks.
    // It is assumed that all of the obs have been collected onto the
    // io pool ranks prior to calling this function.
    if (obsIoPool->inIoPool()) {
      saveOsdfToNetcdf(dataOutParams, obsIoPool->commPool(), srcOsdf, osdfMetadata);
    }
  } else {
    const std::string errMsg = "Unsupported output file type: " + fileType +
                               " Must use 'H5File' for now.";
    throw eckit::BadParameter(errMsg, Here());
  }
  oops::Log::trace() << "writer::saveObs end" << std::endl;
}

//--------------------------------------------------------------------------------
// Private functions
//--------------------------------------------------------------------------------


}  // namespace writer
}  // namespace ioda
