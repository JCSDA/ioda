#pragma once
/*
 * (C) Copyright 2022 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \defgroup ioda_cxx_io WriterPool
 * \brief Public API for ioda::WriterPool
 * \ingroup ioda_cxx_api
 *
 * @{
 * \file WriterPool.h
 * \brief Interfaces for ioda::WriterPool and related classes.
 */

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "eckit/mpi/Comm.h"

#include "ioda/defs.h"
#include "ioda/Engines/WriterBase.h"
#include "ioda/Engines/WriterFactory.h"
#include "ioda/Group.h"
#include "ioda/ioPool/IoPoolBase.h"
#include "ioda/ioPool/IoPoolParameters.h"

#include "oops/util/DateTime.h"
#include "oops/util/parameters/Parameters.h"
#include "oops/util/parameters/OptionalParameter.h"
#include "oops/util/parameters/RequiredPolymorphicParameter.h"
#include "oops/util/Printable.h"

namespace ioda {

/// \brief Writer pool subclass
/// \details This class holds a single io pool which consists of a small number of MPI tasks.
/// The tasks assigned to an io pool object are selected from the total MPI tasks working on
/// the DA run. The tasks in the pool are used to transfer data from memory to a
/// ioda file. Only the tasks in the pool interact with the file and the remaining tasks outside
/// the pool interact with the pool tasks to get their individual pieces of the data being
/// transferred.
/// \ingroup ioda_cxx_io
class IODA_DL WriterPool : public IoPoolBase {
 public:
  /// \brief construct a WriterPool object
  /// \param ioPoolParams Parameters for this io pool
  /// \param writerParams Parameters for the associated backend writer engine
  /// \param commAll MPI "all" communicator group (all tasks in DA run)
  /// \param commTime MPI "time" communicator group (tasks in current time bin for 4DEnVar)
  /// \param winStart DA timing window start
  /// \param winEnd DA timing window end
  /// \param patchObsVec Boolean vector showing which locations belong to this MPI task
  WriterPool(const oops::Parameter<IoPoolParameters> & ioPoolParams,
             const oops::RequiredPolymorphicParameter
                 <Engines::WriterParametersBase, Engines::WriterFactory> & writerParams,
             const eckit::mpi::Comm & commAll, const eckit::mpi::Comm & commTime,
             const util::DateTime & winStart, const util::DateTime & winEnd,
             const std::vector<bool> & patchObsVec);
  ~WriterPool();

  /// \brief return reference to the patch obs vector
  const std::vector<bool> & patchObsVec() const { return patch_obs_vec_; }

  /// \brief return nlocs for this object
  int nlocs() const { return nlocs_; }

  /// \brief return the total nlocs for this rank
  int total_nlocs() const { return total_nlocs_; }

  /// \brief return the global nlocs in the pool
  int global_nlocs() const { return global_nlocs_; }

  /// \brief return the nlocs start position
  /// \details The nlocs start position refers to the position along the nlocs dimension
  /// in the output file (when writing a single output file) where this rank's data
  /// (collected from other non io pool MPI processes) goes. For example, io pool rank 0
  /// data goes at nlocs position 0 in the file. Then if io pool rank 0 data is 10 locations
  /// long, io pool rank 1 data goes in the file at nlocs position 10 and so forth. In other
  /// words, the io pool ranks are stacking their blocks of data together (in series)
  /// in the output file.
  int nlocs_start() const { return nlocs_start_; }

  /// \brief return the number of locations in the patch (ie owned) by this object
  int patch_nlocs() const { return patch_nlocs_; }

  /// \brief save obs data to output file
  /// \param srcGroup source ioda group to be saved into the output file
  void save(const Group & srcGroup);

  /// \brief finalize the io pool before destruction
  /// \detail This routine is here to do specialized clean up after the save function has been
  /// called and before the destructor is called. The primary task is to clean up the eckit
  /// split communicator groups.
  void finalize() override;

  /// \brief fill in print routine for the util::Printable base class
  void print(std::ostream & os) const override;

 private:
  /// \brief writer parameters
  const oops::RequiredPolymorphicParameter
      <Engines::WriterParametersBase, Engines::WriterFactory> & writer_params_;

  /// \brief number of locations for this rank
  std::size_t nlocs_;

  /// \brief total number of locations (sum of this rank nlocs + assigned ranks nlocs)
  std::size_t total_nlocs_;

  /// \brief global number of locations (sum of total_nlocs_ from all ranks in the io pool)
  std::size_t global_nlocs_;

  /// \brief starting point along the nlocs dimension (for single file output)
  std::size_t nlocs_start_;

  /// \brief number of patch locations for this rank
  std::size_t patch_nlocs_;

  /// \brief mulitiple files flag, true -> will be creating a set of output files
  bool create_multiple_files_;

  /// \brief patch vector for this rank
  /// \details The patch vector shows which locations are owned by this rank
  /// as opposed to locations that are duplicates of a neighboring rank. This is relavent
  /// for distributions like Halo where the halo regions can overlap.
  const std::vector<bool> & patch_obs_vec_;

  /// \brief writer engine destination for printing (eg, output file name)
  std::string writerDest_;

  /// \brief pre-/post-processor object associated with the writer engine.
  std::shared_ptr<Engines::WriterProcBase> writer_proc_;

  /// \brief group ranks into sets for the io pool assignments
  /// \detail This function will create a vector of vector of ints structure which
  /// shows how to form the io pool and how to assign the non io pool ranks to each
  /// of the ranks in the io pool.
  /// \param rankGrouping structure that maps ranks outside the pool to ranks in the pool
  void groupRanks(IoPoolGroupMap & rankGrouping) override;

  /// \brief collect nlocs from assigned ranks and compute total for this rank
  /// \detail For each of the ranks in the io pool, this function collects nlocs from
  /// all of the assigned ranks and sums up them up to get the total nlocs for each
  /// output file.
  /// \param nlocs Number of locations for this rank.
  void setTotalNlocs(const std::size_t nlocs);

  /// \brief collect information related to a single file output from all ranks in the io pool
  /// \detail This function will collect two pieces of information. The first is the sum
  /// total nlocs for all ranks in the io pool. This value represents the total amount
  /// of nlocs from all obs spaces in the all communicator group. The global nlocs value
  /// is used to properly size the variables when writing to a single output file.
  /// The second piece of information is the proper start values for each rank in regard
  /// to the nlocs dimension when writing to a single output file.
  void collectSingleFileInfo();
};

}  // namespace ioda

/// @}
