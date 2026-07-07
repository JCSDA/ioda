/*
 * (C) Copyright 2025 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#pragma once

#include <memory>
#include <string>
#include <vector>

#include "eckit/config/Configuration.h"

namespace util {
  class TimeWindow;
}

namespace eckit {
  namespace mpi {
    class Comm;
  }
}

namespace osdf {
  class IFrame;
  class FrameMetadata;
}

namespace ioda {
  class DistributionParametersBase;
  class Distribution;
  class ObsDataInParameters;
  struct ObsSourceStats;
  namespace IoPool {
    class IoPoolParameters;
  }
  namespace ObsIoPool {
    class ObsIoPool;
  }

namespace reader {

/// \brief read observation data from a NetCDF file into an OSDF
/// \param dataInParams obs space data inputs (obsdatain) parameters
/// \param ioPoolParams io pool parameters
/// \param distParams ioda Distribution parameters
/// \param commAll MPI communicator for all ranks
/// \param timeWindow time window for selecting observations
/// \param ospaceDist distribution object for the caller's obs space
/// \param destOsdf destination OSDF to be populated
/// \param obsSourceStats statistics about the obs source (file)
/// \param osdfMetadata frame metadata object for the caller's obs space
void obsRead(const std::vector<eckit::LocalConfiguration>& dataInParams,
             const ioda::IoPool::IoPoolParameters & ioPoolParams,
             const ioda::DistributionParametersBase & distParams,
             const eckit::mpi::Comm & commAll,
             const std::vector<std::string> & obsVarNames,
             const util::TimeWindow & timeWindow,
             std::shared_ptr<Distribution> & ospaceDist,
             std::unique_ptr<osdf::IFrame> & destOsdf,
             ObsSourceStats & obsSourceStats,
             osdf::FrameMetadata & osdfMetadata);

}  // namespace reader
}  // namespace ioda
