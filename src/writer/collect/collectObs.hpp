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
}

namespace ioda {
    class Distribution;
    namespace ObsIoPool {
        class ObsIoPool;
    }

namespace writer {

/// \brief collect all obs onto the io pool ranks
/// \details The locations of each record are collected onto a single
/// io pool rank, in their original order.
/// \param obsIoPool io pool object
/// \param obsGroupVarList obs grouping variables (MetaData group) for forming records
/// \param ospaceDist distribution object for the caller's obs space
/// \param inOutOsdf osdf object
void collectObs(const std::unique_ptr<ObsIoPool::ObsIoPool> & obsIoPool,
                const std::vector<std::string> & obsGroupVarList,
                std::shared_ptr<Distribution> & ospaceDist,
                std::unique_ptr<osdf::IFrame> & inOutOsdf);

/// \brief collect all obs onto the io pool ranks, preserving the input variables
/// \details Records are kept intact in the same way as the overload above.
/// \param obsIoPool io pool object
/// \param obsGroupVarList obs grouping variables (MetaData group) for forming records.
/// \param inOspaceDist input distribution object for the caller's obs space
/// \param inOsdf input osdf object
/// \param outOspaceDist output distribution object for the caller's obs space
/// \param outOsdf output osdf object
void collectObs(const std::unique_ptr<ObsIoPool::ObsIoPool> & obsIoPool,
                const std::vector<std::string> & obsGroupVarList,
                std::shared_ptr<Distribution> & inOspaceDist,
                std::unique_ptr<osdf::IFrame> & inOsdf,
                std::shared_ptr<Distribution> & outOspaceDist,
                std::unique_ptr<osdf::IFrame> & outOsdf);

}  // namespace writer
}  // namespace ioda
