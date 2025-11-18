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
}

namespace ioda {
namespace reader {

/// \brief parallel load across all ranks in the io pool
/// \param dataInParams parameters for obsdatain configuration
/// \param ioPoolComm eckit MPI communicator group for the io pool
/// \param destOSDF destination OSDF container
/// \param osdfMetadata frame metadata for dest OSDF
void loadOsdfFromNetcdf(const ObsDataInParameters & dataInParams,
                        const eckit::mpi::Comm & ioPoolComm,
                        std::unique_ptr<osdf::IFrame> & destOSDF,
                        osdf::FrameMetadata & osdfMetadata);

}  // namespace reader
}  // namespace ioda
