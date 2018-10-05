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

// Forward declarations
namespace eckit {
  class Configuration;
}

namespace ioda {
  class Locations;
  class ObsVector;

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
  Locations * locations(const util::DateTime &, const util::DateTime &) const;
  void generateDistribution(const eckit::Configuration &);
  void printJo(const ObsVector &, const ObsVector &);

 private:
  void print(std::ostream &) const;

  boost::scoped_ptr<oops::ObsSpaceBase> ospace_;

};

// -----------------------------------------------------------------------------

}  // namespace ioda

#endif  // UFO_OBSSPACE_H_
