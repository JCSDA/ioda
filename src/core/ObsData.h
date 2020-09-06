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
#include "ioda/io/ObsIo.h"
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

/// Observation Data
/*!
 * \brief Observation data class for IODA
 *
 * \details This class handles the memory store of observation data. It handles the transfer
 *          of data between memory and files, the distribution of obs data across multiple
 *          process elements, the filtering out of obs data that is outside the DA timing
 *          window, the transfer of data between UFO, OOPS and IODA, and data type
 *          conversion that is "missing value aware".
 *
 * During the DA run, all data transfers are done in memory. The only time file I/O is
 * invoked is during the constructor (read from the file into the obs container) and
 * optionally during the the destructor (write from obs container into the file).
 *
 * \author Stephen Herbener, Xin Zhang (JCSDA)
 */
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
    std::size_t nlocs() const;

    /// \brief return the number of records in the obs space container
    /// \details This is the number of sets of locations after applying the
    /// optional grouping.
    std::size_t nrecs() const;

    /// \brief return the number of variables in the obs space container
    std::size_t nvars() const;

    /// \brief return YAML configuration parameter: obsdatain.obsgrouping.group variable
    std::string obs_group_var() const {return obs_params_.in_file_.obsGroupVar;}

    /// \brief return YAML configuration parameter: obsdatain.obsgrouping.sort variable
    std::string obs_sort_var() const {return obs_params_.in_file_.obsSortVar;}

    /// \brief return YAML configuration parameter: obsdatain.obsgrouping.sort order
    std::string obs_sort_order() const {return obs_params_.in_file_.obsSortOrder;}

    /// \brief return the name of the obs type being stored
    const std::string & obsname() const {return obsname_;}

    /// \brief return the name of the MPI distribution
    std::string distname() const {return obs_params_.top_level_.distributionType;}


  const std::vector<std::size_t> & recnum() const;
  const std::vector<std::size_t> & index() const;

  bool has(const std::string &, const std::string &) const;
  ObsDtype dtype(const std::string &, const std::string &) const;

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

    /// \brief number of variables
    std::size_t nvars_;

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

    /// \brief current frame count for variable dimensioned along nlocs
    Dimensions_t adjusted_frame_count_;



    // ----------------------------- private functions ------------------------------
    ObsData & operator= (const ObsData &);

    /// \brief print function for oops::Printable class
    /// \param os output stream
    void print(std::ostream & os) const;

    // Initialize the database from a source (ObsIo ojbect)
    /// \brief create the in-memory obs_group_ (ObsGroup) object
    /// \param obsIo obs source object
    void createObsGroupFromObsIo(const std::shared_ptr<ObsIo> & obsIo);

    /// \brief initialize the in-memory obs_group_ (ObsGroup) object from the ObsIo source
    /// \param obsIo obs source object
    void initFromObsSource(const std::shared_ptr<ObsIo> & obsIo);

    /// \brief resize along nlocs dimension
    /// \param nlocsSize new size to either append or reset
    /// \param append when true append nlocsSize to current size, otherwise reset to nlocsSize
    void resizeNlocs(const Dimensions_t nlocsSize, const bool append);

    /// \brief true if obs source variable is dimensioned along nlocs
    /// \param obsIo obs source object
    /// \param varName obs source variable name
    bool isObsSourceVarDimByNlocs(const std::shared_ptr<ObsIo> & obsIo,
                                  const std::string & varName);

    /// \brief read in values for variable from obs source
    /// \param obsIo obs source object
    /// \param varName Name of variable in obs source object
    /// \param varValues values for variable
    template<typename VarType>
    void readObsSource(const std::shared_ptr<ObsIo> & obsIo, const std::string & varName,
                       std::vector<VarType> & varValues) {
        Variable var = obsIo->obs_group_.vars.open(varName);

        // Form the selection objects for this variable
        Selection frontendSelection;
        Selection backendSelection;
        obsIo->createFrameSelection(var, frontendSelection, backendSelection);

        // Read the variable
        var.read<VarType>(varValues, frontendSelection, backendSelection);

        // Replace source fill values with corresponding missing marks
        Variable sourceVar = obsIo->obs_group_.vars.open(varName);
        if (sourceVar.hasFillValue()) {
            detail::FillValueData_t sourceFvData = sourceVar.getFillValue();
            VarType sourceFillValue = detail::getFillValue<VarType>(sourceFvData);
            VarType varFillValue = this->getFillValue<VarType>();
            for (auto i = varValues.begin(); i != varValues.end(); ++i) {
                if (*i == sourceFillValue) { *i = varFillValue; }
            }
        }
    }

    /// \brief create a variable in the obs_group_ object based on the obs source
    /// \param obsIo obs source object
    /// \param varName Name of obs_group_ variable for obs_group_ object
    template<typename VarType>
    Variable createVarFromObsSource(const std::shared_ptr<ObsIo> & obsIo,
                                    const std::string & varName) {
        // Creation parameters. Enable chunking, compression, and set a fill
        // value based on the built in missing values marks.
        VariableCreationParameters params;
        params.chunk = true;
        params.compressWithGZIP();
        params.setFillValue<VarType>(this->getFillValue<VarType>());

        std::vector<Variable> varDims = this->setVarDimsFromObsSource(obsIo, varName);
        return obs_group_.vars.createWithScales<VarType>(varName, varDims, params);
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

    /// \brief set the vector of dimension variables for the obs_group_ variable creation
    /// \param obsIo obs source object
    /// \param varName Name of obs_group_ variable for obs_group_ object
    std::vector<Variable> setVarDimsFromObsSource(const std::shared_ptr<ObsIo> & obsIo,
                                                  const std::string & varName);

    /// \brief generate frame indices and corresponding record numbers
    /// \details This method generates a list of indices with their corresponding
    ///  record numbers, where the indices denote which locations are to be
    ///  read into this process element.
    /// \param obsIo obs source object
    /// \param frameStart start of current of frame
    /// \param frameCount size of current frame
    void genFrameIndexRecNums(const std::shared_ptr<ObsIo> & obsIo,
                              const Dimensions_t frameStart, const Dimensions_t frameCount);
   
    /// \details return true if observation is inside the DA timing window.
    /// \param obsDt Observation date time object
    bool insideTimingWindow(const util::DateTime & ObsDt);

  template<typename VarType>
  void StoreToDb(const std::string & GroupName, const std::string & VarName,
                 const std::vector<std::size_t> & VarShape,
                 const std::vector<VarType> & VarData, bool Append = false);
  void BuildSortedObsGroups();
  void createKDTree();

  template<typename VarType>
  std::vector<VarType> ApplyIndex(const std::vector<VarType> & FullData,
                                  const std::vector<std::size_t> & FullShape,
                                  const std::vector<std::size_t> & Index,
                                  std::vector<std::size_t> & IndexedShape) const;

  static std::string DesiredVarType(std::string & GroupName, std::string & FileVarType);

  // Dump the database into the output file
  void SaveToFile(const std::string & file_name, const std::size_t MaxFrameSize);
  template<typename VarType>
  void LoadFromDb(const std::string & GroupName, const std::string & VarName,
                  const std::vector<std::size_t> & VarShape, std::vector<VarType> & VarData,
                  const std::size_t Start = 0, const std::size_t Count = 0) const;

  // Return a fill value
  template<typename DataType>
  void GetFillValue(DataType & FillValue) const;


  /*! \brief KD Tree */
  std::shared_ptr<KDTree> kd_;

  /*! \brief path to input file */
  std::string filein_;

  /*! \brief path to output file */
  std::string fileout_;

  /*! \brief indexes of locations to extract from the input obs file */
  std::vector<std::size_t> indx_;

  /*! \brief record numbers associated with the location indexes */
  std::vector<std::size_t> recnums_;

  /*! \brief profile ordering */
  RecIdxMap recidx_;

  /*! \brief maps for obs grouping via integer, float or string values */
  std::map<int, std::size_t> int_obs_grouping_;
  std::map<float, std::size_t> float_obs_grouping_;
  std::map<std::string, std::size_t> string_obs_grouping_;

  /*! \brief next available record number */
  std::size_t next_rec_num_;

  /*! \brief unique record numbers */
  std::set<std::size_t> unique_rec_nums_;
};

}  // namespace ioda

#endif  // CORE_OBSDATA_H_
