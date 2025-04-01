/*
 * (C) Copyright 2025 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include <memory>

// Forward class declarations
namespace osdf {
    class IFrame;
}
namespace util {
    class TimeWindow;
}

namespace ioda {
namespace reader {

/// \brief remove locations that fail checks
/// \details This function will apply two checks to determine which locations
/// to keep.
///   1. reject locations that have missing values in either of latitude, longitude
///      or dateTime
///   2. reject locations that fall outside the given time window
/// \param timeWindow time window object
/// \param container OSDF container object
void filterObsContainer(const util::TimeWindow & timeWindow,
                        std::unique_ptr<osdf::IFrame> & osdfCont);

}  // namespace reader
}  // namespace ioda
