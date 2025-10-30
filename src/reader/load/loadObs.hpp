/*
 * (C) Copyright 2025 UCAR
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
    class ObsDataInParameters;
    namespace IoPool {
        class IoPoolParameters;
    }
}

namespace ioda {
namespace reader {

/// \brief parallel load across all ranks in the io pool
/// \param dataInParams obs space data input (obsdatain) parameters
/// \param ioPoolParams io pool parameters
/// \param commAll MPI communicator for all ranks
/// \param destOsdf destination OSDF to be populated
/// \param osdfMetadata frame metadata for dest OSDF
void loadObs(const ObsDataInParameters & dataInParams,
             const IoPool::IoPoolParameters & ioPoolParams,
             const eckit::mpi::Comm & commAll,
             std::unique_ptr<osdf::IFrame> & destOsdf,
             osdf::FrameMetadata & osdfMetadata);

}  // namespace reader
}  // namespace ioda
