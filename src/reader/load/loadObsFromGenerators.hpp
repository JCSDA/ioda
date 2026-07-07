/*
 * (C) Copyright 2026 UCAR
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
namespace util {
    class TimeWindow;
}
namespace ioda {
    class ObsDataInParameters;
}

namespace ioda {
namespace reader {

/// \brief Generate observation data from an explicit list of locations into an OSDF container.
///
/// \details OSDF counterpart to the engines-based GenList reader. Validation and value
/// selection are shared with the engines path via Engines::generateObsList; the generated
/// data is stored into the frame by storeGenDataInFrame.
///
/// \param dataInParams parameters for obsdatain configuration (engine type must be "GenList")
/// \param obsVarNames observation variable names (from the obs space)
/// \param destOsdf destination OSDF container
/// \param osdfMetadata frame metadata for the destination OSDF
void loadOsdfFromGenList(const ObsDataInParameters & dataInParams,
                         const std::vector<std::string> & obsVarNames,
                         std::unique_ptr<osdf::IFrame> & destOsdf,
                         osdf::FrameMetadata & osdfMetadata);

/// \brief Generate observation data at random locations into an OSDF container.
///
/// \details OSDF counterpart to the engines-based GenRandom reader. Validation and random
/// generation are shared with the engines path via Engines::generateObsRandom; the generated
/// data is stored into the frame by storeGenDataInFrame.
///
/// \param dataInParams parameters for obsdatain configuration (engine type must be "GenRandom")
/// \param obsVarNames observation variable names (from the obs space)
/// \param timeWindow assimilation time window the generated datetimes must fall within
/// \param destOsdf destination OSDF container
/// \param osdfMetadata frame metadata for the destination OSDF
void loadOsdfFromGenRandom(const ObsDataInParameters & dataInParams,
                           const std::vector<std::string> & obsVarNames,
                           const util::TimeWindow & timeWindow,
                           std::unique_ptr<osdf::IFrame> & destOsdf,
                           osdf::FrameMetadata & osdfMetadata);

}  // namespace reader
}  // namespace ioda
