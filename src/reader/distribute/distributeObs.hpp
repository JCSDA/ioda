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
/// \param[in] distributeParams obs distribution parameters, defining new desired distribution
/// \param[in] commAll mpi communicator group containing all ranks given to the obs space
/// \param[in] obsGroupVarList list of observation grouping variables for creating records
/// \param[in] osdfMetadata Metadata for the inOutOsdf container
/// \param[inout] inOutObsSourceStats struct with metadata about the input file and rank's data
/// \param[inout] inOutDist contains the distribution object reflecting the current distribution of
///               the obs on input, and is set to the new distribution on output.
/// \param[inout] inOutOsdf OSDF container object; contains rank-specific filtered,
///               obs with distIn distribution on input, and distributed obs reflecting
///               distOut on output
void distributeObs(const DistributionParametersBase & distParams,
                   const eckit::mpi::Comm & commAll,
                   const std::vector<std::string> & obsGroupVarList,
                   ObsSourceStats & inOutObsSourceStats,
                   std::shared_ptr<Distribution> & inOutDist,
                   std::unique_ptr<osdf::IFrame> & inOutOsdf);

/// \brief distribute obs across all of the MPI tasks
/// \param[in] distributeParams obs distribution parameters, defining new desired distribution
/// \param[in] commAll mpi communicator group containing all ranks given to the obs space
/// \param[in] obsGroupVarList list of observation grouping variables for creating records
/// \param[in] osdfMetadata Metadata for the inOutOsdf container
/// \param[in] inObsSourceStats struct with metadata about the input file and input rank's data
/// \param[in] inDist contains the distribution object reflecting the current distribution of
///            the obs on input.
/// \param[in] inOsdf OSDF container object; contains rank-specific filtered,
///            obs with distIn distribution.
/// \param[out] outObsSourceStats struct with metadata about the input file and output rank's data
/// \param[out] outDist contains the output distribution object, created using distributeParams.
/// \param[out] outOsdf OSDF container object; contains distributed obs reflecting distout
void distributeObs(const DistributionParametersBase & distParams,
                   const eckit::mpi::Comm & commAll,
                   const std::vector<std::string> & obsGroupVarList,
                   ObsSourceStats & inObsSourceStats,
                   std::shared_ptr<Distribution> & inDist,
                   std::unique_ptr<osdf::IFrame> & inOsdf,
                   ObsSourceStats & outObsSourceStats,
                   std::shared_ptr<Distribution> & outDist,
                   std::unique_ptr<osdf::IFrame> & outOsdf);

}  // namespace reader
}  // namespace ioda
