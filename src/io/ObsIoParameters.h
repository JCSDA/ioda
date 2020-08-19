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

#include "eckit/exception/Exceptions.h"

#include "oops/util/parameters/OptionalParameter.h"
#include "oops/util/parameters/Parameter.h"
#include "oops/util/parameters/Parameters.h"
#include "oops/util/parameters/RequiredParameter.h"

namespace eckit {
  class Configuration;
}

namespace ioda {

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

        /// obs error estimates
        oops::OptionalParameter<std::vector<float>> obsErrors{"obs errors", this};
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
        oops::OptionalParameter<std::vector<float>> obsErrors{"obs errors", this};
};

class ObsFileOutParameters : public oops::Parameters {
    public:
        /// output obs file name
        oops::RequiredParameter<std::string> fileName{"obsfile", this};
};

class ObsIoParameters : public oops::Parameters {
    public:
        /// sub groups of parameters
        std::unique_ptr<ObsFileInParameters> params_in_file_;
        std::unique_ptr<ObsGenerateRandomParameters> params_in_gen_rand_;
        std::unique_ptr<ObsGenerateListParameters> params_in_gen_list_;
        std::unique_ptr<ObsFileOutParameters> params_out_file_;

        /// \brief deserialize the parameter sub groups
        /// \param config "obs space" level configuration
        void deserialize(const eckit::LocalConfiguration & config) {
            /// Must have one of the input parameter sub groups
            oops::Log::trace() << "ObsIoParameters config: " << config << std::endl;
            eckit::LocalConfiguration subConfig;
            if (config.get("obsdatain", subConfig)) {
                oops::Log::trace() << "ObsParameters sub config: FileIn: "
                                   << subConfig << std::endl;
                params_in_file_.reset(new ObsFileInParameters());
                params_in_file_->deserialize(subConfig);
            } else if (config.get("generate", subConfig)) {
                // Need to pass in sub configuration at the generate level, but
                // check to make sure that one of the sub keywords "random" or
                // "list" is specified.
                if (subConfig.has("random")) {
                    oops::Log::trace() << "ObsIoParameters sub config: GenerateRandom: "
                                       << subConfig << std::endl;
                    params_in_gen_rand_.reset(new ObsGenerateRandomParameters());
                    params_in_gen_rand_->deserialize(subConfig);
                } else if (subConfig.has("list")) {
                    oops::Log::trace() << "ObsIoParameters sub config: GenerateList: "
                                       << subConfig << std::endl;
                    params_in_gen_list_.reset(new ObsGenerateListParameters());
                    params_in_gen_list_->deserialize(subConfig);
                } else {
                    throw eckit::BadParameter(
                        "Must specify one of random or list under generate keyword", Here());
                }
            } else {
                throw eckit::BadParameter("Must specify one of obsdatain or generate", Here());
            }

            /// output parameter sub group is optional
            if (config.get("obsdataout", subConfig)) {
                oops::Log::trace() << "ObsIoParameters sub config: FileOut: "
                                   << subConfig << std::endl;
                params_out_file_.reset(new ObsFileOutParameters());
                params_out_file_->deserialize(subConfig);
            }
        }
};

}  // namespace ioda

#endif  // CORE_OBSIOPARAMETERS_H_
