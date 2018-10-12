/*
 * (C) Copyright 2017 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef IODA_OBSSPACEFORT_H_
#define IODA_OBSSPACEFORT_H_

#include <map>
#include <ostream>
#include <string>

#include "oops/util/DateTime.h"
#include "oops/util/Logger.h"
#include "oops/util/Printable.h"

#include "ioda/ObsSpaceBase.h"

#include "Fortran.h"

// Forward declarations
namespace eckit {
  class Configuration;
}

namespace ioda {
  class Locations;
  class ObsVector;

/// Wrapper around ObsHelpQG, mostly to hide the factory
class ObsSpaceFort : public ObsSpaceBase {

 public:
  ObsSpaceFort(const eckit::Configuration &, const util::DateTime &, const util::DateTime &);
  ObsSpaceFort(const ObsSpaceFort &);
  ~ObsSpaceFort();

  void getdb(const std::string &, int &) const;
  void putdb(const std::string &, const int &) const; 

  void get_mdata(const std::string &, double [], const int) const;

  Locations * locations(const util::DateTime &, const util::DateTime &) const;

  void generateDistribution(const eckit::Configuration &);

  const std::string & obsname() const {return obsname_;}
  const util::DateTime & windowStart() const {return winbgn_;}
  const util::DateTime & windowEnd() const {return winend_;}

  int nobs() const;

  int & toFortran() {return keyOspace_;}
  const int & toFortran() const {return keyOspace_;}

  void printJo(const ObsVector &, const ObsVector &);

 private:
  void print(std::ostream &) const;

  ObsSpaceFort & operator= (const ObsSpaceFort &);
  std::string obsname_;
  const util::DateTime winbgn_;
  const util::DateTime winend_;
  F90odb keyOspace_;

  static std::map < std::string, int > theObsFileCount_;
};

}  // namespace ioda

#endif  // IODA_OBSSPACEFORT_H_
