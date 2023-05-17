/*
 * (C) Copyright 2022 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#pragma once

/*! \defgroup ioda_cxx_io NewReaderPool
 * \brief Public API for ioda::NewReaderPool
 * \ingroup ioda_cxx_api
 *
 * @{
 * \file IoPoolBase.h
 * \brief Interfaces for ioda::NewReaderPool and related classes.
 */

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "eckit/mpi/Comm.h"

#include "ioda/defs.h"
#include "ioda/Engines/ReaderBase.h"
#include "ioda/Group.h"
#include "ioda/ioPool/IoPoolBase.h"
#include "ioda/ioPool/IoPoolParameters.h"

#include "oops/util/DateTime.h"
#include "oops/util/parameters/Parameters.h"
#include "oops/util/parameters/OptionalParameter.h"
#include "oops/util/parameters/RequiredPolymorphicParameter.h"
#include "oops/util/Printable.h"

namespace ioda {

class Distribution;
enum class DateTimeFormat;

/// \brief Reader pool subclass
/// \details This class holds a single io pool which consists of a small number of MPI tasks.
/// The tasks assigned to an io pool object are selected from the total MPI tasks working on
/// the DA run. The tasks in the pool are used to transfer data from a ioda file to memory.
/// Only the tasks in the pool interact with the file and the remaining tasks outside
/// the pool interact with the pool tasks to get their individual pieces of the data being
/// transferred.
/// \ingroup ioda_cxx_io
class NewReaderPool : public IoPoolBase {
 public:
  /// \brief construct a NewReaderPool object
  /// \param ioPoolParams Parameters for this io pool
  /// \param readerParams Parameters for the associated backend reader engine
  /// \param commAll MPI "all" communicator group (all tasks in DA run)
  /// \param commTime MPI "time" communicator group (tasks in current time bin for 4DEnVar)
  /// \param winStart DA timing window start
  /// \param winEnd DA timing window end
  /// \param obsVarNames list of simulated variables from YAML specs
  /// "obs space.simulated variables" or "obs space.observed variables"
  /// \param distribution ioda::Distribution object
  /// \param obsGroupVarList list of variables names for obs grouping, from YAML spec
  /// "obs space.obsdatain.obsgrouping.group variables"
  NewReaderPool(const oops::Parameter<IoPoolParameters> & ioPoolParams,
                const oops::RequiredPolymorphicParameter
                    <Engines::ReaderParametersBase, Engines::ReaderFactory> & readerParams,
                const eckit::mpi::Comm & commAll, const eckit::mpi::Comm & commTime,
                const util::DateTime & winStart, const util::DateTime & winEnd,
                const std::vector<std::string> & obsVarNames,
                const std::shared_ptr<Distribution> & distribution,
                const std::vector<std::string> & obsGroupVarList);
  ~NewReaderPool();

  /// \brief return total number of locations from obs source
  std::size_t sourceNlocs() const {return source_nlocs_;}

  /// \brief initialize the io pool after construction
  /// \detail This routine is here to do specialized initialization before the load
  /// function has been called and after the constructor is called.
  void initialize() override;

  /// \brief load obs data from the obs source (file or generator)
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

  /// \brief reader engine source for printing (eg, input file name)
  std::string readerSrc_;

  /// \brief total number of locations in obs source (file or generator)
  std::size_t source_nlocs_;

  /// \brief group ranks into sets for the io pool assignments
  /// \detail This function will create a vector of vector of ints structure which
  /// shows how to form the io pool and how to assign the non io pool ranks to each
  /// of the ranks in the io pool.
  /// \param rankGrouping structure that maps ranks outside the pool to ranks in the pool
  void groupRanks(IoPoolGroupMap & rankGrouping) override;
};

}  // namespace ioda

/// @}
