/*
 * (C) Copyright 2026 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include "ioda/writer/ObsWriter.hpp"

#include <memory>

#include "eckit/mpi/Comm.h"

#include "ioda/containers/IFrame.h"
#include "ioda/containers/FrameMetadata.h"
#include "ioda/core/ObsSourceStats.h"
#include "ioda/distribution/Distribution.h"
#include "ioda/ioPool/IoPoolParameters.h"
#include "ioda/obsIoPool/ObsIoPool.hpp"
#include "ioda/ObsDataIoParameters.h"
#include "ioda/writer/collect/collectObs.hpp"
#include "ioda/writer/save/saveObs.hpp"

#include "oops/util/Logger.h"
#include "oops/util/Timer.h"

namespace ioda {
namespace writer {

// -----------------------------------------------------------------------------
void obsWrite(const ioda::ObsDataOutParameters & dataOutParams,
              const ioda::IoPool::IoPoolParameters & ioPoolParams,
              const eckit::mpi::Comm & commAll,
              const std::string & obsName,
              std::shared_ptr<Distribution> & ospaceDist,
              std::unique_ptr<osdf::IFrame> & srcOsdf,
              ObsSourceStats & obsSourceStats,
              osdf::FrameMetadata & osdfMetadata,
              const bool preserveInputs) {
  oops::Log::trace() << "writer::obsWrite start" << std::endl;
  util::Timer timer("ioda::writer", "obsWrite");

  // Form the io pool.
  std::unique_ptr<ObsIoPool::ObsIoPool> obsIoPool =
    std::make_unique<ObsIoPool::ObsIoPool>(ioPoolParams, commAll);

  // Collect obs from all MPI tasks onto the io pool tasks, and
  // then transfer those obs to the output file(s)
  if (preserveInputs) {
    // create new output objects to pass to collectObs
    std::shared_ptr<Distribution> outOspaceDist;
    ObsSourceStats outObsSourceStats;
    std::unique_ptr<osdf::IFrame> outOsdf;
    collectObs(obsIoPool, ospaceDist, obsSourceStats, srcOsdf,
               outOspaceDist, outObsSourceStats, outOsdf);
    saveObs(dataOutParams, obsIoPool, commAll, obsName, outOsdf, osdfMetadata);
    outOsdf.reset();  // free memory used for the output osdf since we won't need it anymore
  } else {
    collectObs(obsIoPool, ospaceDist, obsSourceStats, srcOsdf);
    saveObs(dataOutParams, obsIoPool, commAll, obsName, srcOsdf, osdfMetadata);
  }

  oops::Log::trace() << "writer::obsWrite end" << std::endl;
}

}  // namespace writer
}  // namespace ioda
