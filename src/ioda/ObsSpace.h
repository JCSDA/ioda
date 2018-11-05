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
#include "oops/interface/ObsSpaceBase.h"
#include "oops/util/DateTime.h"
#include "oops/util/Logger.h"
#include "oops/util/Printable.h"

#include "Fortran.h"

// Forward declarations
namespace eckit {
  class Configuration;
}

namespace ioda {
  class Locations;
  class ObsVector;

/// Wrapper around ObsHelpQG, mostly to hide the factory
class ObsSpace : public oops::ObsSpaceBase {
 public:
  ObsSpace(const eckit::Configuration &, const util::DateTime &, const util::DateTime &);
  ObsSpace(const ObsSpace &);
  ~ObsSpace();

  void getObsVector(const std::string &, std::vector<double> &) const;
  void putObsVector(const std::string &, const std::vector<double> &) const;

  Locations * locations(const util::DateTime &, const util::DateTime &) const;  // to be removed

  void generateDistribution(const eckit::Configuration &);

  const std::string & obsname() const {return obsname_;}
  const util::DateTime & windowStart() const {return winbgn_;}
  const util::DateTime & windowEnd() const {return winend_;}

  int nobs() const;
  int nlocs() const;

  template <typename T>
  void get_db(const std::string & group, const std::string & name,
              const size_t & vsize, T* vdata) const {
    ioda_obsdb_get_f90(keyOspace_, name.size(), name.c_str(), static_cast<int>(vsize), vdata);
  }

  template <typename T>
  void put_db(const std::string & group, const std::string & name,
              const size_t & vsize, T* vdata) const {
    ioda_obsdb_put_f90(keyOspace_, name.size(), name.c_str(), static_cast<int>(vsize), vdata);
  }

  const eckit::mpi::Comm & comm() const {return commMPI_;}
  void printJo(const ObsVector &, const ObsVector &);

 private:
  void print(std::ostream &) const;

  ObsSpace & operator= (const ObsSpace &);
  std::string obsname_;
  const util::DateTime winbgn_;
  const util::DateTime winend_;
  F90odb keyOspace_;
  const eckit::mpi::Comm & commMPI_;

  static std::map < std::string, int > theObsFileCount_;
};

}  // namespace ioda

#endif  // IODA_OBSSPACE_H_
