/*
 * (C) Copyright 2017 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef IODA_OBSSPACE_H_
#define IODA_OBSSPACE_H_

#include <map>
#include <ostream>
#include <string>
#include <vector>

#include "eckit/mpi/Comm.h"
#include "oops/base/ObsSpaceBase.h"
#include "oops/util/DateTime.h"
#include "oops/util/Logger.h"
#include "oops/util/Printable.h"

#include "database/MultiIndexContainer.h"
#include "Fortran.h"

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

  std::size_t nobs() const;
  std::size_t nlocs() const;

  template <typename Type>
  void get_db(const std::string &, const std::string &, const std::size_t &, Type[]) const;
  template <typename Type>
  void put_db(const std::string &, const std::string &, const std::size_t &, const Type[]);

  bool has(const std::string &, const std::string &) const;

  const std::string & obsname() const {return obsname_;}
  const util::DateTime & windowStart() const {return winbgn_;}
  const util::DateTime & windowEnd() const {return winend_;}
  const eckit::mpi::Comm & comm() const {return commMPI_;}

  static double missingValue() {return missingvalue_;}

  void generateDistribution(const eckit::Configuration &);

  void printJo(const ObsVector &, const ObsVector &);  // to be removed

 private:
  void print(std::ostream &) const;

  static const double missingvalue_;
  ObsSpace & operator= (const ObsSpace &);
  std::string obsname_;
  const util::DateTime winbgn_;
  const util::DateTime winend_;
  F90odb keyOspace_;
  const eckit::mpi::Comm & commMPI_;

  static std::map < std::string, int > theObsFileCount_;

  /*! \brief number of observation values on this domain */
  std::size_t nobs_;

  /*! \brief number of locations on this domain */
  std:: size_t nlocs_;

  /*! \brief number of variables */
  std::size_t nvars_;

  /*! \brief filename and path of output */
  std::string fileout_;

  /*! \brief Multi-index container */
  ObsSpaceContainer database_;
};

}  // namespace ioda

#endif  // IODA_OBSSPACE_H_
