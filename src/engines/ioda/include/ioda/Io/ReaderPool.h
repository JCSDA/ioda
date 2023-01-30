#pragma once
/*
 * (C) Copyright 2022 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \defgroup ioda_cxx_io ReaderPool
 * \brief Public API for ioda::ReaderPool
 * \ingroup ioda_cxx_api
 *
 * @{
 * \file IoPoolBase.h
 * \brief Interfaces for ioda::ReaderPool and related classes.
 */

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "eckit/mpi/Comm.h"

#include "ioda/defs.h"
#include "ioda/Engines/ReaderBase.h"
#include "ioda/Group.h"
#include "ioda/Io/IoPoolBase.h"
#include "ioda/Io/IoPoolParameters.h"

#include "oops/util/DateTime.h"
#include "oops/util/parameters/Parameters.h"
#include "oops/util/parameters/OptionalParameter.h"
#include "oops/util/parameters/RequiredPolymorphicParameter.h"
#include "oops/util/Printable.h"

namespace ioda {

/// \brief Reader pool subclass
/// \details This class holds a single io pool which consists of a small number of MPI tasks.
/// The tasks assigned to an io pool object are selected from the total MPI tasks working on
/// the DA run. The tasks in the pool are used to transfer data from memory to a
/// ioda file. Only the tasks in the pool interact with the file and the remaining tasks outside
/// the pool interact with the pool tasks to get their individual pieces of the data being
/// transferred.
/// \ingroup ioda_cxx_io
class IODA_DL ReaderPool : public IoPoolBase {
public:
  /// \brief construct a ReaderPool object
  /// \param ioPoolParams Parameters for this io pool
  /// \param readerParams Parameters for the associated backend reader engine
  /// \param commAll MPI "all" communicator group (all tasks in DA run)
  /// \param commTime MPI "time" communicator group (tasks in current time bin for 4DEnVar)
  /// \param winStart DA timing window start
  /// \param winEnd DA timing window end
  ReaderPool(const oops::Parameter<IoPoolParameters> & ioPoolParams,
             const oops::RequiredPolymorphicParameter
                 <Engines::ReaderParametersBase, Engines::ReaderFactory> & readerParams,
             const eckit::mpi::Comm & commAll, const eckit::mpi::Comm & commTime,
             const util::DateTime & winStart, const util::DateTime & winEnd,
             const std::vector<std::string> & obsVarNames);
  ~ReaderPool();

  /// \brief save obs data to output file
  /// \param destGroup destination ioda group to be loaded from the input file
  void load(Group & destGroup);

  /// \brief finalize the io pool before destruction
  /// \detail This routine is here to do specialized clean up after the load function has been
  /// called and before the destructor is called. The primary task is to clean up the eckit
  /// split communicator groups.
  void finalize() override;

  /// \brief fill in print routine for the util::Printable base class
  void print(std::ostream & os) const override;

private:
  /// \brief reader parameters
  const oops::RequiredPolymorphicParameter
      <Engines::ReaderParametersBase, Engines::ReaderFactory> & reader_params_;

  /// \brief reader engine destination for printing (eg, input file name)
  std::string readerDest_;

  /// \brief list of variable to be simulated (for the generator backends)
  const std::vector<std::string> & obs_var_names_;

  /// \brief group ranks into sets for the io pool assignments
  /// \detail This function will create a vector of vector of ints structure which
  /// shows how to form the io pool and how to assign the non io pool ranks to each
  /// of the ranks in the io pool.
  /// \param rankGrouping structure that maps ranks outside the pool to ranks in the pool
  void groupRanks(IoPoolGroupMap & rankGrouping) override;

  /// \brief assign ranks in the comm_all_ comm group to each of the ranks in the io pool
  /// \detail This function will dole out the ranks within the comm_all_ group, that are
  /// not in the io pool, to the ranks that are in the io pool. This sets up the send/recv
  /// communication for collecting the variable data. When finished, all ranks in the
  /// comm_all_ group will have a list of all the ranks that the send to or receive from.
  /// \param nlocs number of locations on this MPI rank
  /// \param rankGrouping structure that maps ranks outside the pool to ranks in the pool
  void assignRanksToIoPool(const std::size_t nlocs,
                           const IoPoolGroupMap & rankGrouping) override;
};

}  // namespace ioda

/// @}
