#pragma once
/*
 * (C) Copyright 2022 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include <string>
#include <vector>

#include "eckit/mpi/Comm.h"

#include "oops/util/parameters/OptionalParameter.h"
#include "oops/util/parameters/Parameter.h"
#include "oops/util/parameters/Parameters.h"
#include "oops/util/parameters/RequiredParameter.h"
#include "oops/util/parameters/RequiredPolymorphicParameter.h"
#include "oops/util/Printable.h"
#include "oops/util/TimeWindow.h"

#include "ioda/Engines/EngineUtils.h"
#include "ioda/ObsGroup.h"

namespace ioda {
namespace Engines {

class Printable;
class ReaderFactory;

//----------------------------------------------------------------------------------------
// Reader configuration parameters base class
//----------------------------------------------------------------------------------------

/// \brief Parameters base class for the parameters subclasses associated with the
/// ReaderBase subclasses.
class ReaderParametersBase : public oops::Parameters {
    OOPS_ABSTRACT_PARAMETERS(ReaderParametersBase, Parameters)

 public:
    /// \brief Type of the ReaderBase subclass to use.
    oops::RequiredParameter<std::string> type{"type", this};
};

//----------------------------------------------------------------------------------------
// Reader creation parameters
//----------------------------------------------------------------------------------------

class ReaderCreationParameters {
  public:
    ReaderCreationParameters(const util::TimeWindow timeWindow,
                             const eckit::mpi::Comm & comm, const eckit::mpi::Comm & timeComm,
                             const std::vector<std::string> & obsVarNames,
                             const bool isParallelIo);
    const util::TimeWindow timeWindow;

    /// \brief io pool communicator group
    const eckit::mpi::Comm & comm;

    /// \brief time communicator group
    const eckit::mpi::Comm & timeComm;

    /// \brief list of variables to be simulated from the obs source
    const std::vector<std::string> & obsVarNames;

    /// \brief flag indicating whether a parallel io backend is to be used
    /// \details This flag is set to true to indicate a parallel io backend (if available)
    /// should be used.
    const bool isParallelIo;
};

//----------------------------------------------------------------------------------------
// Reader base class
//----------------------------------------------------------------------------------------

/// \details The ReaderBase class along with its subclasses are responsible for providing
/// a ioda engines Group object that is backed up by a particular ioda engines backend for
/// the purpose of reading in obs data.
///
/// \author Stephen Herbener (JCSDA)

class ReaderBase : public util::Printable {
 public:
    ReaderBase(const ReaderCreationParameters & createParams);
    virtual ~ReaderBase() {}

    /// \brief initialize the engine backend after construction
    virtual void initialize() {};

    /// \brief finalize the engine backend before destruction
    virtual void finalize() {};

    /// \brief return the backend that stores the data
    inline ioda::ObsGroup getObsGroup() { return obs_group_; }

    /// \brief return the backend that stores the data
    inline const ioda::ObsGroup getObsGroup() const { return obs_group_; }

    /// \brief return a representative file name for the reader backend
    /// \detail this will be the file name for file sources, and a descriptive
    /// name for generator sources.
    virtual std::string fileName() const = 0;

    /// \brief return true if the locations data (lat, lon, datetime) need to be checked
    /// \details The locations check filters out locations that:
    ///    1. have a datetime value that falls outside the DA timing window
    ///    2. have missing values in either of lat and lon values
    /// Typically you want to enable the check for file backends, and disable the check
    /// for generator backends. The default is true (enabled).
    virtual bool applyLocationsCheck() const { return true; }

 protected:
    //------------------ protected functions ----------------------------------
    /// \brief print() for oops::Printable base class
    /// \param ostream output stream
    virtual void print(std::ostream & os) const = 0;

    //------------------ protected variables ----------------------------------
    /// \brief ObsGroup container associated with the selected backend engine
    ioda::ObsGroup obs_group_;

    /// \brief creation parameters
    ReaderCreationParameters createParams_;
};

}  // namespace Engines
}  // namespace ioda

