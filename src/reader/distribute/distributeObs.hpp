/*
 * (C) Copyright 2025 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#pragma once

#include <cstddef>
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

namespace reader {

/// \brief distribute obs across all of the MPI tasks
/// \details Every column of the container, including sourceLocationIndices, moves with its
/// rows, so the caller can read the location indices from the distributed container.
/// \param[in] distributeParams obs distribution parameters, defining new desired distribution
/// \param[in] commAll mpi communicator group containing all ranks given to the obs space
/// \param[in] obsGroupVarList list of observation grouping variables for creating records
/// \param[inout] inOutDist contains the distribution object reflecting the current distribution of
///               the obs on input, and is set to the new distribution on output.
/// \param[inout] inOutOsdf OSDF container object; contains rank-specific filtered,
///               obs with distIn distribution on input, and distributed obs reflecting
///               distOut on output
/// \param[out] outRecNums global record number of each location in inOutOsdf on output
void distributeObs(const DistributionParametersBase & distParams,
                   const eckit::mpi::Comm & commAll,
                   const std::vector<std::string> & obsGroupVarList,
                   std::shared_ptr<Distribution> & inOutDist,
                   std::unique_ptr<osdf::IFrame> & inOutOsdf,
                   std::vector<std::size_t> & outRecNums);

/// \brief distribute obs across all of the MPI tasks
/// \param[in] distributeParams obs distribution parameters, defining new desired distribution
/// \param[in] commAll mpi communicator group containing all ranks given to the obs space
/// \param[in] obsGroupVarList list of observation grouping variables for creating records
/// \param[in] inDist contains the distribution object reflecting the current distribution of
///            the obs on input.
/// \param[in] inOsdf OSDF container object; contains rank-specific filtered,
///            obs with distIn distribution.
/// \param[out] outDist contains the output distribution object, created using distributeParams.
/// \param[out] outOsdf OSDF container object; contains distributed obs reflecting distout
/// \param[out] outRecNums global record number of each location in outOsdf
void distributeObs(const DistributionParametersBase & distParams,
                   const eckit::mpi::Comm & commAll,
                   const std::vector<std::string> & obsGroupVarList,
                   std::shared_ptr<Distribution> & inDist,
                   std::unique_ptr<osdf::IFrame> & inOsdf,
                   std::shared_ptr<Distribution> & outDist,
                   std::unique_ptr<osdf::IFrame> & outOsdf,
                   std::vector<std::size_t> & outRecNums);

}  // namespace reader
}  // namespace ioda
