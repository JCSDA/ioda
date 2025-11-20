/*
 * (C) Copyright 2025 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#pragma once

#include <string>

namespace ioda {
  class ObsDataOutParameters;

namespace writer {

/// \brief utility to create an empty idoa output file
/// \details This function provides a convenient means for creating
/// an "empty" ioda output file (commonly called a "feedback" file).
/// \param dataOutParams obs space data output (obsdataout) parameters
void obsWriteEmptyFile(const ioda::ObsDataOutParameters & dataOutParams,
                       const std::string & locDimName);

}  // namespace writer
}  // namespace ioda
