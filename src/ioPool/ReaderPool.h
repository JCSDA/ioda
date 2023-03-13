/*
 * (C) Copyright 2022 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#pragma once

/*! \defgroup ioda_cxx_io ReaderPool
 * \brief Public API for ioda::ReaderPool
 * \ingroup ioda_cxx_api
 *
 * @{
 * \file IoPoolBase.h
 * \brief Interfaces for ioda::ReaderPool and related classes.
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
class ReaderPool : public IoPoolBase {
 public:
  /// \brief construct a ReaderPool object
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
  ReaderPool(const oops::Parameter<IoPoolParameters> & ioPoolParams,
             const oops::RequiredPolymorphicParameter
                 <Engines::ReaderParametersBase, Engines::ReaderFactory> & readerParams,
             const eckit::mpi::Comm & commAll, const eckit::mpi::Comm & commTime,
             const util::DateTime & winStart, const util::DateTime & winEnd,
             const std::vector<std::string> & obsVarNames,
             const std::shared_ptr<Distribution> & distribution,
             const std::vector<std::string> & obsGroupVarList);
  ~ReaderPool();

  /// \brief load obs data from the obs source (file or generator)
  /// \param destGroup destination ioda group to be loaded from the input file
  void load(Group & destGroup);

  /// \brief finalize the io pool before destruction
  /// \detail This routine is here to do specialized clean up after the load function has been
  /// called and before the destructor is called. The primary task is to clean up the eckit
  /// split communicator groups.
  void finalize() override;

  /// \brief return the total number of locations in the obs source that are inside the
  /// timing window
  std::size_t globalNumLocs() const {return global_nlocs_;}

  /// \brief return number of locations from obs source that were outside the time window
  std::size_t globalNumLocsOutsideTimeWindow() const {return source_nlocs_outside_timewindow_;}

  /// \brief return number of locations from obs source that were rejected by the QC checks
  std::size_t globalNumLocsRejectQC() const {return source_nlocs_reject_qc_;}

  /// \brief return total number of locations from obs source
  std::size_t sourceNlocs() const {return source_nlocs_;}

  /// \brief return the number of locations on this MPI rank
  std::size_t nlocs() const {return nlocs_;}

  /// \brief return the number of records on this MPI rank
  std::size_t nrecs() const {return nrecs_;}

  /// \brief return list of indices indicating which locations were selected from ObsIo
  std::vector<std::size_t> index() const {return loc_indices_;}

  /// \brief return list of record numbers
  std::vector<std::size_t> recnums() const {return recnums_;}

  /// \brief return the char pointer for the JEDI missing value for a string variable
  std::shared_ptr<std::string> stringMissingValue() const {return jedi_missing_value_string_;}

  /// \brief fill in print routine for the util::Printable base class
  void print(std::ostream & os) const override;

 private:
  /// \brief reader parameters
  const oops::RequiredPolymorphicParameter
      <Engines::ReaderParametersBase, Engines::ReaderFactory> & reader_params_;

  /// \brief reader engine source for printing (eg, input file name)
  std::string readerSrc_;

  /// \brief MPI distribution object
  std::shared_ptr<Distribution> dist_;

  /// \brief list of variables to be simulated (for the generator backends)
  const std::vector<std::string> & obs_var_names_;

  /// \brief list of variables for the obs grouping feature
  const std::vector<std::string> & obs_group_var_list_;

  /// \brief total number of locations in obs source (file or generator)
  std::size_t source_nlocs_;

  /// \brief number of locations in obs source (file or generator)
  std::size_t source_nlocs_inside_timewindow_;

  /// \brief number of nlocs from the obs source that are outside the time window
  std::size_t source_nlocs_outside_timewindow_;

  /// \brief number of nlocs from the obs source that got rejected by the QC checks
  std::size_t source_nlocs_reject_qc_;

  /// \brief total number of locations selected from the obs source (before MPI distribution)
  std::size_t global_nlocs_;

  /// \brief number of locations on this MPI process
  std::size_t nlocs_;

  /// \brief number of records (ie, unique record numbers) on this MPI process
  std::size_t nrecs_;

  /// \brief location indices on this MPI process
  std::vector<std::size_t> loc_indices_;

  /// \brief assigned record numbers for indices in local_loc_indices_
  std::vector<std::size_t> recnums_;

  /// \brief copy of the jedi missing value for string variables
  /// \detail This is being used so that a char * pointing to the jedi missing value
  /// can be used by the ioda reader for replacing input fill values with this missing
  /// value. Note that the input string variable presents itself to the reader as a
  /// vector of char * (pointing to the strings) instead of a vector of strings.
  /// (The hdf5 library does it this way to be compatible with C).
  std::shared_ptr<std::string> jedi_missing_value_string_;

  /// \brief group ranks into sets for the io pool assignments
  /// \detail This function will create a vector of vector of ints structure which
  /// shows how to form the io pool and how to assign the non io pool ranks to each
  /// of the ranks in the io pool.
  /// \param rankGrouping structure that maps ranks outside the pool to ranks in the pool
  void groupRanks(IoPoolGroupMap & rankGrouping) override;
};

}  // namespace ioda

/// @}
