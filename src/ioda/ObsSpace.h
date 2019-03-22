/*
 * (C) Copyright 2017 UCAR
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
#include "oops/util/DateTime.h"
#include "oops/util/Logger.h"
#include "oops/util/Printable.h"
#include "utils/IodaUtils.h"

#include "database/ObsSpaceContainer.h"
#include "distribution/Distribution.h"

// Forward declarations
namespace eckit {
  class Configuration;
}

namespace ioda {
  class ObsVector;

/// Observation Space
class ObsSpace : public oops::ObsSpaceBase {
 public:
  ObsSpace(const eckit::Configuration &, const util::DateTime &, const util::DateTime &);
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

  const std::string & obsname() const {return obsname_;}
  const util::DateTime & windowStart() const {return winbgn_;}
  const util::DateTime & windowEnd() const {return winend_;}
  const eckit::mpi::Comm & comm() const {return commMPI_;}

  void generateDistribution(const eckit::Configuration &);

  void printJo(const ObsVector &, const ObsVector &);  // to be removed

 private:
  void print(std::ostream &) const;

  ObsSpace & operator= (const ObsSpace &);

  // Initialize the database from the input file
  void InitFromFile(const std::string & filename, const std::string & mode,
                    const util::DateTime &, const util::DateTime &);

  template<typename VarType>
  void ApplyDistIndex(std::unique_ptr<VarType> & FullData,
                      const std::vector<std::size_t> & FullShape,
                      std::unique_ptr<VarType> & IndexedData,
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

  /*! \brief path to input file */
  std::string filein_;

  /*! \brief path to output file */
  std::string fileout_;

  /*! \brief MPI distribution object */
  std::unique_ptr<Distribution> dist_;

  /*! \brief Multi-index container */
  ObsSpaceContainer database_;
};

/*! \brief Specialized (for DateTime type) helper function for public get_db */
template <>
void ObsSpace::get_db_helper<util::DateTime>(const std::string &, const std::string &,
                                             const std::size_t &, util::DateTime[]) const;

}  // namespace ioda

#endif  // IODA_OBSSPACE_H_
