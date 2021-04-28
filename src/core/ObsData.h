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

#include "eckit/mpi/Comm.h"
#include "oops/base/Variables.h"
#include "oops/util/DateTime.h"
#include "oops/util/Logger.h"
#include "oops/util/Printable.h"
#include "ioda/core/IodaUtils.h"

#include "ioda/core/ObsSpaceContainer.h"
#include "ioda/distribution/Distribution.h"
#include "ioda/io/IodaIO.h"

// Forward declarations
namespace eckit {
  class Configuration;
}

namespace ioda {

//-------------------------------------------------------------------------------------

/// Enum type for obs variable data types
enum class ObsDtype {
  None,
  Float,
  Integer,
  String,
  DateTime
};

}  // namespace ioda

namespace ioda {

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
  typedef std::map<std::size_t, std::vector<std::size_t>> RecIdxMap;
  typedef RecIdxMap::const_iterator RecIdxIter;

  ObsData(const eckit::Configuration &, const eckit::mpi::Comm &,
          const util::DateTime &, const util::DateTime &, const eckit::mpi::Comm &);
  /*!
   * \details Copy constructor for an ObsData object.
   */
  ObsData(const ObsData &);
  ~ObsData();

  std::size_t globalNumLocs() const;
  std::size_t nlocs() const;
  std::size_t nrecs() const;
  std::size_t nvars() const;
  const std::vector<std::size_t> & recnum() const;
  const std::vector<std::size_t> & index() const;

  bool has(const std::string &, const std::string &) const;
  ObsDtype dtype(const std::string &, const std::string &) const;

  const std::vector<std::string> & obs_group_vars() const;
  std::string obs_sort_var() const;
  std::string obs_sort_order() const;

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

  const RecIdxIter recidx_begin() const;
  const RecIdxIter recidx_end() const;
  bool recidx_has(const std::size_t RecNum) const;
  bool obsAreSorted() const { return recidx_is_sorted_; }
  std::size_t recidx_recnum(const RecIdxIter & Irec) const;
  const std::vector<std::size_t> & recidx_vector(const RecIdxIter & Irec) const;
  const std::vector<std::size_t> & recidx_vector(const std::size_t RecNum) const;
  std::vector<std::size_t> recidx_all_recnums() const;

  /*! \details This method will return the name of the obs type being stored */
  const std::string & obsname() const {return obsname_;}
  /*! \details This method will return the handle to the configuration */
  const eckit::Configuration & getConfig() const {return config_;}
  /*! \details This method will return the start of the DA timing window */
  const util::DateTime & windowStart() const {return winbgn_;}
  /*! \details This method will return the end of the DA timing window */
  const util::DateTime & windowEnd() const {return winend_;}
  /*! \details This method will return the associated MPI communicator */
  const eckit::mpi::Comm & comm() const {return commMPI_;}

  const oops::Variables & obsvariables() const {return obsvars_;}
  const Distribution & distribution() const { return *dist_;}

 private:
  void print(std::ostream &) const;

  ObsData & operator= (const ObsData &);

  // Initialize the database with auto-generated locations
  void generateDistribution(const eckit::Configuration &);
  void genDistRandom(const eckit::Configuration & conf, std::vector<float> & Lats,
                     std::vector<float> & Lons, std::vector<util::DateTime> & Dtimes);
  void genDistList(const eckit::Configuration & conf, std::vector<float> & Lats,
                   std::vector<float> & Lons, std::vector<util::DateTime> & Dtimes);

  // Initialize the database from the input file
  void InitFromFile(const std::string & filename, const std::size_t MaxFrameSize);
  std::vector<std::size_t> GenFrameIndexRecNums(const std::unique_ptr<IodaIO> & FileIO,
                               const std::size_t FrameStart, const std::size_t FrameSize,
                               std::map<std::string, std::size_t> & ObsGroupingMap);
  bool InsideTimingWindow(const util::DateTime & ObsDt);
  void BuildObsGroupingKeys(const std::unique_ptr<IodaIO> & FileIO,
                            const std::vector<std::size_t> & FrameIndex,
                            std::vector<std::string> & GroupingKeys);
  void BuildSortedObsGroups();
  void BuildRecIdxUnsorted();

  template<typename VarType>
  std::vector<VarType> ApplyIndex(const std::vector<VarType> & FullData,
                                  const std::vector<std::size_t> & FullShape,
                                  const std::vector<std::size_t> & Index,
                                  std::vector<std::size_t> & IndexedShape) const;

  static std::string DesiredVarType(std::string & GroupName, std::string & FileVarType);

  void extendObsSpace(const eckit::Configuration & config);

  /*! \brief Extend all vectors in a database with missing values. */
  template <typename T>
    void extendVectorsInDatabase(const ObsSpaceContainer<T> &Db, const size_t nlocsext);

  // Dump the database into the output file
  void SaveToFile(const std::string & file_name, const std::size_t MaxFrameSize);

  /*! \brief name of obs space */
  std::string obsname_;

  /*! \brief Configuration file */
  const eckit::LocalConfiguration config_;

  /*! \brief Beginning of DA timing window */
  const util::DateTime winbgn_;

  /*! \brief End of DA timing window */
  const util::DateTime winend_;

  /*! \brief MPI communicator */
  const eckit::mpi::Comm & commMPI_;

  /*! \brief total number of locations filtered because they were outside of the time window */
  std::size_t gnlocs_outside_timewindow_;

  /*! \brief total number of locations */
  std::size_t gnlocs_;

  /*! \brief number of locations on this domain */
  std::size_t nlocs_;

  /*! \brief number of variables */
  std::size_t nvars_;

  /*! \brief number of records */
  std::size_t nrecs_;

  /*! \brief flag, file has variables with unexpected data types */
  bool file_unexpected_dtypes_;

  /*! \brief flag, file has variables with excess dimensions */
  bool file_excess_dims_;

  /*! \brief path to input file */
  std::string filein_;

  /*! \brief path to output file */
  std::string fileout_;

  /*! \brief max frame size for input file */
  std::size_t in_max_frame_size_;

  /*! \brief max frame size for output file */
  std::size_t out_max_frame_size_;

  /*! \brief indexes of locations to extract from the input obs file */
  std::vector<std::size_t> indx_;

  /*! \brief record numbers associated with the location indexes */
  std::vector<std::size_t> recnums_;

  /*! \brief profile ordering */
  RecIdxMap recidx_;

  /*! \brief indicator whether the data in recidx_ is sorted */
  bool recidx_is_sorted_;

  /*! \brief Multi-index containers */
  ObsSpaceContainer<int> int_database_;
  ObsSpaceContainer<float> float_database_;
  ObsSpaceContainer<std::string> string_database_;
  ObsSpaceContainer<util::DateTime> datetime_database_;

  /*! \brief Observation "variables" to be simulated */
  oops::Variables obsvars_;

  /*! \brief Variable that location grouping is based upon */
  std::vector<std::string> obs_group_variables_;

  /*! \brief Variable that location group sorting is based upon */
  std::string obs_sort_variable_;

  /*! \brief Sort order for obs grouping */
  std::string obs_sort_order_;

  /*! \brief MPI distribution object */
  std::shared_ptr<Distribution> dist_;

  /*! \brief next available record number */
  std::size_t next_rec_num_;

  /*! \brief unique record numbers */
  std::set<std::size_t> unique_rec_nums_;
};

}  // namespace ioda

#endif  // CORE_OBSDATA_H_
