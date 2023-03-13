/*
 * (C) Copyright 2017-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \defgroup ioda_cxx_obsspace Observation Spaces
 * \brief Provides the ioda::ObsSpace interface
 * \ingroup ioda_cxx_api
 *
 * @{
 * \file ObsSpace.h
 * \brief Observation Spaces
 * \ingroup ioda_cxx_obsspace
 */
#ifndef OBSSPACE_H_
#define OBSSPACE_H_

#include <functional>
#include <map>
#include <memory>
#include <numeric>
#include <ostream>
#include <set>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "eckit/mpi/Comm.h"

#include "oops/base/ObsSpaceBase.h"
#include "oops/base/Variables.h"
#include "oops/util/DateTime.h"
#include "oops/util/Logger.h"
#include "ioda/core/IodaUtils.h"
#include "ioda/distribution/Distribution.h"
#include "ioda/Misc/Dimensions.h"
#include "ioda/ObsGroup.h"
#include "ioda/ObsSpaceParameters.h"
#include "ioda/Variables/Fill.h"
#include "ioda/Variables/VarUtils.h"

// Forward declarations
namespace eckit {
    class Configuration;
}

namespace ioda {
    class ObsVector;

    //-------------------------------------------------------------------------------------
    /// Enum type for obs variable data types
    enum class ObsDtype {
        None,
        Float,
        Integer,
        Integer_64,
        String,
        DateTime,
        Bool
    };

    /// \brief Enum type for obs dimension ids
    /// \details The first two dimension names for now are Location and Channel. This will
    /// likely expand in the future, so make sure that this enum class and the following
    /// initializer function stay in sync.
    enum class ObsDimensionId {
        Location,
        Channel
    };

    /// \brief Wrapper class that maps dimension ids to names.
    class ObsDimInfo {
     public:
        ObsDimInfo();

        /// \brief return the standard id value for the given dimension name
        ObsDimensionId get_dim_id(const std::string & dimName) const;

        /// \brief return the dimension name for the given dimension id
        std::string get_dim_name(const ObsDimensionId dimId) const;

        /// \brief return the dimension size for the given dimension id
        std::size_t get_dim_size(const ObsDimensionId dimId) const;

        /// \brief set the dimension size for the given dimension id
        void set_dim_size(const ObsDimensionId dimId, std::size_t dimSize);

     private:
        /// \brief map going from dim id to dim name
        std::map<ObsDimensionId, std::string> dim_id_name_;

        /// \brief map going from dim id to dim size
        std::map<ObsDimensionId, std::size_t> dim_id_size_;

        /// \brief map going from dim name to id
        std::map<std::string, ObsDimensionId> dim_name_id_;
    };

    /// @brief Template handlers for implicit variable conversion.
    /// @tparam Type is the source type of the data.
    template <class Type>
    struct ConvertType {
      /// @brief The type that data should be converted to upon write.
      typedef Type to_type;
    };
    template<>
    struct ConvertType<double> {
      typedef float to_type;
    };

    /// \brief Observation data class for IODA
    ///
    /// \details This class handles the memory store of observation data. It handles
    /// the transfer of data between memory and files, the distribution of obs data
    /// across multiple process elements, the filtering out of obs data that is outside
    /// the DA timing window, the transfer of data between UFO, OOPS and IODA, and data type
    /// conversion that is "missing value aware".
    ///
    /// During the DA run, all data transfers are done in memory. The only time file I/O is
    /// invoked is during the constructor (read from the file into the obs container) and
    /// optionally during the the destructor (write from obs container into the file).
    class ObsSpace : public oops::ObsSpaceBase {
     public:
        //---------------------------- typedefs -------------------------------
        typedef std::map<std::size_t, std::vector<std::size_t>> RecIdxMap;
        typedef RecIdxMap::const_iterator RecIdxIter;
        typedef ObsTopLevelParameters Parameters_;

        //---------------------------- functions ------------------------------
        /// \name Constructors and destructors
        /// @{

        /// \brief Config based constructor for an ObsSpace object.
        ///
        /// \details This constructor will read in from the obs file and transfer the
        /// variables into the obs container. Obs falling outside the DA timing window,
        /// specified by bgn and end, will be discarded before storing them in the
        /// obs container.
        ///
        /// \param params Configuration parameters (an instance of ObsTopLevelParameters)
        /// \param comm MPI communicator for model grouping
        /// \param bgn DateTime object holding the start of the DA timing window
        /// \param end DateTime object holding the end of the DA timing window
        /// \param timeComm MPI communicator for ensemble
        ObsSpace(const Parameters_ & params, const eckit::mpi::Comm & comm,
                const util::DateTime & bgn, const util::DateTime & end,
                const eckit::mpi::Comm & timeComm);
        ObsSpace(const ObsSpace &);
        virtual ~ObsSpace() {}

        /// @}
        /// @name Constructor-defined parameters
        /// @{

        /// \details This method will return the start of the DA timing window
        const util::DateTime & windowStart() const {return winbgn_;}

        /// \details This method will return the end of the DA timing window
        const util::DateTime & windowEnd() const {return winend_;}

        /// \details This method will return the associated MPI communicator
        const eckit::mpi::Comm & comm() const {return commMPI_;}

        /// \details This method will return the associated parameters
        const ObsSpaceParameters & params() const {return obs_params_;}

        /// \brief return MPI distribution object
        std::shared_ptr<const Distribution> distribution() const { return dist_;}

        /// @}
        /// @name Closeout functions
        /// @{

        /// \brief save the obs space data into a file (if obsdataout specified)
        /// \details This function will save the obs space data into a file, but only if
        ///          the obsdataout parameter is specified in the YAML configuration.
        ///          Note that this function will do nothing if the obsdataout specification
        ///          is not present.
        ///
        ///          The purpose of this save function is to fix an issue where the hdf5
        ///          library closes the file (via a C API) during the time when the
        ///          ObsSpace destructor (C++) is still writing to that file. These
        ///          actions can sometimes get out of sync since they are being triggered
        ///          from different sources during the clean up after a job completes.
        void save();

        /// @}
        /// @name General querying functions
        /// @{

        /// \brief return the total number of locations in the corresponding obs spaces
        ///        across all MPI tasks
        std::size_t globalNumLocs() const {return gnlocs_;}

        /// \brief return number of locations from obs source that were outside the time window
        std::size_t globalNumLocsOutsideTimeWindow() const {return gnlocs_outside_timewindow_;}

        /// \brief return number of locations from obs source that were outside the time window
        std::size_t globalNumLocsRejectQC() const {return gnlocs_reject_qc_;}

        /// \brief return the number of locations in the obs space.
        /// Note that nlocs may be smaller than global unique nlocs due to distribution of obs
        /// across multiple process elements.
        inline size_t nlocs() const { return get_dim_size(ObsDimensionId::Location); }

        /// \brief return the number of channels in the container. If this is not a radiance
        /// obs type, then this will return zero.
        inline size_t nchans() const { return get_dim_size(ObsDimensionId::Channel); }

        /// \brief return the number of records in the obs space container
        /// \details This is the number of sets of locations after applying the
        /// optional grouping.
        std::size_t nrecs() const {return nrecs_;}

        /// \brief return the number of variables in the obs space container.
        /// "Variables" refers to the quantities that can be assimilated as opposed to meta data.
        std::size_t nvars() const;

        /// \brief return the standard dimension name for the given dimension id
        std::string get_dim_name(const ObsDimensionId dimId) const {
            return dim_info_.get_dim_name(dimId);
        }

        /// \brief return the standard dimension size for the given dimension id
        std::size_t get_dim_size(const ObsDimensionId dimId) const {
            return dim_info_.get_dim_size(dimId);
        }

        /// \brief return the standard dimension id for the given dimension name
        ObsDimensionId get_dim_id(const std::string & dimName) const {
            return dim_info_.get_dim_id(dimName);
        }

        /// \brief return YAML configuration parameter: obsdatain.obsgrouping.group variables
        const std::vector<std::string> & obs_group_vars() const;

        /// \brief return YAML configuration parameter: obsdatain.obsgrouping.sort variable
        std::string obs_sort_var() const;

        /// \brief return YAML configuration parameter: obsdatain.obsgrouping.sort group
        std::string obs_sort_group() const;

        /// \brief return YAML configuration parameter: obsdatain.obsgrouping.sort order
        std::string obs_sort_order() const;

        /// \brief return the name of the obs type being stored
        const std::string & obsname() const {return obsname_;}

        /// \brief return the name of the MPI distribution
        std::string distname() const {return dist_->name();}

        /// \brief return reference to the record number vector
        const std::vector<std::size_t> & recnum() const {return recnums_;}

        /// \brief return reference to the index vector
        /// \details This method returns a reference to the index vector
        ///          data member. This is for read only access.
        /// The returned vector has length nlocs() and contains the original indices of
        /// locations from the input ioda file corresponding to locations stored in this
        /// ObsSpace object -- i.e. those that were selected by the timing window filter
        /// and the MPI distribution.
        ///
        /// Example 1: Suppose the RoundRobin distribution is used and and there are two
        /// MPI tasks (ranks 0 and 1). The even-numbered locations from the file will go
        /// to rank 0, and the odd-numbered locations will go to rank 1. This means that
        /// `ObsSpace::index()` will return the vector `0, 2, 4, 6, ...` on rank 0 and
        /// `1, 3, 5, 7, ...` on rank 1.
        ///
        /// Example 2: Suppose MPI is not used and the file contains 10 locations in total,
        /// but locations 2, 3 and 7 are outside the DA timing window. In this case,
        /// `ObsSpace::index()` will return `0, 1, 4, 5, 6, 8, 9`.
        const std::vector<std::size_t> & index() const {return indx_;}

        /// \brief return true if variable `name` exists in group `group` or (unless `skipDerived`
        /// is set to true) `"Derived" + `group`.
        bool has(const std::string & group, const std::string & name,
                 bool skipDerived = false) const;

        /// \brief return data type for group/variable
        /// \param group Group name containting the variable
        /// \param name Variable name
        /// \param skipDerived
        ///   By default, this function will look for the variable `name` in the group `"Derived" +
        ///   group` first and only if it doesn't exist will it look in the group `group`. Set this
        ///   parameter to `true` to look only in the group `group`.
        ObsDtype dtype(const std::string & group, const std::string & name,
                       bool skipDerived = false) const;

        /// \brief return the collection of all variables to be processed
        /// (observed + derived variables)
        const oops::Variables & obsvariables() const {return obsvars_;}

        /// \brief return the collection of observed variables loaded from the input file
        const oops::Variables & initial_obsvariables() const
        { return initial_obsvars_; }

        /// \brief return the collection of derived variables (variables computed
        /// after loading the input file)
        const oops::Variables & derived_obsvariables() const
        { return derived_obsvars_; }

        /// \brief return the collection of simulated variables
        const oops::Variables & assimvariables() const
        { return assimvars_;}

        /// @}
        /// @name Functions to access the behind-the-scenes ObsGroup
        /// @{

        /// \brief return the ObsGroup that stores the data
        inline ObsGroup getObsGroup() { return obs_group_; }

        /// \brief return the ObsGroup that stores the data
        inline const ObsGroup getObsGroup() const { return obs_group_; }

        /// @}
        /// @name IO functions
        /// @{

        /// \brief transfer data from the obs container to vdata
        ///
        /// \details The following get_db methods are the same except for the data type
        /// of the data being transferred (integer, float, double, string, DateTime). The
        /// caller needs to allocate the memory that the vdata parameter points to
        ///
        /// \param group Name of container group (ObsValue, ObsError, MetaData, etc.)
        /// \param name  Name of container variable
        /// \param vdata Vector where container data is being transferred to
        /// \param chanSelect Channel selection (list of channel numbers)
        /// \param skipDerived
        ///   By default, this function will look for the variable `name` in the group `"Derived" +
        ///   group` first and only if it doesn't exist will it look in the group `group`. Set this
        ///   parameter to `true` to look only in the group `group`.
        void get_db(const std::string & group, const std::string & name,
                    std::vector<int> & vdata,
                    const std::vector<int> & chanSelect = { },
                    bool skipDerived = false) const;
        void get_db(const std::string & group, const std::string & name,
                    std::vector<int64_t> & vdata,
                    const std::vector<int> & chanSelect = { },
                    bool skipDerived = false) const;
        void get_db(const std::string & group, const std::string & name,
                    std::vector<float> & vdata,
                    const std::vector<int> & chanSelect = { },
                    bool skipDerived = false) const;
        void get_db(const std::string & group, const std::string & name,
                    std::vector<double> & vdata,
                    const std::vector<int> & chanSelect = { },
                    bool skipDerived = false) const;
        void get_db(const std::string & group, const std::string & name,
                    std::vector<std::string> & vdata,
                    const std::vector<int> & chanSelect = { },
                    bool skipDerived = false) const;
        void get_db(const std::string & group, const std::string & name,
                    std::vector<util::DateTime> & vdata,
                    const std::vector<int> & chanSelect = { },
                    bool skipDerived = false) const;
        void get_db(const std::string & group, const std::string & name,
                    std::vector<bool> & vdata,
                    const std::vector<int> & chanSelect = { },
                    bool skipDerived = false) const;

        /// \brief transfer data from vdata to the obs container
        ///
        /// \details The following put_db methods are the same except for the data type
        /// of the data being transferred (integer, float, double, string, DateTime). The
        /// caller needs to allocate and assign the memory that the vdata parameter points to.
        ///
        /// \param group Name of container group (ObsValue, ObsError, MetaData, etc.)
        /// \param name  Name of container variable
        /// \param vdata Vector where container data is being transferred from
        /// \param dimList Vector of dimension names (for creating variable if needed)
        void put_db(const std::string & group, const std::string & name,
                    const std::vector<int> & vdata,
                    const std::vector<std::string> & dimList = { "Location" });
        void put_db(const std::string & group, const std::string & name,
                    const std::vector<int64_t> & vdata,
                    const std::vector<std::string> & dimList = { "Location" });
        void put_db(const std::string & group, const std::string & name,
                    const std::vector<float> & vdata,
                    const std::vector<std::string> & dimList = { "Location" });
        void put_db(const std::string & group, const std::string & name,
                    const std::vector<double> & vdata,
                    const std::vector<std::string> & dimList = { "Location" });
        void put_db(const std::string & group, const std::string & name,
                    const std::vector<std::string> & vdata,
                    const std::vector<std::string> & dimList = { "Location" });
        void put_db(const std::string & group, const std::string & name,
                    const std::vector<util::DateTime> & vdata,
                    const std::vector<std::string> & dimList = { "Location" });
        void put_db(const std::string & group, const std::string & name,
                    const std::vector<bool> & vdata,
                    const std::vector<std::string> & dimList = { "Location" });

        /// @}
        /// @name Record index and sorting functions
        /// @{

        /// \brief Return the begin iterator associated with the recidx_ data member
        const RecIdxIter recidx_begin() const;

        /// \brief Return the end iterator associated with the recidx_ data member
        const RecIdxIter recidx_end() const;

        /// \brief true if given record number exists in the recidx_ data member
        /// \param recNum Record number being searched for
        bool recidx_has(const std::size_t recNum) const;

        /// \brief true if the groups in the recidx data member are sorted
        bool obsAreSorted() const { return recidx_is_sorted_; }

        /// \brief return record number pointed to by the given iterator
        /// \param irec Iterator into the recidx_ data member
        std::size_t recidx_recnum(const RecIdxIter & irec) const;

        /// \brief return record number vector pointed to by the given iterator
        /// \param irec Iterator into the recidx_ data member
        const std::vector<std::size_t> & recidx_vector(const RecIdxIter & irec) const;

        /// \brief return record number vector selected by the given record number
        /// \param recNum Record number being searched for
        const std::vector<std::size_t> & recidx_vector(const std::size_t recNum) const;

        /// \brief return all record numbers from the recidx_ data member
        std::vector<std::size_t> recidx_all_recnums() const;

        /// @}


     private:
        // ----------------------------- private data members ---------------------------
        /// \brief Beginning of DA timing window
        const util::DateTime winbgn_;

        /// \brief End of DA timing window
        const util::DateTime winend_;

        /// \brief MPI communicator
        const eckit::mpi::Comm & commMPI_;

        /// \brief total number of locations
        std::size_t gnlocs_;

        /// \brief number of nlocs from the obs source that are outside the time window
        std::size_t gnlocs_outside_timewindow_;

        /// \brief number of nlocs from the obs source that are outside the time window
        std::size_t gnlocs_reject_qc_;

        /// \brief number of records
        std::size_t nrecs_;

        /// \brief dimension information for variables in this obs space
        ObsDimInfo dim_info_;

        /// \brief map to go from channel number (not necessarily consecutive)
        ///        to channel index (consecutive, starting from zero).
        std::map<int, int> chan_num_to_index_;

        /// \brief observation data store
        ObsGroup obs_group_;

        /// \brief obs io parameters
        ObsSpaceParameters obs_params_;

        /// \brief name of obs space
        std::string obsname_;

        /// \brief When greater than zero print run stats (runtime, memory usage)
        int print_run_stats_;

        /// \brief Initial observation variables to be processed (observations
        /// present in input file)
        oops::Variables initial_obsvars_;

        /// \brief Derived observation variables to be processed (variables computed
        /// after loading the input file)
        oops::Variables derived_obsvars_;

        /// \brief Observation variables to be processed
        oops::Variables obsvars_;

        /// \brief Observation variables to be simulated
        oops::Variables assimvars_;

        /// \brief MPI distribution object
        std::shared_ptr<Distribution> dist_;

        /// \brief indexes of locations to extract from the input obs file
        std::vector<std::size_t> indx_;

        /// \brief record numbers associated with the location indexes
        std::vector<std::size_t> recnums_;

        /// \brief profile ordering
        RecIdxMap recidx_;

        /// \brief indicator whether the data in recidx_ is sorted
        bool recidx_is_sorted_;

        /// \brief map showing association of dim names with each variable name
        VarUtils::VarDimMap dims_attached_to_vars_;

        /// \brief cache for frontend selection
        std::map<VarUtils::Vec_Named_Variable, Selection> known_fe_selections_;

        /// \brief cache for backend selection
        std::map<VarUtils::Vec_Named_Variable, Selection> known_be_selections_;

        /// \brief disable the "=" operator
        ObsSpace & operator= (const ObsSpace &) = delete;

        // ----------------------------- private functions ------------------------------
        /// \brief print function for oops::Printable class
        /// \param os output stream
        void print(std::ostream & os) const;

        /// \brief load the obs space data from an obs source (file or generator)
        void load();

        /// \brief Extend the ObsSpace according to the method requested in
        ///  the configuration file.
        /// \param params object containing specs for extending the ObsSpace
        void extendObsSpace(const ObsExtendParameters & params);

        /// \brief For each simulated variable that doesn't have an accompanying array
        /// in the ObsError or DerivedObsError group, create one, fill it with missing values
        /// and add it to the DerivedObsError group.
        void createMissingObsErrors();

        /// \brief Create the recidx data structure holding sorted record groups
        /// \details This method will construct a data structure that holds the
        /// location order within each group sorted by the values of the specified
        /// sort variable.
        void buildSortedObsGroups();

        /// \brief Create the recidx data structure with unsorted record groups
        /// \details This method will initialize the recidx structure without
        /// any particular ordering of the record groups.
        void buildRecIdxUnsorted();

        /// \brief resize along Location dimension
        /// \param LocationSize new size to either append or reset
        /// \param append when true append LocationSize to current size, otherwise reset size
        void resizeLocation(const Dimensions_t LocationSize, const bool append);

        /// \brief store a variable in the obs_group_ object
        /// \param obsIo obs source object
        /// \param varName Name of obs_group_ variable for obs_group_ object
        /// \param varValues Values for obs_group_ variable
        /// \param frameStart is the start of the ObsFrame
        /// \param frameCount is the size of the ObsFrame
        template<typename VarType>
        void storeVar(const std::string & varName, std::vector<VarType> & varValues,
                      const Dimensions_t frameStart, const Dimensions_t frameCount);

        /// \brief get fill value for use in the obs_group_ object
        template<typename DataType>
        DataType getFillValue() {
            DataType fillVal = util::missingValue(fillVal);
            return fillVal;
        }

        /// \brief load a variable from the obs_group_ object
        /// \details This function will load data from the obs_group_ object into
        ///          the memory buffer (vector) varValues. The chanSelect parameter
        ///          is only used when the variable is 2D radiance data (Location X Channel),
        ///          and contains a list of channel numbers to be selected from the
        ///          obs_group_ variable.
        /// \param group Name of Group in obs_group_
        /// \param name Name of Variable in group
        /// \param selectChan Vector of channel numbers for selection
        /// \param varValues memory to load from obs_group_ variable
        /// \param skipDerived
        ///   By default, this function will search for the variable `name` both in the group
        ///   `group` and `"Derived" + group`. Set this parameter to `true` to search only in the
        ///   group `group`.
        template<typename VarType>
        void loadVar(const std::string & group, const std::string & name,
                     const std::vector<int> & chanSelect,
                     std::vector<VarType> & varValues, bool skipDerived = false) const;

        /// \brief save a variable to the obs_group_ object
        /// \param group Name of Group in obs_group_
        /// \param name Name of Variable in group.
        /// \param varValues values to be saved
        /// \param dimList Vector of dimension names (for creating variable if needed)
        ///
        /// If the group `group` does not contain a variable with the specified name, but this name
        /// has the form <string>_<integer> and `obs_group_` contains an `Channel` dimension, this
        /// function will save `varValues` in the slice of variable <string> corresponding to
        /// channel <integer>. If channel <integer> does not exist or the variable <string> already
        /// exists but is not associated with the `Channel` dimension, an exception will be thrown.
        template<typename VarType>
        void saveVar(const std::string & group, std::string name,
                     const std::vector<VarType> & varValues,
                     const std::vector<std::string> & dimList);

        /// \brief Create selections of slices of the variable \p variable along dimension
        /// \p ChannelDimIndex corresponding to channels \p channels.
        ///
        /// \returns The number of elements in each selection.
        std::size_t createChannelSelections(const Variable & variable,
                                            std::size_t ChannelDimIndex,
                                            const std::vector<int> & channels,
                                            Selection & memSelect,
                                            Selection & obsGroupSelect) const;

        /// \brief open an obs_group_ variable, create the variable if necessary
        template<typename VarType>
        Variable openCreateVar(const std::string & varName,
                               const std::vector<std::string> & varDimList) {
            Variable var;
            if (obs_group_.vars.exists(varName)) {
                var = obs_group_.vars.open(varName);
            } else {
                // Create a vector of the dimension variables
                //
                // TODO(srh) For now use a default chunk size (10000) for the chunk size
                // in the creation parameters when the first dimension is Location.
                // This is being done since the size of location can vary across MPI tasks,
                // and we need it to be constant for the parallel io to work properly.
                // The assigned chunk size may need to be optimized further than using a
                // rough guess of 10000.
                std::vector<ioda::Dimensions_t> chunkDims;
                std::vector<Variable> varDims;
                for (auto & dimName : varDimList) {
                    Variable dimVar = obs_group_.vars.open(dimName);
                    if (dimName == "Location") {
                        chunkDims.push_back(VarUtils::DefaultChunkSize);
                    } else {
                        chunkDims.push_back(dimVar.getDimensions().dimsCur[0]);
                    }
                    varDims.push_back(dimVar);
                }

                // Create the variable. Use the JEDI internal missing value marks for
                // fill values.
                VarType fillVal = this->getFillValue<VarType>();
                VariableCreationParameters params;
                params.chunk = true;
                params.setChunks(chunkDims);
                params.compressWithGZIP();
                params.setFillValue<VarType>(fillVal);

                var = obs_group_.vars.createWithScales<VarType>(varName, varDims, params);
            }
            return var;
        }

        /// \brief fill in the channel number to channel index map
        void fillChanNumToIndexMap();

        /// \brief split off the channel number suffix from a given variable name
        /// \details If the given variable name does not exist, the channelSelect vector
        ///          is empty, and the given variable name has a suffix matching
        ///          "_[0-9][0-9]*" (ie, a numeric suffix), then this routine will strip
        ///          off the channel number from the name and place that channel number
        ///          into the ouput canSelectToUse vector. The new name will be returned
        ///          in the nameToUse string.
        ///          This is being done for backward compatibility until the ufo Variables
        ///          class and its clients are modified to handle a single variable name
        ///          and a vector of channel numbers.
        /// \param group Name of Group in obs_group_
        /// \param name Name of Variable in group
        /// \param selectChan Vector of channel numbers for selection
        /// \param varName Name of Variable after splitting off the channel number
        /// \param skipDerived
        ///   By default, this function will search for the variable `name` both in the group
        ///   `group` and `"Derived" + group`. Set this parameter to `true` to search only in the
        ///   group `group`.
        void splitChanSuffix(const std::string & group, const std::string & name,
                             const std::vector<int> & chanSelect, std::string & nameToUse,
                             std::vector<int> & chanSelectToUse,
                             bool skipDerived = false) const;

        /// \brief Extend the given variable
        /// \param extendVar database variable to be extended
        /// \param upperBoundOnGlobalNumOriginalRecs upper bound, across all processors,
        ///        of the number of records in the original ObsSpace.
        template <typename DataType>
        void extendVariable(Variable & extendVar, const size_t upperBoundOnGlobalNumOriginalRecs);
    };

}  // namespace ioda

#endif  // OBSSPACE_H_
