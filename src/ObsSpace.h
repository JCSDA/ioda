/*
 * (C) Copyright 2017-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
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

#include "eckit/exception/Exceptions.h"
#include "eckit/mpi/Comm.h"

#include "oops/base/ObsSpaceBase.h"
#include "oops/base/Variables.h"
#include "oops/util/DateTime.h"
#include "oops/util/Logger.h"
#include "ioda/core/IodaUtils.h"
#include "ioda/distribution/Distribution.h"
#include "ioda/Engines/Factory.h"
#include "ioda/Misc/Dimensions.h"
#include "ioda/ObsGroup.h"
#include "ioda/ObsSpaceParameters.h"
#include "ioda/Variables/Fill.h"

// Forward declarations
namespace eckit {
    class Configuration;
}

namespace ioda {
    class ObsFrameRead;
    class ObsVector;

    //-------------------------------------------------------------------------------------
    // Enum type for obs variable data types
    enum class ObsDtype {
        None,
        Float,
        Integer,
        String,
        DateTime
    };

    // Enum type for obs dimension ids
    // The first two dimension names for now are nlocs and nchans. This will likely expand
    // in the future, so make sure that this enum class and the following initializer
    // function stay in sync.
    enum class ObsDimensionId {
        Nlocs,
        Nchans
    };

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

        //---------------------------- functions ------------------------------
        /// \brief Config based constructor for an ObsSpace object.
        ///
        /// \details This constructor will read in from the obs file and transfer the
        /// variables into the obs container. Obs falling outside the DA timing window,
        /// specified by bgn and end, will be discarded before storing them in the
        /// obs container.
        ///
        /// \param config eckit configuration segment holding obs types specs
        /// \param comm MPI communicator for model grouping
        /// \param bgn DateTime object holding the start of the DA timing window
        /// \param end DateTime object holding the end of the DA timing window
        /// \param timeComm MPI communicator for ensemble
        ObsSpace(const eckit::Configuration & config, const eckit::mpi::Comm & comm,
                const util::DateTime & bgn, const util::DateTime & end,
                const eckit::mpi::Comm & timeComm);
        ObsSpace(const ObsSpace &);
        virtual ~ObsSpace() {}

        /// \details This method will return the start of the DA timing window
        const util::DateTime & windowStart() const {return winbgn_;}

        /// \details This method will return the end of the DA timing window
        const util::DateTime & windowEnd() const {return winend_;}

        /// \details This method will return the associated MPI communicator
        const eckit::mpi::Comm & comm() const {return commMPI_;}

        /// \details This method will return the associated parameters
        const ObsSpaceParameters & params() const {return obs_params_;}

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

        /// \brief return the total number of locations in the corresponding obs spaces
        ///        across all MPI tasks
        std::size_t globalNumLocs() const {return gnlocs_;}

        /// \brief return number of locations from obs source that were outside the time window
        std::size_t globalNumLocsOutsideTimeWindow() const {return gnlocs_outside_timewindow_;}

        /// \brief return the number of locations in the obs space.
        /// Note that nlocs may be smaller than global unique nlocs due to distribution of obs
        /// across multiple process elements.
        inline size_t nlocs() const { return get_dim_size(ObsDimensionId::Nlocs); }

        /// \brief return the number of channels in the container. If this is not a radiance
        /// obs type, then this will return zero.
        inline size_t nchans() const { return get_dim_size(ObsDimensionId::Nchans); }

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

        /// \brief return YAML configuration parameter: obsdatain.obsgrouping.sort order
        std::string obs_sort_order() const;

        /// \brief return the name of the obs type being stored
        const std::string & obsname() const {return obsname_;}

        /// \brief return the name of the MPI distribution
        std::string distname() const {return obs_params_.top_level_.distName;}

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

        /// \brief return true if group/variable exists
        bool has(const std::string & group, const std::string & name) const;

        /// \brief return data type for group/variable
        /// \param group Group name containting the variable
        /// \param name Variable name
        ObsDtype dtype(const std::string & group, const std::string & name) const;

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
        void get_db(const std::string & group, const std::string & name,
                    std::vector<int> & vdata,
                    const std::vector<int> & chanSelect = { }) const;
        void get_db(const std::string & group, const std::string & name,
                    std::vector<float> & vdata,
                    const std::vector<int> & chanSelect = { }) const;
        void get_db(const std::string & group, const std::string & name,
                    std::vector<double> & vdata,
                    const std::vector<int> & chanSelect = { }) const;
        void get_db(const std::string & group, const std::string & name,
                    std::vector<std::string> & vdata,
                    const std::vector<int> & chanSelect = { }) const;
        void get_db(const std::string & group, const std::string & name,
                    std::vector<util::DateTime> & vdata,
                    const std::vector<int> & chanSelect = { }) const;

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
                    const std::vector<std::string> & dimList = { "nlocs" });
        void put_db(const std::string & group, const std::string & name,
                    const std::vector<float> & vdata,
                    const std::vector<std::string> & dimList = { "nlocs" });
        void put_db(const std::string & group, const std::string & name,
                    const std::vector<double> & vdata,
                    const std::vector<std::string> & dimList = { "nlocs" });
        void put_db(const std::string & group, const std::string & name,
                    const std::vector<std::string> & vdata,
                    const std::vector<std::string> & dimList = { "nlocs" });
        void put_db(const std::string & group, const std::string & name,
                    const std::vector<util::DateTime> & vdata,
                    const std::vector<std::string> & dimList = { "nlocs" });

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

        /// \brief return oops variables object (simulated variables)
       const oops::Variables & obsvariables() const {return obsvars_;}

       /// \brief return MPI distribution object
       std::shared_ptr<const Distribution> distribution() const { return dist_;}

     private:
        // ----------------------------- private data members ---------------------------
        /// \brief Configuration file
        const eckit::LocalConfiguration config_;

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

        /// \brief Observation "variables" to be simulated
        oops::Variables obsvars_;

        /// \brief MPI distribution object
        std::shared_ptr<const Distribution> dist_;

        /// \brief indexes of locations to extract from the input obs file
        std::vector<std::size_t> indx_;

        /// \brief record numbers associated with the location indexes
        std::vector<std::size_t> recnums_;

        /// \brief profile ordering
        RecIdxMap recidx_;

        /// \brief indicator whether the data in recidx_ is sorted
        bool recidx_is_sorted_;

        /// \brief map showing association of dim names with each variable name
        VarDimMap dims_attached_to_vars_;

        /// \brief cache for frontend selection
        std::map<std::vector<std::string>, Selection> known_fe_selections_;

        /// \brief cache for backend selection
        std::map<std::vector<std::string>, Selection> known_be_selections_;

        /// \brief disable the "=" operator
        ObsSpace & operator= (const ObsSpace &) = delete;

        // ----------------------------- private functions ------------------------------
        /// \brief print function for oops::Printable class
        /// \param os output stream
        void print(std::ostream & os) const;

        /// \brief Initialize the database from a source (ObsFrame ojbect)
        /// \param obsFrame obs source object
        void createObsGroupFromObsFrame(ObsFrameRead & obsFrame);

        /// \brief Extend the ObsSpace according to the method requested in
        ///  the configuration file.
        /// \param params object containing specs for extending the ObsSpace
        void extendObsSpace(const ObsExtendParameters & params);

        /// \brief Dump the database into the output file
        void saveToFile();

        /// \brief Create the recidx data structure holding sorted record groups
        /// \details This method will construct a data structure that holds the
        /// location order within each group sorted by the values of the specified
        /// sort variable.
        void buildSortedObsGroups();

        /// \brief Create the recidx data structure with unsorted record groups
        /// \details This method will initialize the recidx structure without
        /// any particular ordering of the record groups.
        void buildRecIdxUnsorted();

        /// \brief initialize the in-memory obs_group_ (ObsGroup) object from the ObsIo source
        /// \param obsIo obs source object
        void initFromObsSource(ObsFrameRead & obsFrame);

        /// \brief resize along nlocs dimension
        /// \param nlocsSize new size to either append or reset
        /// \param append when true append nlocsSize to current size, otherwise reset size
        void resizeNlocs(const Dimensions_t nlocsSize, const bool append);

        /// \brief read in values for variable from obs source
        /// \param obsFrame obs frame object
        /// \param varName Name of variable in obs source object
        /// \param varValues values for variable
        template<typename VarType>
        bool readObsSource(ObsFrameRead & obsFrame,
                           const std::string & varName, std::vector<VarType> & varValues);

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
        ///          is only used when the variable is 2D radiance data (nlocs X nchans),
        ///          and contains a list of channel numbers to be selected from the
        ///          obs_group_ variable.
        /// \param group Name of Group in obs_group_
        /// \param name Name of Variable in group
        /// \param selectChan Vector of channel numbers for selection
        /// \param varValues memory to load from obs_group_ variable
        template<typename VarType>
        void loadVar(const std::string & group, const std::string & name,
                     const std::vector<int> & chanSelect,
                     std::vector<VarType> & varValues) const;

        /// \brief save a variable to the obs_group_ object
        /// \param group Name of Group in obs_group_
        /// \param name Name of Variable in group.
        /// \param varValues values to be saved
        /// \param dimList Vector of dimension names (for creating variable if needed)
        ///
        /// If the group `group` does not contain a variable with the specified name, but this name
        /// has the form <string>_<integer> and `obs_group_` contains an `nchans` dimension, this
        /// function will save `varValues` in the slice of variable <string> corresponding to
        /// channel <integer>. If channel <integer> does not exist or the variable <string> already
        /// exists but is not associated with the `nchans` dimension, an exception will be thrown.
        template<typename VarType>
        void saveVar(const std::string & group, std::string name,
                     const std::vector<VarType> & varValues,
                     const std::vector<std::string> & dimList);

        /// \brief Create selections of slices of the variable \p variable along dimension
        /// \p nchansDimIndex corresponding to channels \p channels.
        ///
        /// \returns The number of elements in each selection.
        std::size_t createChannelSelections(const Variable & variable,
                                            std::size_t nchansDimIndex,
                                            const std::vector<int> & channels,
                                            Selection & memSelect,
                                            Selection & obsGroupSelect) const;

        /// \brief create set of variables from source variables and lists
        /// \param srcVarContainer Has_Variables object from source
        /// \param destVarContainer Has_Variables object from destination
        /// \param dimsAttachedToVars Map containing list of attached dims for each variable
        void createVariables(const Has_Variables & srcVarContainer,
                             Has_Variables & destVarContainer,
                             const VarDimMap & dimsAttachedToVars);

        /// \brief open an obs_group_ variable, create the varialbe if necessary
        template<typename VarType>
        Variable openCreateVar(const std::string & varName,
                               const std::vector<std::string> & varDimList) {
            Variable var;
            if (obs_group_.vars.exists(varName)) {
                var = obs_group_.vars.open(varName);
            } else {
                // Create a vector of the dimension variables
                std::vector<Variable> varDims;
                for (auto & dimName : varDimList) {
                    varDims.push_back(obs_group_.vars.open(dimName));
                }

                // Create the variable. Use the JEDI internal missing value marks for
                // fill values.
                VarType fillVal = this->getFillValue<VarType>();
                VariableCreationParameters params;
                params.chunk = true;
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
        void splitChanSuffix(const std::string & group, const std::string & name,
                     const std::vector<int> & chanSelect, std::string & nameToUse,
                     std::vector<int> & chanSelectToUse) const;

        /// \brief Extend the given variable
        /// \param extendVar database variable to be extended
        /// \param startFill nlocs index indicating the start of the extended region
        template <typename DataType>
        void extendVariable(Variable & extendVar, const size_t startFill);
    };

}  // namespace ioda

#endif  // OBSSPACE_H_
