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
 * \file WriterPoolBase.h
 * \brief Interfaces for ioda::WriterPoolBase and related classes.
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

//------------------------------------------------------------------------------------
// Writer pool creation parameters
//------------------------------------------------------------------------------------

class WriterPoolCreationParameters : public IoPoolCreationParameters {
 public:
    WriterPoolCreationParameters(
            const eckit::mpi::Comm & commAll, const eckit::mpi::Comm & commTime,
            const oops::RequiredPolymorphicParameter
                <Engines::WriterParametersBase, Engines::WriterFactory> & writerParams,
            const std::vector<bool> & patchObsVec);
    virtual ~WriterPoolCreationParameters() {}

    /// \brief parameters to be sent to the writer engine factory
    const oops::RequiredPolymorphicParameter
                <Engines::WriterParametersBase, Engines::WriterFactory> & writerParams;

    /// \brief patch vector for identifying "ownership" of locations by each MPI task
    const std::vector<bool> & patchObsVec;
};

//------------------------------------------------------------------------------------
// Writer pool base class
//------------------------------------------------------------------------------------

/// \brief Writer pool subclass
/// \details This class holds a single io pool which consists of a small number of MPI tasks.
/// The tasks assigned to an io pool object are selected from the total MPI tasks working on
/// the DA run. The tasks in the pool are used to transfer data from memory to a
/// ioda file. Only the tasks in the pool interact with the file and the remaining tasks outside
/// the pool interact with the pool tasks to get their individual pieces of the data being
/// transferred.
/// \ingroup ioda_cxx_io
class IODA_DL WriterPoolBase : public IoPoolBase {
 public:
  WriterPoolBase(const IoPoolParameters & configParams,
                 const WriterPoolCreationParameters & createParams);
  virtual ~WriterPoolBase() {}

  /// \brief vector showing ownership of locations for this MPI task
  const std::vector<bool> & patchObsVec() const { return patchObsVec_; }

  /// \brief total number of locations (sum of this rank nlocs + assigned ranks nlocs)
  std::size_t totalNlocs() const { return totalNlocs_; }

  /// \brief starting point along the nlocs dimension (for single file output)
  std::size_t nlocsStart() const { return nlocsStart_; }

  /// \brief number of locations "owned" by this MPI task
  std::size_t patchNlocs() const { return patchNlocs_; }

  /// \brief save obs data to output file
  /// \param srcGroup source ioda group to be saved into the output file
  virtual void save(const Group & srcGroup) = 0;

 protected:
  /// \brief writer engine parameters
  const oops::RequiredPolymorphicParameter
              <Engines::WriterParametersBase, Engines::WriterFactory> & writerParams_;

  /// \brief vector showing ownership of locations for this MPI task
  const std::vector<bool> & patchObsVec_;

  /// \brief total number of locations (sum of this rank nlocs + assigned ranks nlocs)
  std::size_t totalNlocs_;

  /// \brief starting point along the nlocs dimension (for single file output)
  std::size_t nlocsStart_;

  /// \brief number of locations "owned" by this MPI task
  std::size_t patchNlocs_;

  /// \brief when true we are creating multiple files (one per rank in the io pool)
  bool createMultipleFiles_;

  /// output file.
  /// \param nlocs Number of locations for this rank.
  virtual void setTotalNlocs(const std::size_t nlocs);

  /// \brief collect information related to a single file output from all ranks in the io pool
  /// \detail This function will collect two pieces of information. The first is the sum
  /// total nlocs for all ranks in the io pool. This value represents the total amount
  /// of nlocs from all obs spaces in the all communicator group. The global nlocs value
  /// is used to properly size the variables when writing to a single output file.
  /// The second piece of information is the proper start values for each rank in regard
  /// to the nlocs dimension when writing to a single output file.
  virtual void collectSingleFileInfo();
};

}  // namespace ioda

/// @}
