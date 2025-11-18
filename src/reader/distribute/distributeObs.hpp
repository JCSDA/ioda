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
/// \param distributeParams obs distribution parameters
/// \param commAll mpi communicator group containing all ranks given to the obs space
/// \param obsSourceStats struct with info about the input file
/// \param ospaceDist Distribution object for the obs space
/// \param osdfCont OSDF container object
void distributeObs(const DistributionParametersBase & distParams,
                   const eckit::mpi::Comm & commAll,
                   ObsSourceStats & obsSourceStats,
                   std::shared_ptr<Distribution> & ospaceDist,
                   std::unique_ptr<osdf::IFrame> & osdfCont);

}  // namespace reader
}  // namespace ioda
