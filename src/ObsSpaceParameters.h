/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef OBSSPACEPARAMETERS_H_
#define OBSSPACEPARAMETERS_H_

#include <string>
#include <vector>

#include "ioda/Group.h"
#include "ioda/Engines/Factory.h"
#include "ioda/Misc/DimensionScales.h"
#include "ioda/Misc/Dimensions.h"
#include "ioda/ObsGroup.h"

#include "eckit/exception/Exceptions.h"
#include "eckit/mpi/Comm.h"

#include "oops/util/DateTime.h"
#include "oops/util/Logger.h"
#include "oops/util/parameters/OptionalParameter.h"
#include "oops/util/parameters/Parameter.h"
#include "oops/util/parameters/Parameters.h"
#include "oops/util/parameters/RequiredParameter.h"

namespace eckit {
  class Configuration;
}

namespace ioda {

constexpr int DEFAULT_FRAME_SIZE = 10000;

enum class ObsIoActions {
OPEN_FILE,
CREATE_FILE,
CREATE_GENERATOR
};

enum class ObsIoModes {
READ_ONLY,
READ_WRITE,
CLOBBER,
NO_CLOBBER
};

enum class ObsIoTypes {
NONE,
OBS_FILE,
GENERATOR_RANDOM,
GENERATOR_LIST
};

class ObsGroupingParameters : public oops::Parameters {
    OOPS_CONCRETE_PARAMETERS(ObsGroupingParameters, Parameters)
 public:
    /// variable of which to base obs record grouping
    oops::Parameter<std::string> obsGroupVar{"group variable", "", this};

    /// variable of which to base obs record sorting
    oops::Parameter<std::string> obsSortVar{"sort variable", "", this};

    /// direction for sort
    oops::Parameter<std::string> obsSortOrder{"sort order", "ascending", this};
};

class ObsFileInParameters : public oops::Parameters {
    OOPS_CONCRETE_PARAMETERS(ObsFileInParameters, Parameters)
 public:
    /// input obs file name
    oops::RequiredParameter<std::string> fileName{"obsfile", this};

    /// options controlling obs record grouping
    oops::Parameter<ObsGroupingParameters> obsGrouping{"obsgrouping", {}, this};

    /// maximum frame size
    oops::Parameter<int> maxFrameSize{"max frame size", DEFAULT_FRAME_SIZE, this};
};

class ObsGenerateRandomParameters : public oops::Parameters {
    OOPS_CONCRETE_PARAMETERS(ObsGenerateRandomParameters, Parameters)
 public:
    /// number of observations
    oops::RequiredParameter<int> numObs{"nobs", this};

    /// latitude range start
    oops::RequiredParameter<float> latStart{"lat1", this};

    /// latitude range end
    oops::RequiredParameter<float> latEnd{"lat2", this};

    /// longitude range start
    oops::RequiredParameter<float> lonStart{"lon1", this};

    /// longitude range end
    oops::RequiredParameter<float> lonEnd{"lon2", this};

    /// random seed
    oops::OptionalParameter<int> ranSeed{"random seed", this};
};

class ObsGenerateListParameters : public oops::Parameters {
    OOPS_CONCRETE_PARAMETERS(ObsGenerateListParameters, Parameters)
 public:
    /// latitude values
    oops::RequiredParameter<std::vector<float>> lats{"lats", this};

    /// longitude values
    oops::RequiredParameter<std::vector<float>> lons{"lons", this};

    /// datetime values
    oops::RequiredParameter<std::vector<std::string>> datetimes{"datetimes", this};
};

class ObsGenerateParameters : public oops::Parameters {
    OOPS_CONCRETE_PARAMETERS(ObsGenerateParameters, Parameters)
 public:
    /// specification for generating using the random method
    oops::OptionalParameter<ObsGenerateRandomParameters> random{"random", this};

    /// specification for generating using the list method
    oops::OptionalParameter<ObsGenerateListParameters> list{"list", this};

    /// obs error estimates
    oops::Parameter<std::vector<float>> obsErrors{"obs errors", { }, this};

    /// maximum frame size
    oops::Parameter<int> maxFrameSize{"max frame size", DEFAULT_FRAME_SIZE, this};
};

class ObsFileOutParameters : public oops::Parameters {
    OOPS_CONCRETE_PARAMETERS(ObsFileOutParameters, Parameters)
 public:
    /// output obs file name
    oops::RequiredParameter<std::string> fileName{"obsfile", this};

    /// maximum frame size
    oops::Parameter<int> maxFrameSize{"max frame size", DEFAULT_FRAME_SIZE, this};
};

class ObsTopLevelParameters : public oops::Parameters {
    OOPS_CONCRETE_PARAMETERS(ObsTopLevelParameters, Parameters)
 public:
    /// name of obs space
    oops::RequiredParameter<std::string> obsSpaceName{"name", this};

    /// name of MPI distribution
    oops::Parameter<std::string> distName{"distribution", "RoundRobin", this};

    /// simulated variables
    oops::RequiredParameter<std::vector<std::string>> simVars{"simulated variables", this};

    /// input specification by reading from a file
    oops::OptionalParameter<ObsFileInParameters> obsInFile{"obsdatain", this};

    /// input specification by reading from a generator
    oops::OptionalParameter<ObsGenerateParameters> obsGenerate{"generate", this};

    /// output specification by writing to a file
    oops::OptionalParameter<ObsFileOutParameters> obsOutFile{"obsdataout", this};
};

class ObsSpaceParameters {
 public:
    /// sub groups of parameters
    ObsTopLevelParameters top_level_;

    /// Constructor
    ObsSpaceParameters(const util::DateTime & winStart, const util::DateTime & winEnd,
                       const eckit::mpi::Comm & comm, const eckit::mpi::Comm & timeComm) :
                           win_start_(winStart), win_end_(winEnd), comm_(comm),
                           time_comm_(timeComm), in_type_(ObsIoTypes::NONE),
                           out_type_(ObsIoTypes::NONE), new_dims_(), max_var_size_(0) {
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

    /// \brief deserialize the parameter sub groups
    /// \param config "obs space" level configuration
    void deserialize(const eckit::Configuration & config) {
        // Must have one of the input parameter sub groups
        oops::Log::trace() << "ObsSpaceParameters config: " << config << std::endl;

        // First deserialize the configuration, then check to make sure we got
        // only one of obsdatain or generate.
        top_level_.deserialize(config);
        if (top_level_.obsInFile.value() != boost::none) {
            in_type_ = ObsIoTypes::OBS_FILE;
        } else if (top_level_.obsGenerate.value() != boost::none) {
            // Check to make sure that one of the sub configurations "random" or
            // "list" is specified.
            if (top_level_.obsGenerate.value()->random.value() != boost::none) {
                in_type_ = ObsIoTypes::GENERATOR_RANDOM;
            } else if (top_level_.obsGenerate.value()->list.value() != boost::none) {
                in_type_ = ObsIoTypes::GENERATOR_LIST;
            } else {
                throw eckit::BadParameter(
                    "Must specify one of random or list under generate keyword", Here());
            }
        } else {
            throw eckit::BadParameter("Must specify one of obsdatain or generate", Here());
        }

        /// output parameter sub group is optional
        if (top_level_.obsOutFile.value() != boost::none) {
            out_type_ = ObsIoTypes::OBS_FILE;
        } else {
            out_type_ = ObsIoTypes::NONE;
        }
    }

    /// \brief return input io type
    ObsIoTypes in_type() const { return in_type_; }

    /// \brief return output io type
    ObsIoTypes out_type() const { return out_type_; }

    /// \brief return the start of the DA timing window
    const util::DateTime & windowStart() const {return win_start_;}

    /// \brief return the end of the DA timing window
    const util::DateTime & windowEnd() const {return win_end_;}

    /// \brief return the associated MPI group communicator
    const eckit::mpi::Comm & comm() const {return comm_;}

    /// \brief return the associated MPI time communicator
    const eckit::mpi::Comm & timeComm() const {return time_comm_;}

    /// \brief set a new dimension scale
    void setDimScale(const std::string & dimName, const Dimensions_t curSize,
                     const Dimensions_t maxSize, const Dimensions_t chunkSize) {
        new_dims_.push_back(
            std::make_shared<NewDimensionScale<int>>(dimName, curSize, maxSize, chunkSize));
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
    /// \brief ObsIo input type
    ObsIoTypes in_type_;

    /// \brief ObsIo output type
    ObsIoTypes out_type_;

    /// \brief Beginning of DA timing window
    const util::DateTime win_start_;

    /// \brief End of DA timing window
    const util::DateTime win_end_;

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

#endif  // OBSSPACEPARAMETERS_H_
