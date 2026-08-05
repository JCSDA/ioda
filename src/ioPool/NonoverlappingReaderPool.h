/*
 * (C) Crown Copyright 2025 Met Office UK
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#pragma once

#include "ioda/ioPool/IoPoolParameters.h"
#include "ioda/ioPool/ReaderPoolBase.h"

namespace ioda {

class Group;

namespace IoPool {

class IoPoolParameters;

/// \brief A pool of readers with intrinsic support for parallel I/O.
///
/// This class expects the constructed reader to produce a separate ObsGroup on each process. Each
/// location that survives window and QC filtering is expected to be included in exactly one of
/// these ObsGroups. The distribution of locations among processes is left entirely to the reader
/// (in particular, the reader is responsible for placing all locations belonging to a given record
/// on a single rank); the pool does not redistribute the locations in any way. Therefore this class
/// must be used in combination with the IdentityDistribution distribution type.
///
/// \ingroup ioda_cxx_io
class NonoverlappingReaderPool : public ReaderPoolBase {
 public:
  /// \brief construct a NonoverlappingReaderPool object
  /// \param configParams Parameters for this io pool
  /// \param createParams Parameters for creating the reader pool
  NonoverlappingReaderPool(const IoPoolParameters & configParams,
                           const ReaderPoolCreationParameters & createParams);
  ~NonoverlappingReaderPool() {}

  /// \brief initialize the io pool after construction
  /// \detail This routine is here to do specialized initialization before the load
  /// function has been called and after the constructor is called.
  void initialize() override;

  /// \brief load obs data from the obs source (file or generator)
  /// \param destGroup destination ioda group to be loaded from the input file
  void load(Group & destGroup) override;

  /// \brief finalize the io pool before destruction
  /// \detail This routine is here to do specialized clean up after the load function has been
  /// called and before the destructor is called. The primary task is to clean up the eckit
  /// split communicator groups.
  void finalize() override;

  /// \brief fill in print routine for the util::Printable base class
  void print(std::ostream & os) const override;

 private:
  /// \detail This function will determine the target pool size
  /// \param numMpiTasks total number of mpi tasks (typically, commAll().size())
  void setTargetPoolSize(const int numMpiTasks) override;

  /// \detail This function will create a vector of vector of ints structure which
  /// shows how to form the io pool and how to assign the non io pool ranks to each
  /// of the ranks in the io pool.
  /// \param rankGrouping structure that maps ranks outside the pool to ranks in the pool
  /// \param numMpiTasks total number of mpi tasks (typically, commAll().size())
  void groupRanks(const int numMpiTasks, IoPoolGroupMap & rankGrouping) override;

  /// \brief Copy the group structure (the list of variables, variable groups, dimensions and
  /// attributes) from the lowest rank holding at least one location to all ranks holding no
  /// locations. Do the same with the dtimeEpoch_ member variable.
  ///
  /// The aim of this operation is to prevent exceptions from being thrown later due to the absence
  /// of variables, dimensions etc. on ranks that have not been assigned with any files or parts of
  /// files to read.
  ///
  /// This function must not be called if no ranks hold any locations.
  ///
  /// \param thisRankHasLocations whether this rank holds any locations
  /// \param group group to be modified (on ranks without locations) or to serve as a model
  void copyGroupStructureFromLowestRankWithLocationsToRanksWithoutLocations(
          bool thisRankHasLocations, Group &group);

  /// \brief Copy the value of the dtimeEpoch_ member variable from the sole rank in `subcomm`
  /// holding at least one location to all other ranks.
  ///
  /// \param subcomm subcommunicator of commPool_ containing exactly one rank holding at least one
  ///                location and all ranks holding no locations
  /// \param rankWithLocations index of the sole process in `subcomm` holding at least one location
  void setDtimeEpochOnRanksWithoutLocations(const eckit::mpi::Comm &subcomm,
                                            size_t rankWithLocations);

  /// \brief Copy the group structure (the list of variables, variable groups, dimensions and
  /// attributes) from the sole rank `subcomm` holding at least one locations to all other ranks.
  ///
  /// \param subcomm subcommunicator of commPool_ containing exactly one rank holding at least one
  ///                location and all ranks holding no locations
  /// \param rankWithLocations index of the sole process in `subcomm` holding at least one location
  /// \param group group to be modified (on ranks without locations) or to serve as a model
  void setGroupStructureOnRanksWithoutLocations(const eckit::mpi::Comm &subcomm,
                                                size_t rankWithLocations, Group &group);

 private:
  /// \brief input file is empty when true
  bool emptyFile_;
};

}  // namespace IoPool
}  // namespace ioda
