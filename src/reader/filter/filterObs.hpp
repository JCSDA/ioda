/*
 * (C) Copyright 2025 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#pragma once

#include <memory>

// Forward class declarations
namespace eckit {
    namespace mpi {
        class Comm;
    }
}
namespace osdf {
    class IFrame;
    class FrameMetadata;
}
namespace util {
    class TimeWindow;
}

namespace ioda {
    struct ObsSourceStats;
namespace reader {

/// \brief remove locations that fail checks
/// \details This function will apply two checks to determine which locations
/// to keep.
///   1. reject locations that have missing values in either of latitude, longitude
///      or dateTime
///   2. reject locations that fall outside the given time window
/// \param timeWindow time window object
/// \param commAll mpi communicator for all the ranks in the obs space communicator
/// \param obsSourceStats struct with info about the input file
/// \param osdfCont OSDF container object
/// \param osdfMetadata frame metadata for dest OSDF
void filterObs(const util::TimeWindow & timeWindow,
               const eckit::mpi::Comm & commAll,
               ObsSourceStats & obsSourceStats,
               std::unique_ptr<osdf::IFrame> & osdfCont,
               osdf::FrameMetadata & osdfMetadata);

}  // namespace reader
}  // namespace ioda
