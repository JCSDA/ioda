/*
 * (C) Copyright 2026 UCAR
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
}
namespace ioda {
    namespace Engines {
        struct GeneratedObsData;
    }
}

namespace ioda {
namespace reader {

/// \brief Store generated observation data into an OSDF container.
///
/// \details Container-side counterpart to the engine-side storeGenData (which targets an
/// ObsGroup). Both consume the same Engines::GeneratedObsData produced by the shared
/// generation routines, keeping the GenList/GenRandom data layout identical between the
/// engines-based reader and the OSDF-based reader. All generated columns are dimensioned
/// by Location only.
///
/// \param data generated observation data (locations, datetimes, obs values/errors)
/// \param destOsdf destination OSDF container
/// \param osdfMetadata frame metadata for the destination OSDF
void storeGenDataInFrame(const Engines::GeneratedObsData & data,
                         std::unique_ptr<osdf::IFrame> & destOsdf,
                         osdf::FrameMetadata & osdfMetadata);

}  // namespace reader
}  // namespace ioda
