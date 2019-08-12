/*
 * (C) Copyright 2017-2019 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef IODA_OBSSPACE_H_
#define IODA_OBSSPACE_H_

#include <map>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

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

/// Observation Space
/*!
 * \brief Observation space class for IODA
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
class ObsSpace : public oops::ObsSpaceBase {
 public:
  ObsSpace(const eckit::Configuration &, const util::DateTime &, const util::DateTime &);
  /*!
   * \details Copy constructor for an ObsSpace object.
   */
  ObsSpace(const ObsSpace &);
  ~ObsSpace();

  std::size_t nlocs() const;
  std::size_t nrecs() const;
  std::size_t nvars() const;

  bool has(const std::string &, const std::string &) const;

  void get_db(const std::string & group, const std::string & name,
              const size_t & vsize, int vdata[]) const;
  void get_db(const std::string & group, const std::string & name,
              const size_t & vsize, float vdata[]) const;
  void get_db(const std::string & group, const std::string & name,
              const size_t & vsize, double vdata[]) const;
  void get_db(const std::string & group, const std::string & name,
              const size_t & vsize, util::DateTime vdata[]) const;

  void put_db(const std::string & group, const std::string & name,
              const size_t & vsize, const int vdata[]);
  void put_db(const std::string & group, const std::string & name,
              const size_t & vsize, const float vdata[]);
  void put_db(const std::string & group, const std::string & name,
              const size_t & vsize, const double vdata[]);
  void put_db(const std::string & group, const std::string & name,
              const size_t & vsize, const util::DateTime vdata[]);

  /*! \details This method will return the name of the obs type being stored */
  const std::string & obsname() const {return obsname_;}
  /*! \details This method will return the start of the DA timing window */
  const util::DateTime & windowStart() const {return winbgn_;}
  /*! \details This method will return the end of the DA timing window */
  const util::DateTime & windowEnd() const {return winend_;}
  /*! \details This method will return the associated MPI communicator */
  const eckit::mpi::Comm & comm() const {return commMPI_;}

  void generateDistribution(const eckit::Configuration &);

  void printJo(const ObsVector &, const ObsVector &);  // to be removed

  const oops::Variables & obsvariables() const {return obsvars_;}

 private:
  void print(std::ostream &) const;

  ObsSpace & operator= (const ObsSpace &);

  // Initialize the database from the input file
  void InitFromFile(const std::string & filename);
  void GenGroupNumbers(const std::unique_ptr<IodaIO> & Fid,
                       std::vector<std::size_t> & Groups);
  template<typename DATATYPE>
  void GenGnumsFromVar(const std::vector<DATATYPE> & VarData,
                       std::vector<std::size_t> & Groups);

  template<typename VarType>
  void ApplyDistIndex(std::unique_ptr<VarType[]> & FullData,
                      const std::vector<std::size_t> & FullShape,
                      std::unique_ptr<VarType[]> & IndexedData,
                      std::vector<std::size_t> & IndexedShape, std::size_t & IndexedSize);

  std::string DesiredVarType(std::string & GroupName, std::string & FileVarType);

  // Convert variable data types including the missing value marks
  template<typename FromType, typename ToType>
  void ConvertVarType(const FromType * FromVar, ToType * ToVar,
                      const std::size_t & VarSize) const;

  // Dump the database into the output file
  void SaveToFile(const std::string & file_name);

  // Methods for tranferring data from the database into a variable.
  template <typename DATATYPE>
  void get_db_helper(const std::string &, const std::string &,
                     const std::size_t &, DATATYPE[]) const;

  template<typename DbType, typename VarType>
  void LoadFromDbConvert(const std::string & GroupName, const std::string & VarName,
                    const std::vector<std::size_t> & VarShape, const std::size_t & VarSize,
                    VarType * VarData) const;

  // Methods for tranferring data from a variable into the database.
  template <typename DATATYPE>
  void put_db_helper(const std::string &, const std::string &,
                     const std::size_t &, const DATATYPE[]);

  template<typename VarType, typename DbType>
  void ConvertStoreToDb(const std::string & GroupName, const std::string & VarName,
                    const std::vector<std::size_t> & VarShape, const std::size_t & VarSize,
                    const VarType * VarData);

  /*! \brief name of obs space */
  std::string obsname_;

  /*! \brief Beginning of DA timing window */
  const util::DateTime winbgn_;

  /*! \brief End of DA timing window */
  const util::DateTime winend_;

  /*! \brief MPI communicator */
  const eckit::mpi::Comm & commMPI_;

  /*! \brief number of locations in the input file */
  std::size_t file_nlocs_;

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

  /*! \brief Multi-index container */
  ObsSpaceContainer database_;

  /*! \brief Observation "variables" to be simulated */
  oops::Variables obsvars_;

  /*! \brief Distribution type */
  std::string distname_;

  /*! \brief Variable that location grouping is based upon */
  // element zero is group name, element 1 is variable name
  std::vector<std::string> obs_grouping_;
};

/*! \brief Specialized (for DateTime type) helper function for public get_db */
template <>
void ObsSpace::get_db_helper<util::DateTime>(const std::string &, const std::string &,
                                             const std::size_t &, util::DateTime[]) const;

}  // namespace ioda

#endif  // IODA_OBSSPACE_H_
