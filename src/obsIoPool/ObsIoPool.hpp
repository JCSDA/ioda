/*
 * (C) Copyright 2025 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#pragma once

/// \file ObsIoPool.h
/// \brief Interfaces for ioda::ObsIoPool and related classes.

#include "eckit/mpi/Comm.h"

#include "ioda/ioPool/IoPoolParameters.h"

namespace ioda {
namespace ObsIoPool {

/// \brief Obs IO pool class
/// \details This class holds a single io pool which consists of a small number of MPI tasks.
/// The tasks assigned to an io pool object are selected from the total MPI tasks working on
/// the DA run. The tasks in the pool are used to transfer data to/from memory from/to a
/// ioda file. Only the tasks in the pool interact with the file and the remaining tasks outside
/// the pool transfer any data during the downstream distribute task.
/// \ingroup ioda_cxx_io
class ObsIoPool : public util::Printable {
 public:
  /// \brief construct an ObsIoPool object
  /// \param configParams io pool configuration parameters
  /// \param commAll MPI "all" communicator group (all tasks in DA run)
  ObsIoPool(const ioda::IoPool::IoPoolParameters & configParams, const eckit::mpi::Comm & commAll);
  ~ObsIoPool();

  /// \brief return the "all" mpi communicator
  const eckit::mpi::Comm & commAll() const { return commAll_; }

  /// \brief return the "pool" mpi communicator
  const eckit::mpi::Comm & commPool() const { return *commPool_; }

  /// \brief flag indicating if this rank is in the io pool
  bool inIoPool() const { return inIoPool_; }

  /// \brief return the number of ranks in the io pool
  /// \details This is valid on every rank, including the ranks outside the pool.
  ///          Note that commPool().size() is the number of ranks outside the
  ///          pool on the ranks outside the pool.
  int poolSize() const { return poolSize_; }

  /// \brief print the ObsIoPool object
  /// \param os output stream
  void print(std::ostream & os) const override;

 private:
  /// \brief io pool parameters
  const ioda::IoPool::IoPoolParameters & configParams_;

  /// \brief MPI communicator group for all processes
  const eckit::mpi::Comm & commAll_;

  /// \brief MPI communicator group for all processes in the i/o pool
  /// \details This communicator group will hold the results of splitting the commAll_
  /// communicator group into two groups, one for the i/o pool and one for all the other
  /// ranks not in the pool.
  eckit::mpi::Comm * commPool_;

  /// \brief flag indicating if this rank is in the io pool
  bool inIoPool_;

  /// \brief number of ranks in the io pool
  /// \details This is set on all ranks, both inside and outside the pool.
  int poolSize_;
};

}  // namespace ObsIoPool
}  // namespace ioda

/// @}
