/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef CORE_OBSIOPARAMETERS_H_
#define CORE_OBSIOPARAMETERS_H_

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

class ObsTopLevelParameters : public oops::Parameters {
    public:
        /// simulated variables
        oops::RequiredParameter<std::vector<std::string>> simVars{"simulated variables", this};

        /// input specification by reading from a file
        oops::OptionalParameter<eckit::LocalConfiguration> obsInFile{"obsdatain", this};

        /// input specification by reading from a file
        oops::OptionalParameter<eckit::LocalConfiguration> obsGenerate{"generate", this};

        /// output specification by writing to a file
        oops::OptionalParameter<eckit::LocalConfiguration> obsOutFile{"obsdataout", this};
};

class ObsFileInParameters : public oops::Parameters {
    public:
        /// input obs file name
        oops::RequiredParameter<std::string> fileName{"obsfile", this};

        /// variable of which to base obs record grouping
        oops::Parameter<std::string> obsGroupVar{"group variable", "", this};

        /// variable of which to base obs record sorting 
        oops::Parameter<std::string> obsSortVar{"sort variable", "", this};

        /// direction for sort
        oops::Parameter<std::string> obsSortOrder{"sort order", "", this};

        /// maximum frame size
        oops::Parameter<int> maxFrameSize{"max frame size", DEFAULT_FRAME_SIZE, this};
};

class ObsGenerateRandomParameters : public oops::Parameters {
    public:
        /// number of observations
        oops::RequiredParameter<int> numObs{"random.nobs", this};

        /// latitude range start
        oops::RequiredParameter<float> latStart{"random.lat1", this};

        /// latitude range end
        oops::RequiredParameter<float> latEnd{"random.lat2", this};

        /// longitude range start
        oops::RequiredParameter<float> lonStart{"random.lon1", this};

        /// longitude range end
        oops::RequiredParameter<float> lonEnd{"random.lon2", this};

        /// random seed
        oops::OptionalParameter<int> ranSeed{"random.random seed", this};

        /// obs error estimates
        oops::Parameter<std::vector<float>> obsErrors{"obs errors", { }, this};

        /// maximum frame size
        oops::Parameter<int> maxFrameSize{"max frame size", DEFAULT_FRAME_SIZE, this};
};

class ObsGenerateListParameters : public oops::Parameters {
    public:
        /// latitude values
        oops::RequiredParameter<std::vector<float>> lats{"list.lats", this};

        /// longitude values
        oops::RequiredParameter<std::vector<float>> lons{"list.lons", this};

        /// datetime values
        oops::RequiredParameter<std::vector<std::string>> datetimes{"list.datetimes", this};

        /// obs error estimates
        oops::Parameter<std::vector<float>> obsErrors{"obs errors", { }, this};

        /// maximum frame size
        oops::Parameter<int> maxFrameSize{"max frame size", DEFAULT_FRAME_SIZE, this};
};

class ObsFileOutParameters : public oops::Parameters {
    public:
        /// output obs file name
        oops::RequiredParameter<std::string> fileName{"obsfile", this};

        /// maximum frame size
        oops::Parameter<int> maxFrameSize{"max frame size", DEFAULT_FRAME_SIZE, this};
};

class ObsIoParameters : public oops::Parameters {
    public:
        /// sub groups of parameters
        ObsTopLevelParameters top_level_;
        ObsFileInParameters in_file_;
        ObsGenerateRandomParameters in_gen_rand_;
        ObsGenerateListParameters in_gen_list_;
        ObsFileOutParameters out_file_;

        /// Constructor
        ObsIoParameters(const util::DateTime & winStart, const util::DateTime & winEnd,
                        const eckit::mpi::Comm & comm) :
                            win_start_(winStart), win_end_(winEnd), comm_(comm) {}

        /// \brief deserialize the parameter sub groups
        /// \param config "obs space" level configuration
        void deserialize(const eckit::LocalConfiguration & config) {
            /// Must have one of the input parameter sub groups
            oops::Log::trace() << "ObsIoParameters config: " << config << std::endl;

            /// First deserialize the top level parameters, then deserialize the
            /// appropriate sub configurations
            top_level_.deserialize(config);
            if (top_level_.obsInFile.value() != boost::none) {
                in_file_.deserialize(top_level_.obsInFile.value().get());
                in_type_ = ObsIoTypes::OBS_FILE;
            } else if (top_level_.obsGenerate.value() != boost::none) {
                // Need to pass in sub configuration at the generate level, but
                // check to make sure that one of the sub keywords "random" or
                // "list" is specified.
                if (top_level_.obsGenerate.value().get().has("random")) {
                    in_gen_rand_.deserialize(top_level_.obsGenerate.value().get());
                    in_type_ = ObsIoTypes::GENERATOR_RANDOM;
                } else if (top_level_.obsGenerate.value().get().has("list")) {
                    in_gen_list_.deserialize(top_level_.obsGenerate.value().get());
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
                out_file_.deserialize(top_level_.obsOutFile.value().get());
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

        /// \brief return the associated MPI communicator
        const eckit::mpi::Comm & comm() const {return comm_;}

        /// \brief set a new dimension scale
        void setDimScale(const std::string & dimName, const Dimensions_t curSize,
                         const Dimensions_t maxSize, const Dimensions_t chunkSize) {
            new_dims_.push_back(
                std::make_shared<NewDimensionScale<int>>(dimName, curSize, maxSize, chunkSize));
            }

        /// \brief get a new dimension scale
        NewDimensionScales_t getDimScales() const { return new_dims_; };

        /// \brief set the maximum variable size
        void setMaxVarSize(const int maxVarSize) { max_var_size_ = maxVarSize; };

        /// \brief get the maximum variable size
        int getMaxVarSize() const { return max_var_size_; };

    private:
        /// \brief ObsIo input type
        ObsIoTypes in_type_;

        /// \brief ObsIo output type
        ObsIoTypes out_type_;

        /// \brief Beginning of DA timing window
        const util::DateTime win_start_;

        /// \brief End of DA timing window
        const util::DateTime win_end_;

        /// \brief MPI communicator
        const eckit::mpi::Comm & comm_;

        /// \brief new dimension scales for output file construction
        NewDimensionScales_t new_dims_;

        /// \brief maximum variable size for output file contruction
        int max_var_size_;
};

}  // namespace ioda

#endif  // CORE_OBSIOPARAMETERS_H_
