/*
 * (C) Copyright 2025 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#pragma once

#include <memory>
#include <string>
#include <vector>

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
    class DistributionParametersBase;
    class Distribution;
    struct ObsSourceStats;

namespace reader {

/// \brief distribute obs across all of the MPI tasks
/// \param[in] distributeParams obs distribution parameters
/// \param[in] commAll mpi communicator group containing all ranks given to the obs space
/// \param[in] obsGroupVarList list of observation grouping variables for creating records
/// \param[in] osdfMetadata Metadata for the inOutOsdf container
/// \param[out] obsSourceStats struct with info about the input file
/// \param[out] ospaceDist Distribution object for the obs space
/// \param[inout] inOutOsdf OSDF container object; contains rank-specific filtered, non-distributed
///               obs on input, and distributed obs on output
void distributeObs(const DistributionParametersBase & distParams,
                   const eckit::mpi::Comm & commAll,
                   const std::vector<std::string> & obsGroupVarList,
                   const osdf::FrameMetadata & osdfMetadata,
                   ObsSourceStats & obsSourceStats,
                   std::shared_ptr<Distribution> & ospaceDist,
                   std::unique_ptr<osdf::IFrame> & inOutOsdf);

}  // namespace reader
}  // namespace ioda
