/*
 * (C) Copyright 2018 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef UFO_OBSSPACE_H_
#define UFO_OBSSPACE_H_

#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>

#include "oops/util/Printable.h"

#include "ioda/ObsSpaceBase.h"
#include "ioda/Locations.h"
#include "ioda/ObsVector.h"

// Forward declarations
namespace eckit {
  class Configuration;
}

namespace ioda {
  class Locations;
  class ObsVector;
  class ObsSpaceBase;

// -----------------------------------------------------------------------------

class ObsSpace : public util::Printable,
                    private boost::noncopyable {
 public:
  // Constructor, Destructor
  ObsSpace(const eckit::Configuration &, const util::DateTime &, const util::DateTime &);
  ~ObsSpace();

  // Assimilation window
  const util::DateTime & windowStart() const;
  const util::DateTime & windowEnd() const;

  // Others
  const eckit::Configuration & config() const;
  Locations* locations(const util::DateTime &, const util::DateTime &) const;
  void generateDistribution(const eckit::Configuration &);
  void printJo(const ObsVector &, const ObsVector &);

  const std::string & obsname() const;
  int nobs() const;

  void getdb(const std::string &, int &) const;
  void putdb(const std::string &, const int &) const;

  double * get_mdata(const std::string &) const;

 private:
  void print(std::ostream &) const;

  boost::scoped_ptr<ObsSpaceBase> ospace_;

};

// -----------------------------------------------------------------------------

}  // namespace ioda

#endif  // UFO_OBSSPACE_H_
