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
}

namespace ioda {
    class Distribution;
    struct ObsSourceStats;
    namespace ObsIoPool {
        class ObsIoPool;
    }

namespace writer {

/// \brief collect all obs onto the io pool ranks
/// \param obsIoPool io pool object
/// \param ospaceDist distribution object for the caller's obs space
/// \param obsSourceStats statistics about the obs source (file)
/// \param inOutOsdf osdf object
void collectObs(const std::unique_ptr<ObsIoPool::ObsIoPool> & obsIoPool,
                std::shared_ptr<Distribution> & ospaceDist,
                ObsSourceStats & obsSourceStats,
                std::unique_ptr<osdf::IFrame> & inOutOsdf);

}  // namespace writer
}  // namespace ioda
