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

//------------------------------------------------------------------------------------
// Reader pool creation parameters
//------------------------------------------------------------------------------------

class ReaderPoolCreationParameters : public IoPoolCreationParameters {
 public:
    ReaderPoolCreationParameters(
            const eckit::mpi::Comm & commAll, const eckit::mpi::Comm & commTime,
            const oops::RequiredPolymorphicParameter
                <Engines::ReaderParametersBase, Engines::ReaderFactory> & readerParams,
            const util::DateTime & winStart, const util::DateTime & winEnd,
            const std::vector<std::string> & obsVarNames,
            const std::shared_ptr<Distribution> & distribution,
            const std::vector<std::string> & obsGroupVarList);
    virtual ~ReaderPoolCreationParameters() {}

    /// \brief parameters to be sent to the reader engine factory
    const oops::RequiredPolymorphicParameter
                <Engines::ReaderParametersBase, Engines::ReaderFactory> & readerParams;

    /// \brief time window start
    const util::DateTime & winStart;

    /// \brief time window end
    const util::DateTime & winEnd;

    /// \brief list of variables being assimilated (used by the generator engines)
    const std::vector<std::string> & obsVarNames;

    /// \brief ioda Distribution object associated with the ObsSpace object
    const std::shared_ptr<Distribution> & distribution;

    /// \brief list of variables used for the obs grouping function
    const std::vector<std::string> & obsGroupVarList;
};

//------------------------------------------------------------------------------------
// Reader pool base class
//------------------------------------------------------------------------------------

/// \brief Reader pool subclass
/// \details This class holds a single io pool which consists of a small number of MPI tasks.
/// The tasks assigned to an io pool object are selected from the total MPI tasks working on
/// the DA run. The tasks in the pool are used to transfer data from a ioda file to memory.
/// Only the tasks in the pool interact with the file and the remaining tasks outside
/// the pool interact with the pool tasks to get their individual pieces of the data being
/// transferred.
/// \ingroup ioda_cxx_io
class ReaderPoolBase : public IoPoolBase {
 public:
  ReaderPoolBase(const IoPoolParameters & configParams,
                 const ReaderPoolCreationParameters & createParams);
  virtual ~ReaderPoolBase() {}

  /// \brief time window start
  const util::DateTime & winStart() const { return winStart_; }

  /// \brief time window end
  const util::DateTime & winEnd() const { return winEnd_; }

  /// \brief list of variables being assimilated (used by the generator engines)
  const std::vector<std::string> & obsVarNames() const { return obsVarNames_; }

  /// \brief ioda Distribution object associated with the ObsSpace object
  const std::shared_ptr<Distribution> & distribution() const { return distribution_; }

  /// \brief list of variables used for the obs grouping function
  const std::vector<std::string> & obsGroupVarList() const { return obsGroupVarList_; }

  /// \brief missing value for string variables
  std::shared_ptr<std::string> stringMissingValue() const { return stringMissingValue_; }

  /// \brief number of locations in the obs source (file or generator)
  std::size_t sourceNlocs() const { return sourceNlocs_; }

  /// \brief number of locations in the obs source that fall inside the DA time window
  std::size_t sourceNlocsInsideTimeWindow() const { return sourceNlocsInsideTimeWindow_; }

  /// \brief number of locations in the obs source that fall outside the DA time window
  std::size_t sourceNlocsOutsideTimeWindow() const { return sourceNlocsOutsideTimeWindow_; }

  /// \brief number of locations in the obs source that were rejected by QC checks
  std::size_t sourceNlocsRejectQC() const { return sourceNlocsRejectQC_; }

  /// \brief number of locations in the obs source (file or generator)
  std::size_t nrecs() const { return nrecs_; }

  /// \brief location indices from obs source for this MPI task
  const std::vector<std::size_t> & index() const { return locIndices_; }

  /// \brief assigned record numbers for indices in locIndices_
  const std::vector<std::size_t> & recnums() const { return recNums_; }

  /// \brief save obs data to output file
  /// \param srcGroup source ioda group to be saved into the output file
  virtual void load(Group & destGroup) = 0;

 protected:
  /// \brief parameters to be sent to the reader engine factory
  const oops::RequiredPolymorphicParameter
              <Engines::ReaderParametersBase, Engines::ReaderFactory> & readerParams_;

  /// \brief time window start
  const util::DateTime & winStart_;

  /// \brief time window end
  const util::DateTime & winEnd_;

  /// \brief missing value for string variables
  std::shared_ptr<std::string> stringMissingValue_;

  /// \brief number of locations in the obs source (file or generator)
  std::size_t sourceNlocs_;

  /// \brief number of locations in the obs source (file or generator)
  std::size_t nrecs_;

  /// \brief number of locations in the obs source that fall inside the DA time window
  std::size_t sourceNlocsInsideTimeWindow_;

  /// \brief number of locations in the obs source that fall outside the DA time window
  std::size_t sourceNlocsOutsideTimeWindow_;

  /// \brief number of locations in the obs source that were rejected by QC checks
  std::size_t sourceNlocsRejectQC_;

  /// \brief location indices from obs source for this MPI task
  std::vector<std::size_t> locIndices_;

  /// \brief assigned record numbers for indices in locIndices_
  std::vector<std::size_t> recNums_;

  /// \brief list of variables being assimilated (used by the generator engines)
  const std::vector<std::string> & obsVarNames_;

  /// \brief ioda Distribution object associated with the ObsSpace object
  const std::shared_ptr<Distribution> & distribution_;

  /// \brief list of variables used for the obs grouping function
  const std::vector<std::string> & obsGroupVarList_;
};

}  // namespace ioda

/// @}