#pragma once
/*
 * (C) Copyright 2022 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \defgroup ioda_cxx_io IoPoolBase
 * \brief Public API for ioda::IoPoolBase
 * \ingroup ioda_cxx_api
 *
 * @{
 * \file IoPoolBase.h
 * \brief Interfaces for ioda::IoPoolBase and related classes.
 */

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "eckit/mpi/Comm.h"

#include "ioda/defs.h"
#include "ioda/Engines/ReaderBase.h"
#include "ioda/Engines/WriterBase.h"
#include "ioda/Group.h"
#include "ioda/ioPool/IoPoolParameters.h"

#include "oops/util/DateTime.h"
#include "oops/util/parameters/Parameters.h"
#include "oops/util/parameters/OptionalParameter.h"
#include "oops/util/parameters/RequiredPolymorphicParameter.h"
#include "oops/util/Printable.h"

namespace ioda {
namespace IoPool {

//------------------------------------------------------------------------------------
// Common io pool creation parameters
//------------------------------------------------------------------------------------

class IoPoolCreationParameters {
 public:
    IoPoolCreationParameters(const eckit::mpi::Comm & commAll,
                             const eckit::mpi::Comm & commTime);
    virtual ~IoPoolCreationParameters() {}

    /// \brief io pool communicator group
    const eckit::mpi::Comm & commAll;

    /// \brief time communicator group
    const eckit::mpi::Comm & commTime;
};

//------------------------------------------------------------------------------------
// Io pool base class
//------------------------------------------------------------------------------------

/// \brief IO pool base class
/// \details This class holds a single io pool which consists of a small number of MPI tasks.
/// The tasks assigned to an io pool object are selected from the total MPI tasks working on
/// the DA run. The tasks in the pool are used to transfer data to/from memory from/to a
/// ioda file. Only the tasks in the pool interact with the file and the remaining tasks outside
/// the pool interact with the pool tasks to get their individual pieces of the data being
/// transferred.
/// \ingroup ioda_cxx_io
class IODA_DL IoPoolBase : public util::Printable {
 public:
  /// \brief construct an IoPoolBase object
  /// \param configParams io pool configuration parameters
  /// \param commAll MPI "all" communicator group (all tasks in DA run)
  /// \param commTime MPI "time" communicator group (tasks in current time bin for 4DEnVar)
  /// \param poolColor "color" value for pool members when splitting commAll communicator
  /// \param nonPoolColor "color" value for non-pool members when splitting commAll communicator
  /// \param poolCommName name for pool MPI communicator
  /// \param nonPoolCommName name for non-pool MPI communicator
  IoPoolBase(const IoPoolParameters & configParams,
             const eckit::mpi::Comm & commAll, const eckit::mpi::Comm & commTime,
             const int poolColor, const int nonPoolColor,
             const char * poolCommName, const char * nonPoolCommName);
  virtual ~IoPoolBase() {}

  /// \brief return the "all" mpi communicator
  const eckit::mpi::Comm & commAll() const { return commAll_; }

  /// \brief return the "time" mpi communicator
  const eckit::mpi::Comm & commTime() const { return commTime_; }

  /// \brief return the pool mpi communicator
  const eckit::mpi::Comm * commPool() const { return commPool_; }

  /// \brief return the rank assignment for this object.
  const std::vector<std::pair<int, int>> & rankAssignment() const { return rankAssignment_; }

  /// \brief return the pool color
  int poolColor() const { return poolColor_; }

  /// \brief return the pool color
  int nonPoolColor() const { return nonPoolColor_; }

  /// \brief return the pool color
  const char * poolCommName() const { return poolCommName_; }

  /// \brief return the pool color
  const char * nonPoolCommName() const { return nonPoolCommName_; }

  /// \brief number of locations on this MPI rank
  std::size_t nlocs() const { return nlocs_; }

  /// \brief global number of locations (total across MPI tasks)
  std::size_t globalNlocs() const { return globalNlocs_; }

  /// \brief initialize the io pool after construction
  virtual void initialize() = 0;

  /// \brief finalize the io pool before destruction
  virtual void finalize() = 0;

  /// \brief fill in print routine for the util::Printable base class
  virtual void print(std::ostream & os) const = 0;

 protected:
  typedef std::map<int, std::vector<int>> IoPoolGroupMap;

  /// \brief io pool parameters
  const IoPoolParameters & configParams_;

  /// \brief MPI communicator group for all processes
  const eckit::mpi::Comm & commAll_;

  /// \brief MPI time communicator group
  const eckit::mpi::Comm & commTime_;

  /// \brief MPI communicator group for all processes in the i/o pool
  /// \details This communicator group will hold a subset of the world communicator
  /// group. If an MPI task is not a member of the i/o pool, then this pointer will
  /// be set to nullptr to indicate that.
  eckit::mpi::Comm *commPool_;

  // These next two constants are the "color" values used for the MPI split comm command.
  // They just need to be two different numbers, which will create the pool communicator,
  // and a second communicator that holds all of the other ranks not in the pool.
  //
  // Unfortunately, the eckit interface doesn't appear to support using MPI_UNDEFINED for
  // the nonPoolColor. Ie, you need to assign all ranks into a communicator group.
  /// \brief color value for splitting the MPI communicator (in the pool)
  const int poolColor_;

  /// \brief color value for splitting the MPI communicator (not in the pool)
  const int nonPoolColor_;

  /// \brief name for splitting the MPI commnunicator (in the pool)
  const char * poolCommName_;

  /// \brief name for splitting the MPI commnunicator (not in the pool)
  const char * nonPoolCommName_;

  /// \brief parallel io flag, true -> read/write in parallel mode
  bool isParallelIo_;

  /// \brief target pool size
  int targetPoolSize_;

  /// \brief ranks in the all_comm_ group that this rank transfers data
  /// \detail Each pair in this vector contains as the first element the rank number
  /// it is assigned and as the second element the number of locations for the assigned
  /// rank. Note that the data types in the pair need to match for the eckit MPI send/recv
  /// commands.
  std::vector<std::pair<int, int>> rankAssignment_;

  /// \brief number of locations on this MPI rank
  std::size_t nlocs_;

  /// \brief global number of locations (total across MPI tasks)
  std::size_t globalNlocs_;

  /// \brief set the pool size (number of MPI processes) for this instance
  /// \detail This function sets the data member targetPoolSize_ to the minumum of
  /// the specified maximum pool size or the size of the commAll_ communicator group.
  virtual void setTargetPoolSize();

  /// \brief group ranks into sets for the io pool assignments
  /// \detail This function will create a vector of vector of ints structure which
  /// shows how to form the io pool and how to assign the non io pool ranks to each
  /// of the ranks in the io pool.
  /// \param rankGrouping structure that maps ranks outside the pool to ranks in the pool
  virtual void groupRanks(IoPoolGroupMap & rankGrouping);

  /// \brief assign ranks in the comm_all_ comm group to each of the ranks in the io pool
  /// \detail This function will dole out the ranks within the comm_all_ group, that are
  /// not in the io pool, to the ranks that are in the io pool. This sets up the send/recv
  /// communication for collecting the variable data. When finished, all ranks in the
  /// comm_all_ group will have a list of all the ranks that they send to in the comm_pool_
  /// group, and all ranks in the comm_pool_ group will have the corresponding ranks in
  /// the comm_all_ group that they receive from.
  /// \param nlocs number of locations on this MPI rank
  /// \param rankGrouping structure that maps ranks outside the pool to ranks in the pool
  virtual void assignRanksToIoPool(const std::size_t nlocs, const IoPoolGroupMap & rankGrouping);

  /// \brief create the io pool communicator group
  /// \detail This function will create the io pool communicator group using the eckit
  /// MPI split command. This function sets the comm_pool_, rank_pool_ and size_pool_
  /// data members. If this rank is not in the io pool communicator group comm_pool_ is
  /// set to nullptr, and both rank_pool_ and size_pool_ are set to -1.
  /// \param rankGrouping structure that maps ranks outside the pool to ranks in the pool
  virtual void createIoPool(IoPoolGroupMap & rankGrouping);

  /// \brief build the io pool
  /// \detail This function establishes the io pool assignments, ie which MPI ranks go
  /// into the pool and which MPI ranks get associated with the io pool ranks.
  /// The number of locations is passed into this function so that both the writer
  /// and reader can utilize it. In the case of the writer the number of patch nlocs
  /// needs to be passed in to avoid writing duplicate locations for overlapping
  /// distributions (eg Halo). The reader simply passes in the nlocs from the pool object
  /// since the number of patch nlocs gets determined downstream from calling this function.
  /// \param numLocs number of locations
  void buildIoPool(const std::size_t numLocs);
};

}  // namespace IoPool
}  // namespace ioda

/// @}
