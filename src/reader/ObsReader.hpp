/*
 * (C) Copyright 2025 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#pragma once

#include <cstddef>
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

/// \brief renumber a freshly loaded frame's location indices so they follow on from
///        the input files already loaded, and report how many locations it holds
/// \details The netCDF reader numbers the rows it loads relative to the start of the file
/// they came from, and every io pool rank loads a block of every file, so the raw numbering
/// repeats from one input file to the next. Shifting each file's indices by the number of
/// locations in the files ahead of it makes the assembled frame number its rows by their
/// position in the concatenated input, which does not depend on how the io pool happened to
/// split the reads.
///
/// This must be called on every rank in commAll, including ranks holding an empty frame,
/// because of the collective reduction used to count the frame's locations.
///
/// \param commAll MPI communicator for all ranks
/// \param osdf frame to renumber
/// \param offset number of locations in the input files already loaded
/// \return number of locations this frame holds across all ranks in commAll
std::size_t offsetSourceLocationIndices(const eckit::mpi::Comm & commAll,
                                        const std::unique_ptr<osdf::IFrame> & osdf,
                                        const std::size_t offset);

/// \brief return the input file position of each location held in an OSDF container
/// \param osdf OSDF container object
std::vector<std::size_t> getSourceLocationIndices(const osdf::IFrame & osdf);

/// \brief set the obs source stats that describe the locations held on this rank
/// \details Sets nlocs, locIndices, recNums and nrecs from a distributed OSDF container and
/// the record numbers returned by distributeObs.
/// \param osdf OSDF container object holding this rank's locations
/// \param recNums global record number of each location in osdf
/// \param obsSourceStats statistics about the obs source (file), updated in place
void setLocalSourceStats(const osdf::IFrame & osdf, std::vector<std::size_t> recNums,
                         ObsSourceStats & obsSourceStats);

/// \brief read observation data from a NetCDF file into an OSDF
/// \param dataInParams obs space data inputs (obsdatain) parameters
/// \param ioPoolParams io pool parameters
/// \param distParams ioda Distribution parameters
/// \param commAll MPI communicator for all ranks
/// \param timeWindow time window for selecting observations
/// \param obsName obs space name (used in the log messaging)
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
             const std::string & obsName,
             std::shared_ptr<Distribution> & ospaceDist,
             std::unique_ptr<osdf::IFrame> & destOsdf,
             ObsSourceStats & obsSourceStats,
             osdf::FrameMetadata & osdfMetadata);

}  // namespace reader
}  // namespace ioda
