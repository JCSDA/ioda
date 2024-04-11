/*
 * (C) Copyright 2022-2023 UCAR
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
#include "ioda/Engines/ReaderFactory.h"
#include "ioda/Group.h"
#include "ioda/ioPool/IoPoolBase.h"
#include "ioda/ioPool/IoPoolParameters.h"

#include "oops/mpi/mpi.h"
#include "oops/util/DateTime.h"
#include "oops/util/parameters/Parameters.h"
#include "oops/util/parameters/OptionalParameter.h"
#include "oops/util/parameters/RequiredPolymorphicParameter.h"
#include "oops/util/Printable.h"
#include "oops/util/TimeWindow.h"

namespace ioda {

class Distribution;

namespace IoPool {


typedef std::map<int, std::vector<std::size_t>> ReaderDistributionMap;

//------------------------------------------------------------------------------------
// Reader pool creation parameters
//------------------------------------------------------------------------------------

class ReaderPoolCreationParameters : public IoPoolCreationParameters {
 public:
    ReaderPoolCreationParameters(
            const eckit::mpi::Comm & commAll, const eckit::mpi::Comm & commTime,
            const oops::RequiredPolymorphicParameter
                <Engines::ReaderParametersBase, Engines::ReaderFactory> & readerParams,
            const util::TimeWindow timeWindow,
            const std::vector<std::string> & obsVarNames,
            const std::shared_ptr<Distribution> & distribution,
            const std::vector<std::string> & obsGroupVarList,
            const std::string & inputFilePrepType);
    virtual ~ReaderPoolCreationParameters() {}

    /// \brief parameters to be sent to the reader engine factory
    const oops::RequiredPolymorphicParameter
                <Engines::ReaderParametersBase, Engines::ReaderFactory> & readerParams;

    const util::TimeWindow timeWindow;

    /// \brief list of variables being assimilated (used by the generator engines)
    const std::vector<std::string> & obsVarNames;

    /// \brief ioda Distribution object associated with the ObsSpace object
    const std::shared_ptr<Distribution> & distribution;

    /// \brief list of variables used for the obs grouping function
    const std::vector<std::string> & obsGroupVarList;

    /// \brief input file preparation type
    /// \detail Allowed values are "internal" and "external". "internal" signifies that
    /// the input file preparation is done during the reader initialize step, and "external"
    /// signifies that the standalone application has already been run to prepare the input
    /// files and they are ready to be used by the reader.
    const std::string & inputFilePrepType;
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

  const util::TimeWindow timeWindow() const {return timeWindow_;}

  /// \brief list of variables being assimilated (used by the generator engines)
  const std::vector<std::string> & obsVarNames() const { return obsVarNames_; }

  /// \brief ioda Distribution object associated with the ObsSpace object
  const std::shared_ptr<Distribution> & distribution() const { return distribution_; }

  /// \brief list of variables used for the obs grouping function
  const std::vector<std::string> & obsGroupVarList() const { return obsGroupVarList_; }

  /// \brief input file prep type ("internal" or "external")
  const std::string & inputFilePrepType() const { return inputFilePrepType_; }

  /// \brief file name passed in from YAML configuration
  const std::string & fileName() const { return fileName_; }

  /// \brief name of pre-processed input file (which the load function uses)
  const std::string & newInputFileName() const { return newInputFileName_; }

  /// \brief name of prep info file (which the load function uses)
  const std::string & prepInfoFileName() const { return prepInfoFileName_; }

  /// \brief work directory
  const std::string & workDir() const { return configParams_.workDir; }

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

  /// \brief epoch string for date time variable
  std::string dtimeEpoch() const { return dtimeEpoch_; }

  /// \brief string value representing the obs source (typically a file)
  std::string readerSrc() const { return readerSrc_; }

  /// \brief save obs data to output file
  /// \param srcGroup source ioda group to be saved into the output file
  virtual void load(Group & destGroup) = 0;

  /// \brief mapping that shows which source location indices go to which ranks
  const ReaderDistributionMap & distributionMap() const { return distributionMap_; }

 protected:
  /// \brief parameters to be sent to the reader engine factory
  const oops::RequiredPolymorphicParameter
              <Engines::ReaderParametersBase, Engines::ReaderFactory> & readerParams_;

  const util::TimeWindow timeWindow_;

  /// \brief list of variables being assimilated (used by the generator engines)
  const std::vector<std::string> & obsVarNames_;

  /// \brief ioda Distribution object associated with the ObsSpace object
  const std::shared_ptr<Distribution> & distribution_;

  /// \brief list of variables used for the obs grouping function
  const std::vector<std::string> & obsGroupVarList_;

  /// \brief list of variables used for the obs grouping function
  const std::string & inputFilePrepType_;

  /// \brief original input file name
  std::string fileName_;

  /// \brief file name for pre-processed input file
  std::string newInputFileName_;

  /// \brief file name for pre-processed prep info file
  std::string prepInfoFileName_;

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

  /// \brief date time epoch string
  std::string dtimeEpoch_;

  /// \brief reader engine source for printing (eg, input file name)
  std::string readerSrc_;

  /// \brief mapping that shows which source location indices go to which ranks
  /// \detail This structure is a map of size_t vectors where the index of
  /// the outer vector denotes the rank in the commAll communicator, and the
  /// entries in the inner vector denote which locations need to go to that rank.
  ReaderDistributionMap distributionMap_;

  /// @brief return prep info file suffix
  std::string prepInfoFileSuffix();

  /// \brief generate and record the file prep info file name
  /// \param fileName file name including path
  std::string setPrepInfoFileName(const std::string & fileName);

  /// \brief generate and record the new input file name associated with this rank
  /// \detail The new input file name is set to an empty string for non pool members.
  /// For pool members the new input file name is based on the work directory path
  /// and original file name. For now, an hdf5 file will always be used.
  /// \param fileName file name including path
  /// \param poolRankNumber MPI rank number in the io pool communicator group
  std::string setNewInputFileName(const std::string & fileName, const int poolRankNumber);
};

}  // namespace IoPool
}  // namespace ioda

/// @}
