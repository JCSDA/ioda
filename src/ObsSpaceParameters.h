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

#include "ioda/core/FileFormat.h"
#include "ioda/core/ParameterTraitsFileFormat.h"
#include "ioda/distribution/DistributionFactory.h"
#include "ioda/Misc/DimensionScales.h"
#include "ioda/Misc/Dimensions.h"
#include "ioda/Misc/IoPoolParameters.h"
#include "ioda/io/ObsIoFactory.h"
#include "ioda/io/ObsIoParametersBase.h"


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

class ObsFileInParameters : public ObsIoParametersBase {
     OOPS_CONCRETE_PARAMETERS(ObsFileInParameters, ObsIoParametersBase)

 public:
    /// input obs file name
    oops::RequiredParameter<std::string> fileName{"obsfile", this};

    /// input obs file format
    ///
    /// Possible values:
    /// * `hdf5`: HDF5 file format
    /// * `odb`: ODB file format
    /// * `auto` (default): file format determined automatically from the file name extension
    ///   (`.odb` -- ODB, everything else -- HDF5).
    oops::Parameter<FileFormat> format{"format", FileFormat::AUTO, this};

    /// reading from multiple files (1 per MPI task)
    /// This option is not typically used. It is used to tell the system
    /// to read observations from the ioda output files (one per MPI task)
    /// from a prior run instead of reading and distributing from the original
    /// file. This is currently being used in LETKF applictions.
    oops::Parameter<bool> readFromSeparateFiles{"read obs from separate file", false, this};

    /// file with variable name mapping rules
    ///
    /// Required for obs files in the ODB format, unused otherwise.
    oops::Parameter<std::string> mappingFile{"mapping file", "", this};
    /// file with query parameters
    ///
    /// Required for obs files in the ODB format, unused otherwise.
    oops::Parameter<std::string> queryFile{"query file", "", this};
};

class ObsFileOutParameters : public ObsIoParametersBase {
    OOPS_CONCRETE_PARAMETERS(ObsFileOutParameters, ObsIoParametersBase)

 public:
    /// output obs file name
    oops::RequiredParameter<std::string> fileName{"obsfile", this};
};

class ObsExtendParameters : public oops::Parameters {
    OOPS_CONCRETE_PARAMETERS(ObsExtendParameters, oops::Parameters)

 public:
    /// Number of locations allocated to each companion record produced when extending the ObsSpace.
    oops::RequiredParameter<int> companionRecordLength
        {"allocate companion records with length", this};

    /// Variables that are filled with non-missing values when producing companion profiles.
    oops::Parameter<std::vector<std::string>> nonMissingExtendedVars
        {"variables filled with non-missing values",
            { "latitude", "longitude", "dateTime", "air_pressure",
                "air_pressure_levels", "station_id" },
            this};
};

class ObsGenerateParametersBase : public ObsIoParametersBase {
    OOPS_ABSTRACT_PARAMETERS(ObsGenerateParametersBase, ObsIoParametersBase)

 public:
    /// obs error estimates
    oops::Parameter<std::vector<float>> obsErrors{"obs errors", { }, this};
};

class EmbeddedObsGenerateRandomParameters : public oops::Parameters {
    OOPS_CONCRETE_PARAMETERS(EmbeddedObsGenerateRandomParameters, Parameters)

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

/// Options controlling the ObsIoGenerateRandom class
class ObsGenerateRandomParameters : public ObsGenerateParametersBase {
    OOPS_CONCRETE_PARAMETERS(ObsGenerateRandomParameters, ObsGenerateParametersBase)

 public:
    /// options shared by this class and the legacy implementation (LegacyObsGenerateParameters)
    EmbeddedObsGenerateRandomParameters random{this};
};

class EmbeddedObsGenerateListParameters : public oops::Parameters {
    OOPS_CONCRETE_PARAMETERS(EmbeddedObsGenerateListParameters, Parameters)

 public:
    /// latitude values
    oops::RequiredParameter<std::vector<float>> lats{"lats", this};

    /// longitude values
    oops::RequiredParameter<std::vector<float>> lons{"lons", this};

    /// time offsets (s) relative to epoch
    oops::RequiredParameter<std::vector<int64_t>> dateTimes{"dateTimes", this};

    /// epoch (ISO 8601 string) relative to which datetimes are computed
    oops::Parameter<std::string> epoch{"epoch", "seconds since 1970-01-01T00:00:00Z", this};
};

/// Options controlling the ObsIoGenerateList class
class ObsGenerateListParameters : public ObsGenerateParametersBase {
    OOPS_CONCRETE_PARAMETERS(ObsGenerateListParameters, ObsGenerateParametersBase)

 public:
    /// options shared by this class and the legacy implementation (LegacyObsGenerateParameters)
    EmbeddedObsGenerateListParameters list{this};
};

/// \brief Options in the 'generate' YAML section.
///
/// \note If you add or remove any Parameter member variables from this class, be sure to update
/// ObsTopLevelParameters::deserialize() to match.
class LegacyObsGenerateParameters : public oops::Parameters {
    OOPS_CONCRETE_PARAMETERS(LegacyObsGenerateParameters, Parameters)

 public:
    /// specification for generating using the random method
    oops::OptionalParameter<EmbeddedObsGenerateRandomParameters> random{"random", this};

    /// specification for generating using the list method
    oops::OptionalParameter<EmbeddedObsGenerateListParameters> list{"list", this};

    /// options controlling obs record grouping
    oops::Parameter<ObsGroupingParameters> obsGrouping{"obsgrouping", { }, this};

    /// obs error estimates
    oops::Parameter<std::vector<float>> obsErrors{"obs errors", { }, this};

    /// maximum frame size
    oops::Parameter<int> maxFrameSize{"max frame size", DEFAULT_FRAME_SIZE, this};
};

class ObsIoParametersWrapper : public oops::Parameters {
    OOPS_CONCRETE_PARAMETERS(ObsIoParametersWrapper, Parameters)
 public:
    oops::RequiredPolymorphicParameter<ObsIoParametersBase, ObsIoFactory>
      obsIoInParameters{"type", this};
};

class ObsTopLevelParameters : public oops::ObsSpaceParametersBase {
    OOPS_CONCRETE_PARAMETERS(ObsTopLevelParameters, ObsSpaceParametersBase)

 public:
    /// Reimplemented to store contents of the `obsdatain` or `generate` section (if present)
    /// in the source member variable. This makes it possible for the options related to the source
    /// of input data to be accessed in a uniform way (regardless of in which section they were
    /// specified) by calling obsIoInParameters().
    void deserialize(util::CompositePath &path, const eckit::Configuration &config) override;

    /// name of obs space
    oops::RequiredParameter<std::string> obsSpaceName{"name", this};

    /// parameters of the MPI distribution
    oops::Parameter<DistributionParametersWrapper> distribution{"distribution", {}, this};

    /// If saveObsDistribution/"save obs distribution" set to true,
    /// global location indices and record numbers will be stored
    /// in the MetaData/saved_index and MetaData/saved_record_number variables, respectively.
    /// These variables will be saved along with all other variables
    /// to the output files generated if the obsdataout.obsfile option is set.
    ///
    /// When the "obsdatain.read obs from separate file" option is set
    /// and hence each process reads a separate input file,
    /// the presence of these variables makes it possible
    /// to identify observations stored in more than one input file.

    oops::Parameter<bool> saveObsDistribution{"save obs distribution", false, this};

    /// simulated variables
    oops::RequiredParameter<oops::Variables> simVars{"simulated variables", this};

    /// Simulated variables whose observed values may be absent from the input file, but must be
    /// created (computed) by the start of the data assimilation stage.
    oops::Parameter<oops::Variables> derivedSimVars{"derived variables", {}, this};

    /// Observation variables whose observed values are to be processed.
    oops::Parameter<oops::Variables> ObservedVars{"observed variables", {}, this};

    /// Io pool parameters
    oops::Parameter<IoPoolParameters> ioPool{"io pool", {}, this};

    /// output specification by writing to a file
    oops::OptionalParameter<ObsFileOutParameters> obsOutFile{"obsdataout", this};

    /// extend the ObsSpace with extra fixed-size records
    oops::OptionalParameter<ObsExtendParameters> obsExtend{"extension", this};

    /// DateTime of epoch to use when storing dateTime variables.
    /// Note that this should not be prefixed with "seconds since"
    oops::Parameter<util::DateTime> epochDateTime{"epoch DateTime",
                                                  util::DateTime("1970-01-01T00:00:00Z"), this};

    /// parameters indicating where to load data from
    const ObsIoParametersBase &obsIoInParameters() const {
      if (source.value() != boost::none)
        return source.value()->obsIoInParameters.value();
      throw eckit::BadValue("obsIoInParameters() must not be called before deserialize()", Here());
    }

 private:
    /// \brief Fill this section to read observations from a file.
    oops::OptionalParameter<ObsFileInParameters> obsInFile{"obsdatain", this};

    /// \brief Fill this section to generate observations on the fly.
    oops::OptionalParameter<LegacyObsGenerateParameters> obsGenerate{"generate", this};

    /// \brief Fill this section instead of `obsdatain` and `generate` to load observations from
    /// any other source.
    oops::OptionalParameter<ObsIoParametersWrapper> source{"source", this};
};

class ObsSpaceParameters {
 public:
    /// sub groups of parameters
    ObsTopLevelParameters top_level_;

    /// Constructor
    ObsSpaceParameters(const ObsTopLevelParameters &topLevelParams,
                       const util::DateTime & winStart, const util::DateTime & winEnd,
                       const eckit::mpi::Comm & comm, const eckit::mpi::Comm & timeComm) :
                           top_level_(topLevelParams),
                           win_start_(winStart), win_end_(winEnd), comm_(comm),
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

    /// \brief return the start of the DA timing window
    const util::DateTime & windowStart() const {return win_start_;}

    /// \brief return the end of the DA timing window
    const util::DateTime & windowEnd() const {return win_end_;}

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
