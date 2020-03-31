/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef IODA_OBSLOCGC99_H_
#define IODA_OBSLOCGC99_H_

#include <ostream>
#include <string>
#include <vector>
#include <boost/noncopyable.hpp>

#include "eckit/config/Configuration.h"
#include "oops/util/DateTime.h"
#include "oops/util/Printable.h"

#include "ioda/IodaTrait.h"

// Forward declarations
namespace ioda {
  class ObsVector;

/// ObsLocalization matrix for IODA model.

// -----------------------------------------------------------------------------
class ObsLocGC99: public util::Printable {
 public:
  static const std::string classname() {return "ioda::ObsLocGC99";}

  ObsLocGC99(const eckit::Configuration &, const ObsSpace &);
  ~ObsLocGC99();
  void multiply(ObsVector &) const;

 private:
  void print(std::ostream &) const;
  const ObsSpace & obsdb_;
  const double rscale_;
};
// -----------------------------------------------------------------------------
}  // namespace ioda

#endif  // IODA_OBSLOCGC99_H_
