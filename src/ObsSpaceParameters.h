#pragma once
/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include <string>
#include <vector>

#include "ioda/core/FileFormat.h"
#include "ioda/core/ParameterTraitsFileFormat.h"
#include "ioda/distribution/DistributionFactory.h"
#include "ioda/ioPool/IoPoolParameters.h"
#include "ioda/Misc/DimensionScales.h"
#include "ioda/Misc/Dimensions.h"
#include "ioda/ObsDataIoParameters.h"

#include "eckit/exception/Exceptions.h"
#include "eckit/mpi/Comm.h"

#include "oops/base/ObsSpaceBase.h"  // for ObsSpaceParametersBase
#include "oops/base/ParameterTraitsVariables.h"
#include "oops/util/DateTime.h"
#include "oops/util/Logger.h"
#include "oops/util/parameters/OptionalParameter.h"
#include "oops/util/parameters/Parameter.h"
#include "oops/util/parameters/Parameters.h"
#include "oops/util/parameters/RequiredParameter.h"
#include "oops/util/parameters/RequiredPolymorphicParameter.h"


namespace eckit {
  class Configuration;
}

namespace ioda {

class ObsExtendParameters : public oops::Parameters {
    OOPS_CONCRETE_PARAMETERS(ObsExtendParameters, oops::Parameters)

 public:
    /// Number of locations allocated to each companion record produced when
    /// extending the ObsSpace.
    oops::RequiredParameter<int> companionRecordLength
        {"allocate companion records with length", this};

    /// Variables that are filled with non-missing values when producing companion profiles.
    oops::Parameter<std::vector<std::string>> nonMissingExtendedVars
        {"variables filled with non-missing values",
            { "latitude", "longitude", "dateTime", "pressure",
                "air_pressure_levels", "stationIdentification" },
            this};
};

class ObsTopLevelParameters : public oops::ObsSpaceParametersBase {
    OOPS_CONCRETE_PARAMETERS(ObsTopLevelParameters, ObsSpaceParametersBase)

 public:
    /// name of obs space
    oops::RequiredParameter<std::string> obsSpaceName{"name", this};

    /// parameters of the MPI distribution
    oops::Parameter<DistributionParametersWrapper> distribution{"distribution", {}, this};

    /// simulated variables
    oops::RequiredParameter<oops::Variables> simVars{"simulated variables", this};

    /// Simulated variables whose observed values may be absent from the input file, but must be
    /// created (computed) by the start of the data assimilation stage.
    oops::Parameter<oops::Variables> derivedSimVars{"derived variables", {}, this};

    /// Observation variables whose observed values are to be processed.
    oops::Parameter<oops::Variables> ObservedVars{"observed variables", {}, this};

    /// Io pool parameters
    oops::Parameter<IoPool::IoPoolParameters> ioPool{"io pool", {}, this};

    /// extend the ObsSpace with extra fixed-size records
    oops::OptionalParameter<ObsExtendParameters> obsExtend{"extension", this};

    /// DateTime of epoch to use when storing dateTime variables.
    /// Note that this should not be prefixed with "seconds since"
    oops::Parameter<util::DateTime> epochDateTime{"epoch DateTime",
                                                  util::DateTime("1970-01-01T00:00:00Z"), this};

    /// \brief Fill this section to read observations from a file.
    oops::RequiredParameter<ObsDataInParameters> obsDataIn{"obsdatain", this};

    /// output specification by writing to a file
    oops::OptionalParameter<ObsDataOutParameters> obsDataOut{"obsdataout", this};
};

class ObsSpaceParameters {
 public:
    /// sub groups of parameters
    ObsTopLevelParameters top_level_;

    /// Constructor
    ObsSpaceParameters(const ObsTopLevelParameters &topLevelParams,
                       const util::TimeWindow timeWindow,
                       const eckit::mpi::Comm & comm, const eckit::mpi::Comm & timeComm) :
                           top_level_(topLevelParams),
                           time_window_(timeWindow), comm_(comm),
                           time_comm_(timeComm),
                           new_dims_(), max_var_size_(0) {
        // Record the MPI rank number. The rank number is being saved during the
        // construction of the Parameters for the ObsSpace saveToFile routine.
        // (saveToFile will uniquify the output file name by tagging on the MPI rank
        // number) For some reason, querying the saved MPI communicator (comm_) during
        // the deconstruction process (when saveToFile is being run) will not reliably
        // return the correct rank number. It was attempted to put in an MPI barrier call
        // in case the issue was one rank finishing up before the other got to the query, but
        // the barrier command itself caused a crash. It appears that the saved MPI
        // communicator is getting corrupted during the deconstruction, but this has not
        // been fully debugged, and should therefore be looked at later.
        mpi_rank_ = comm.rank();
        if (timeComm.size() > 1) {
            mpi_time_rank_ = timeComm.rank();
        } else {
            mpi_time_rank_ = -1;
        }
    }


    const util::TimeWindow timeWindow() const {return time_window_;}

    /// \brief return the associated MPI group communicator
    const eckit::mpi::Comm & comm() const {return comm_;}

    /// \brief return the associated perturbations seed
    int  obsPertSeed() const {return top_level_.obsPerturbationsSeed;}

    /// \brief return the associated MPI time communicator
    const eckit::mpi::Comm & timeComm() const {return time_comm_;}

    /// \brief set a new dimension scale
    void setDimScale(const std::string & dimName, const Dimensions_t curSize,
                     const Dimensions_t maxSize, const Dimensions_t chunkSize) {
        new_dims_.push_back(
            NewDimensionScale<int>(dimName, curSize, maxSize, chunkSize));
        }

    /// \brief get a new dimension scale
    NewDimensionScales_t getDimScales() const { return new_dims_; }

    /// \brief set the maximum variable size
    void setMaxVarSize(const Dimensions_t maxVarSize) { max_var_size_ = maxVarSize; }

    /// \brief get the maximum variable size
    Dimensions_t getMaxVarSize() const { return max_var_size_; }

    /// \brief get the MPI rank number
    std::size_t getMpiRank() const { return mpi_rank_; }

    /// \brief get the MPI rank number
    int getMpiTimeRank() const { return mpi_time_rank_; }

 private:
    const util::TimeWindow time_window_;

    /// \brief MPI group communicator
    const eckit::mpi::Comm & comm_;

    /// \brief MPI time communicator
    const eckit::mpi::Comm & time_comm_;

    /// \brief new dimension scales for output file construction
    NewDimensionScales_t new_dims_;

    /// \brief maximum variable size for output file contruction
    Dimensions_t max_var_size_;

    /// \brief group MPI rank number for output file construction
    std::size_t mpi_rank_;

    /// \brief time MPI rank number of output file construction
    int mpi_time_rank_;
};

}  // namespace ioda
