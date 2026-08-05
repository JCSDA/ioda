/*
 * (C) Copyright 2026 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#pragma once

#include <memory>

namespace eckit {
  namespace mpi {
    class Comm;
  }
}

namespace osdf {
  class IFrame;
  class FrameMetadata;
}

namespace ioda {
  class ObsDataOutParameters;
  class Distribution;
  struct ObsSourceStats;
  namespace IoPool {
    class IoPoolParameters;
  }

namespace writer {

/// \brief write OSDF into an output file
/// \param dataOutParams obs space data output (obsdataout) parameters
/// \param ioPoolParams io pool parameters
/// \param commAll MPI communicator for all ranks
/// \param ospaceDist distribution object for the caller's obs space
/// \param srcOsdf source OSDF to be transferred to the output file
/// \param obsSourceStats statistics about the obs source (file)
/// \param osdfMetadata frame metadata object associated with srcOsdf
/// \param preserveInputs preserves the srcOsdf (requires more memory). Otherwise it is overwritten.
void obsWrite(const ioda::ObsDataOutParameters & dataOutParams,
              const ioda::IoPool::IoPoolParameters & ioPoolParams,
              const eckit::mpi::Comm & commAll,
              std::shared_ptr<Distribution> & ospaceDist,
              std::unique_ptr<osdf::IFrame> & srcOsdf,
              ObsSourceStats & obsSourceStats,
              osdf::FrameMetadata & osdfMetadata,
              const bool preserveInputs = false);

}  // namespace writer
}  // namespace ioda
