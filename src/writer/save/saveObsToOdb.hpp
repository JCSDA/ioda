/*
 * (C) Crown copyright 2026, Met Office
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
}  // namespace osdf
namespace eckit {
namespace mpi {
class Comm;
}
}  // namespace eckit
namespace ioda {
class ObsDataOutParameters;
}

namespace ioda {
namespace writer {
/// \brief save OSDF to file for this io pool rank
/// \param dataOutParams parameters for obsdataout configuration
/// \param ioPoolComm eckit MPI communicator group for the io pool
/// \param srcOsdf source OSDF container
/// \param osdfMetadata frame metadata for srcOsdf
void saveOsdfToOdb(const ObsDataOutParameters &dataOutParams,
                   const eckit::mpi::Comm &ioPoolComm,
                   std::unique_ptr<osdf::IFrame> &srcOsdf,
                   osdf::FrameMetadata &osdfMetadata);
}  // namespace writer
}  // namespace ioda
