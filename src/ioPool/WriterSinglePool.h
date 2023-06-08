#pragma once
/*
 * (C) Copyright 2022 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \defgroup ioda_cxx_io WriterSinglePool
 * \brief Public API for ioda::WriterSinglePool
 * \ingroup ioda_cxx_api
 *
 * @{
 * \file WriterSinglePool.h
 * \brief Interfaces for ioda::WriterSinglePool and related classes.
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
#include "ioda/ioPool/IoPoolParameters.h"
#include "ioda/ioPool/WriterPoolBase.h"

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
class IODA_DL WriterSinglePool : public WriterPoolBase {
 public:
  /// \brief construct a WriterSinglePool object
  /// \param configParams Configuration parameters (from YAML specs) for this io pool
  /// \param createParams Parameters for the writer pool creation
  WriterSinglePool(const IoPoolParameters & configParams,
                   const WriterPoolCreationParameters & createParams);
  ~WriterSinglePool() {}

  /// \brief save obs data to output file
  /// \param srcGroup source ioda group to be saved into the output file
  void save(const Group & srcGroup) override;

  /// \brief initialize the io pool after construction
  /// \detail This routine is here to do specialized initialization up before the save
  /// function has been called and after the constructor is called.
  void initialize() override;

  /// \brief finalize the io pool before destruction
  /// \detail This routine is here to do specialized clean up after the save function has been
  /// called and before the destructor is called. The primary task is to clean up the eckit
  /// split communicator groups.
  void finalize() override;

  /// \brief fill in print routine for the util::Printable base class
  void print(std::ostream & os) const override;

 private:
  /// \brief writer engine destination for printing (eg, output file name)
  std::string writerDest_;

  /// \brief pre-/post-processor object associated with the writer engine.
  std::shared_ptr<Engines::WriterProcBase> writer_proc_;
};

}  // namespace ioda

/// @}
