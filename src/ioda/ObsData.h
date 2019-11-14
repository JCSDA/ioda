/*
 * (C) Copyright 2017-2019 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef IODA_OBSDATA_H_
#define IODA_OBSDATA_H_

#include <map>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

#include "eckit/container/KDTree.h"
#include "eckit/geometry/Point2.h"
#include "eckit/geometry/Point3.h"
#include "eckit/geometry/UnitSphere.h"
#include "eckit/mpi/Comm.h"
#include "oops/base/ObsSpaceBase.h"
#include "oops/base/Variables.h"
#include "oops/util/DateTime.h"
#include "oops/util/Logger.h"
#include "oops/util/Printable.h"
#include "utils/IodaUtils.h"

#include "database/ObsSpaceContainer.h"
#include "distribution/Distribution.h"
#include "fileio/IodaIO.h"

// Forward declarations
namespace eckit {
  class Configuration;
}

namespace ioda {
  class ObsVector;

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
class ObsData : public oops::ObsSpaceBase {
 public:
  typedef std::map<std::size_t, std::vector<std::size_t>> RecIdxMap;
  typedef RecIdxMap::const_iterator RecIdxIter;
  struct TreeTrait {
      typedef eckit::geometry::Point3 Point;
      typedef double                  Payload;
  };
  typedef eckit::KDTreeMemory<TreeTrait> KDTree;

  ObsData(const eckit::Configuration &, const eckit::mpi::Comm &,
          const util::DateTime &, const util::DateTime &);
  /*!
   * \details Copy constructor for an ObsData object.
   */
  ObsData(const ObsData &);
  ~ObsData();

  std::size_t gnlocs() const;
  std::size_t nlocs() const;
  std::size_t nrecs() const;
  std::size_t nvars() const;
  const std::vector<std::size_t> & recnum() const;
  const std::vector<std::size_t> & index() const;

  bool has(const std::string &, const std::string &) const;

  void get_db(const std::string & group, const std::string & name,
              const std::size_t vsize, std::vector<int> & vdata) const;
  void get_db(const std::string & group, const std::string & name,
              const std::size_t vsize, std::vector<float> & vdata) const;
  void get_db(const std::string & group, const std::string & name,
              const std::size_t vsize, std::vector<double> & vdata) const;
  void get_db(const std::string & group, const std::string & name,
              const std::size_t vsize, std::vector<std::string> & vdata) const;
  void get_db(const std::string & group, const std::string & name,
              const std::size_t vsize, std::vector<util::DateTime> & vdata) const;

  void put_db(const std::string & group, const std::string & name,
              const std::size_t vsize, const std::vector<int> & vdata);
  void put_db(const std::string & group, const std::string & name,
              const std::size_t vsize, const std::vector<float> & vdata);
  void put_db(const std::string & group, const std::string & name,
              const std::size_t vsize, const std::vector<double> & vdata);
  void put_db(const std::string & group, const std::string & name,
              const std::size_t vsize, const std::vector<std::string> & vdata);
  void put_db(const std::string & group, const std::string & name,
              const std::size_t vsize, const std::vector<util::DateTime> & vdata);

  const KDTree & getKDTree();

  const RecIdxIter recidx_begin() const;
  const RecIdxIter recidx_end() const;
  bool recidx_has(const std::size_t RecNum) const;
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

  void printJo(const ObsVector &, const ObsVector &);  // to be removed

  const oops::Variables & obsvariables() const {return obsvars_;}
  const std::shared_ptr<Distribution> distribution() const { return dist_;}

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
  void InitFromFile(const std::string & filename);
  void GenMpiDistribution(const std::unique_ptr<IodaIO> & FileIO);
  void GenRecordNumbers(const std::unique_ptr<IodaIO> & FileIO,
                        std::vector<std::size_t> & Records) const;
  template<typename DATATYPE>
  static void GenRnumsFromVar(const std::vector<DATATYPE> & VarData,
                              std::vector<std::size_t> & Records);
  void ApplyTimingWindow(const std::unique_ptr<IodaIO> & FileIO);
  void BuildSortedObsGroups();
  void createKDTree();

  template<typename VarType>
  void ApplyDistIndex(std::vector<VarType> & FullData,
                      const std::vector<std::size_t> & FullShape,
                      std::vector<VarType> & IndexedData,
                      std::vector<std::size_t> & IndexedShape,
                      std::size_t & IndexedSize) const;

  static std::string DesiredVarType(std::string & GroupName, std::string & FileVarType);

  // Convert variable data types including the missing value marks
  template<typename FromType, typename ToType>
  static void ConvertVarType(const std::vector<FromType> & FromVar, std::vector<ToType> & ToVar);

  // Dump the database into the output file
  void SaveToFile(const std::string & file_name);

  // Methods for tranferring data from a variable into the database.
  template<typename VarType, typename DbType>
  void ConvertStoreToDb(const std::string & GroupName, const std::string & VarName,
                        const std::vector<std::size_t> & VarShape,
                        const std::vector<VarType> & VarData);

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

  /*! \brief KD Tree */
  std::shared_ptr<KDTree> kd_;

  /*! \brief total number of locations */
  std::size_t gnlocs_;

  /*! \brief number of locations on this domain */
  std::size_t nlocs_;

  /*! \brief number of variables */
  std::size_t nvars_;

  /*! \brief number of records */
  std::size_t nrecs_;

  /*! \brief number of file variable data type warnings */
  std::size_t nwarns_fdtype_;

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

  /*! \brief Multi-index containers */
  ObsSpaceContainer<int> int_database_;
  ObsSpaceContainer<float> float_database_;
  ObsSpaceContainer<std::string> string_database_;
  ObsSpaceContainer<util::DateTime> datetime_database_;

  /*! \brief Observation "variables" to be simulated */
  oops::Variables obsvars_;

  /*! \brief Distribution type */
  std::string distname_;

  /*! \brief Variable that location grouping is based upon */
  std::string obs_group_variable_;

  /*! \brief Variable that location group sorting is based upon */
  std::string obs_sort_variable_;

  /*! \brief Sort order for obs grouping */
  std::string obs_sort_order_;

  std::shared_ptr<Distribution> dist_;
};

}  // namespace ioda

#endif  // IODA_OBSDATA_H_
