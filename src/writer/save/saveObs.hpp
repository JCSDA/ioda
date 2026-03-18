/*
 * (C) Copyright 2026 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#pragma once

#include <memory>

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
    namespace IoPool {
        class IoPoolParameters;
    }
}

namespace ioda {
namespace writer {

/// \brief save source OSDF(s) to file(s) across all ranks in the io pool
/// \param dataOutParams obs space data out (obsdataout) parameters
/// \param ioPoolParams io pool parameters
/// \param commAll MPI communicator for all ranks
/// \param srcOsdf source OSDF to be saved
/// \param osdfMetadata frame metadata for srcOsdf
void saveObs(const ObsDataOutParameters & dataOutParams,
             const IoPool::IoPoolParameters & ioPoolParams,
             const eckit::mpi::Comm & commAll,
             std::unique_ptr<osdf::IFrame> & srcOsdf,
             osdf::FrameMetadata & osdfMetadata);

}  // namespace writer
}  // namespace ioda
