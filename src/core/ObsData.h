/*
 * (C) Copyright 2017-2019 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef CORE_OBSDATA_H_
#define CORE_OBSDATA_H_

#include <map>
#include <memory>
#include <ostream>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "eckit/container/KDTree.h"
#include "eckit/geometry/Point2.h"
#include "eckit/geometry/Point3.h"
#include "eckit/geometry/UnitSphere.h"
#include "eckit/mpi/Comm.h"

#include "oops/base/Variables.h"
#include "oops/util/DateTime.h"
#include "oops/util/Logger.h"
#include "oops/util/Printable.h"

#include "ioda/core/IodaUtils.h"
#include "ioda/distribution/Distribution.h"
#include "ioda/Engines/Factory.h"
#include "ioda/io/ObsFrame.h"
#include "ioda/Misc/Dimensions.h"
#include "ioda/ObsGroup.h"
#include "ioda/ObsSpaceParameters.h"
#include "ioda/Variables/Fill.h"

// Forward declarations
namespace eckit {
    class Configuration;
}

namespace ioda {
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
    class ObsData : public util::Printable {
     public:
        //---------------------------- typedefs -------------------------------
        typedef std::map<std::size_t, std::vector<std::size_t>> RecIdxMap;
        typedef RecIdxMap::const_iterator RecIdxIter;
        struct TreeTrait {
            typedef eckit::geometry::Point3 Point;
            typedef double                  Payload;
        };
        typedef eckit::KDTreeMemory<TreeTrait> KDTree;

        //---------------------------- functions ------------------------------
        /// \brief Config based constructor for an ObsData object.
        ///
        /// \details This constructor will read in from the obs file and transfer the
        /// variables into the obs container. Obs falling outside the DA timing window,
        /// specified by bgn and end, will be discarded before storing them in the
        /// obs container.
        ///
        /// \param config eckit configuration segment holding obs types specs
        /// \param bgn    DateTime object holding the start of the DA timing window
        /// \param end    DateTime object holding the end of the DA timing window
        ObsData(const eckit::Configuration &, const eckit::mpi::Comm &,
                const util::DateTime &, const util::DateTime &);
        ObsData(const ObsData &);
        ~ObsData();

        /// \details This method will return the handle to the configuration
        const eckit::Configuration & getConfig() const {return config_;}

        /// \details This method will return the start of the DA timing window
        const util::DateTime & windowStart() const {return winbgn_;}

        /// \details This method will return the end of the DA timing window
        const util::DateTime & windowEnd() const {return winend_;}

        /// \details This method will return the associated MPI communicator
        const eckit::mpi::Comm & comm() const {return commMPI_;}

        /// \brief return the number of locations in the obs source (ObsIo)
        /// \details This is the number of loctions from the input file or input
        /// obs generator.
        std::size_t gnlocs() const {return gnlocs_;}

        /// \brief return the number of locations in the obs space
        /// \details This is the number of loctions in the resulting obs space container
        /// after removing obs outside the timing window and applying MPI distribution.
        std::size_t nlocs() const {return nlocs_;}

        /// \brief return the number of records in the obs space container
        /// \details This is the number of sets of locations after applying the
        /// optional grouping.
        std::size_t nrecs() const {return nrecs_;}

        /// \brief return the number of variables in the obs space container
        std::size_t nvars() const;

        /// \brief return YAML configuration parameter: obsdatain.obsgrouping.group variable
        std::string obs_group_var() const;

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
        const std::vector<std::size_t> & index() const {return indx_;}

        /// \brief return true if group/variable exists
        bool has(const std::string &, const std::string &) const;

        /// \brief return data type for group/variable
        ObsDtype dtype(const std::string &, const std::string &) const;

        /// \brief transfer data from the obs container to vdata
        ///
        /// \details The following get_db methods are the same except for the data type
        /// of the data being transferred (integer, float, double, string, DateTime). The
        /// caller needs to allocate the memory that the vdata parameter points to
        ///
        /// \param group Name of container group (ObsValue, ObsError, MetaData, etc.)
        /// \param name  Name of container variable
        /// \param vdata Vector where container data is being transferred to
        void get_db(const std::string & group, const std::string & name,
                    std::vector<int> & vdata) const;
        void get_db(const std::string & group, const std::string & name,
                    std::vector<float> & vdata) const;
        void get_db(const std::string & group, const std::string & name,
                    std::vector<double> & vdata) const;
        void get_db(const std::string & group, const std::string & name,
                    std::vector<std::string> & vdata) const;
        void get_db(const std::string & group, const std::string & name,
                    std::vector<util::DateTime> & vdata) const;

        /// \brief transfer data from vdata to the obs container
        ///
        /// \details The following put_db methods are the same except for the data type
        /// of the data being transferred (integer, float, double, string, DateTime). The
        /// caller needs to allocate and assign the memory that the vdata parameter points to.
        ///
        /// \param group Name of container group (ObsValue, ObsError, MetaData, etc.)
        /// \param name  Name of container variable
        /// \param vdata Vector where container data is being transferred from

        void put_db(const std::string & group, const std::string & name,
                    const std::vector<int> & vdata);
        void put_db(const std::string & group, const std::string & name,
                    const std::vector<float> & vdata);
        void put_db(const std::string & group, const std::string & name,
                    const std::vector<double> & vdata);
        void put_db(const std::string & group, const std::string & name,
                    const std::vector<std::string> & vdata);
        void put_db(const std::string & group, const std::string & name,
                    const std::vector<util::DateTime> & vdata);








  KDTree & getKDTree();

  const RecIdxIter recidx_begin() const;
  const RecIdxIter recidx_end() const;
  bool recidx_has(const std::size_t RecNum) const;
  std::size_t recidx_recnum(const RecIdxIter & Irec) const;
  const std::vector<std::size_t> & recidx_vector(const RecIdxIter & Irec) const;
  const std::vector<std::size_t> & recidx_vector(const std::size_t RecNum) const;
  std::vector<std::size_t> recidx_all_recnums() const;

  void printJo(const ObsVector &, const ObsVector &);  // to be removed

  const oops::Variables & obsvariables() const {return obsvars_;}
  const std::shared_ptr<Distribution> distribution() const { return dist_;}

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

        /// \brief number of locations on this domain
        std::size_t nlocs_;

        /// \brief number of records
        std::size_t nrecs_;

        /// \brief observation data store
        ObsGroup obs_group_;

        /// \brief obs io parameters
        ObsSpaceParameters obs_params_;

        /// \brief name of obs space
        std::string obsname_;

        /// \brief Observation "variables" to be simulated
        oops::Variables obsvars_;

        /// \brief MPI distribution object
        std::shared_ptr<Distribution> dist_;

        /// \brief indexes of locations to extract from the input obs file
        std::vector<std::size_t> indx_;

        /// \brief record numbers associated with the location indexes
        std::vector<std::size_t> recnums_;

        /// \brief profile ordering
        RecIdxMap recidx_;

        /// \brief KD Tree
        std::shared_ptr<KDTree> kd_;

        // ----------------------------- private functions ------------------------------
        ObsData & operator= (const ObsData &);

        /// \brief print function for oops::Printable class
        /// \param os output stream
        void print(std::ostream & os) const;

        /// \brief Initialize the database from a source (ObsFrame ojbect)
        /// \param obsFrame obs source object
        void createObsGroupFromObsFrame(const std::shared_ptr<ObsFrame> & obsFrame);

        /// \brief Dump the database into the output file
        void saveToFile(const bool useOldFormat = false);

        /// \brief Create a data structure holding sorted records
        /// \details This method will construct a data structure that holds the
        /// location order within each group sorted by the values of the specified
        /// sort variable.
        void BuildSortedObsGroups();

        /// \brief Create a data structure holding a KD-tree based on locations
        /// \details This method creates a private KDTree class member that can be used
        /// for searching for local obs to create an ObsSpace.
        void createKDTree();

        /// \brief initialize the in-memory obs_group_ (ObsGroup) object from the ObsIo source
        /// \param obsIo obs source object
        void initFromObsSource(const std::shared_ptr<ObsFrame> & obsFrame);

        /// \brief resize along nlocs dimension
        /// \param nlocsSize new size to either append or reset
        /// \param append when true append nlocsSize to current size, otherwise reset size
        void resizeNlocs(const Dimensions_t nlocsSize, const bool append);

        /// \brief read in values for variable from obs source
        /// \param obsIo obs source object
        /// \param varName Name of variable in obs source object
        /// \param varValues values for variable
        template<typename VarType>
        void readObsSource(const std::shared_ptr<ObsFrame> & obsFrame,
            const std::string & varName, std::vector<VarType> & varValues) {
            Variable var = obsFrame->vars().open(varName);

            // Form the selection objects for this variable
            Selection frontendSelection;
            Selection backendSelection;
            obsFrame->createFrameSelection(var, frontendSelection, backendSelection);

            // Read the variable
            var.read<VarType>(varValues, frontendSelection, backendSelection);

            // Replace source fill values with corresponding missing marks
            Variable sourceVar = obsFrame->vars().open(varName);
            if (sourceVar.hasFillValue()) {
                detail::FillValueData_t sourceFvData = sourceVar.getFillValue();
                VarType sourceFillValue = detail::getFillValue<VarType>(sourceFvData);
                VarType varFillValue = this->getFillValue<VarType>();
                for (auto i = varValues.begin(); i != varValues.end(); ++i) {
                    if (*i == sourceFillValue) { *i = varFillValue; }
                }
            }
        }

        /// \brief read in values for variable from obs source
        /// \details This function is a template specialization for a string variable. It
        /// is temporary in that it is here to handle the old ioda file format where
        /// a string vector is stored as a string array in the file.
        /// \param obsIo obs source object
        /// \param varName Name of variable in obs source object
        /// \param varValues values for variable
        template<>
        void readObsSource(const std::shared_ptr<ObsFrame> & obsFrame,
            const std::string & varName, std::vector<std::string> & varValues) {
            Variable var = obsFrame->vars().open(varName);

            // Form the selection objects for this variable
            Selection frontendSelection;
            Selection backendSelection;
            obsFrame->createFrameSelection(var, frontendSelection, backendSelection);

            // Read the variable
            Dimensions_t frameCount = obsFrame->frameCount(var);
            getFrameStringVar(var, frontendSelection, backendSelection, frameCount, varValues);

            // Replace source fill values with corresponding missing marks
            Variable sourceVar = obsFrame->vars().open(varName);
            if (sourceVar.hasFillValue()) {
                detail::FillValueData_t sourceFvData = sourceVar.getFillValue();
                std::string sourceFillValue = detail::getFillValue<std::string>(sourceFvData);
                std::string varFillValue = this->getFillValue<std::string>();
                for (auto i = varValues.begin(); i != varValues.end(); ++i) {
                    if (*i == sourceFillValue) { *i = varFillValue; }
                }
            }
        }

        /// \brief store a variable in the obs_group_ object
        /// \param obsIo obs source object
        /// \param varName Name of obs_group_ variable for obs_group_ object
        /// \param varValues Values for obs_group_ variable
        template<typename VarType>
        void storeVar(const std::string & varName, std::vector<VarType> & varValues,
                      const Dimensions_t frameStart, const Dimensions_t frameCount) {
            // get the dimensions of the variable
            Variable var = obs_group_.vars.open(varName);
            Dimensions varDims = var.getDimensions();

            // front end always starts at zero, and the count for the first dimension
            // is the frame count
            std::vector<Dimensions_t> feCounts = varDims.dimsCur;
            feCounts[0] = frameCount;
            std::vector<Dimensions_t> feStarts(feCounts.size(), 0);

            // backend end starts at frameStart, and the count is the same as the
            // frontend counts (with the first dimension adjusted)
            std::vector<Dimensions_t> beCounts = feCounts;
            std::vector<Dimensions_t> beStarts(beCounts.size(), 0);
            beStarts[0] = frameStart;

            var.write<VarType>(varValues,
                Selection().extent(feCounts).select({ SelectionOperator::SET, feStarts, feCounts }),
                Selection().select({ SelectionOperator::SET, beStarts, beCounts }));
        }

        /// \brief get fill value for use in the obs_group_ object
        template<typename DataType>
        DataType getFillValue() {
            DataType fillVal = util::missingValue(fillVal);
            return fillVal;
        }

        /// \brief template specialization for string types
        template<>
        std::string getFillValue<std::string>() {
            return std::string("_fill_");
        }

        /// \brief create set of variables from source variables and lists
        /// \param srcVarContainer Has_Variables object from source
        /// \param destVarContainer Has_Variables object from destination
        /// \param dimsAttachedToVars Map containing list of attached dims for each variable
        /// \param useOldFormat If true, use old format with @ symbol for the new var names
        void createVariables(const Has_Variables & srcVarContainer,
                             Has_Variables & destVarContainer,
                             const VarDimMap & dimsAttachedToVars,
                             const bool useOldFormat = false);

        /// \brief open an obs_group_ variable, create the varialbe if necessary
        template<typename VarType>
        Variable openCreateVar(const std::string & varName) {
            Variable var;
            if (obs_group_.vars.exists(varName)) {
                var = obs_group_.vars.open(varName);
            } else {
                // Create the variable. Use the JEDI internal missing value marks for
                // fill values.
                // TODO(srh) Current put_db interface has no means for passing in which
                // TODO(srh) dimension the variable is associated with. For now assume
                // TODO(srh) all variables are 1D dimensioned by nlocs.
                VarType fillVal = this->getFillValue<VarType>();
                Variable nlocsVar = obs_group_.vars.open("nlocs");

                VariableCreationParameters params;
                params.chunk = true;
                params.compressWithGZIP();
                params.setFillValue<VarType>(fillVal);

                var = obs_group_.vars.createWithScales<VarType>(varName, { nlocsVar }, params);
            }
            return var;
        }
    };

}  // namespace ioda

#endif  // CORE_OBSDATA_H_
