/*
 * (C) Copyright 2026 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#pragma once

#include <memory>
#include <string>

// Forward class declarations
namespace osdf {
    class IFrame;
    class FrameMetadata;
}
namespace eckit {
    namespace mpi {
        class Comm;
    }
}
namespace ioda {
    class ObsDataOutParameters;
    namespace ObsIoPool {
        class ObsIoPool;
    }
}

namespace ioda {
namespace writer {

/// \brief save source OSDF(s) to file(s) across all ranks in the io pool
/// \param dataOutParams obs space data out (obsdataout) parameters
/// \param obsIoPool io pool object
/// \param commAll MPI communicator for all ranks
/// \param obsName obs space name (used in the log messaging)
/// \param srcOsdf source OSDF to be saved
/// \param osdfMetadata frame metadata for srcOsdf
void saveObs(const ObsDataOutParameters & dataOutParams,
             const std::unique_ptr<ObsIoPool::ObsIoPool> & obsIoPool,
             const eckit::mpi::Comm & commAll,
             const std::string & obsName,
             std::unique_ptr<osdf::IFrame> & srcOsdf,
             osdf::FrameMetadata & osdfMetadata);

}  // namespace writer
}  // namespace ioda
