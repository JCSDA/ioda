/*
 * (C) Copyright 2025 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#pragma once

#include <cstddef>
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
namespace reader {

/// \brief global counts of the locations removed by filterObs
struct FilterCounts {
  /// \brief number of locations that fell outside the time window
  std::size_t gNlocsOutsideTimewindow = 0;

  /// \brief number of locations rejected for missing or invalid date/time or lat/lon
  std::size_t gNlocsRejectQc = 0;
};

/// \brief remove locations that fail checks
/// \details This function will apply two checks to determine which locations
/// to keep.
///   1. reject locations that have missing values in either of latitude, longitude
///      or dateTime
///   2. reject locations that fall outside the given time window
/// Both checks are governed by applyLocationChecks, which is a flag that allows them
/// to be switched off, intended for testing purposes.
/// \param timeWindow time window object
/// \param commAll mpi communicator for all the ranks in the obs space communicator
/// \param osdfCont OSDF container object
/// \param osdfMetadata frame metadata for dest OSDF
/// \param applyLocationChecks when false, keep every location and skip both checks
/// \return global counts of the locations removed by each check
FilterCounts filterObs(const util::TimeWindow & timeWindow,
                       const eckit::mpi::Comm & commAll,
                       std::unique_ptr<osdf::IFrame> & osdfCont,
                       osdf::FrameMetadata & osdfMetadata,
                       const bool applyLocationChecks = true);

}  // namespace reader
}  // namespace ioda
