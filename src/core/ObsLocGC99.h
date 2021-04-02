/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef CORE_OBSLOCGC99_H_
#define CORE_OBSLOCGC99_H_

#include <ostream>
#include <vector>

#include "eckit/config/Configuration.h"

#include "ioda/IodaTrait.h"
#include "ioda/ObsSpace.h"
#include "ioda/ObsVector.h"

#include "oops/generic/gc99.h"
#include "oops/util/Printable.h"

// Forward declarations
namespace ioda {

/// Horizontal Gaspari-Cohn observation space localization
template<class MODEL>
class ObsLocGC99: public util::Printable {
  typedef typename MODEL::GeometryIterator   GeometryIterator_;
 public:
  ObsLocGC99(const eckit::Configuration &, const ObsSpace &);

  /// compute Gaspari-Cohn localization, save in \p obsvector
  void computeLocalization(const GeometryIterator_ &, ObsVector & obsvector) const;

 private:
  void print(std::ostream &) const;

  /// Gaspari-Cohn horizontal localization distance (localization goes to zero at rscale_)
  const double rscale_;

  /// local obs space
  const ObsSpace & obsdb_;
};

// -----------------------------------------------------------------------------

template<typename MODEL>
ObsLocGC99<MODEL>::ObsLocGC99(const eckit::Configuration & config, const ObsSpace & obsdb)
  : rscale_(config.getDouble("lengthscale")), obsdb_(obsdb)
{
}

// -----------------------------------------------------------------------------

template<typename MODEL>
void ObsLocGC99<MODEL>::computeLocalization(const GeometryIterator_ &,
                                            ObsVector & obsvector) const {
  const std::vector<double> & obsdist = obsdb_.obsdist();
  const size_t nlocs = obsvector.nlocs();
  const size_t nvars = obsvector.nvars();

  for (size_t jloc = 0; jloc < nlocs; ++jloc) {
    double gc = oops::gc99(obsdist[jloc] / rscale_);
    // obsdist is calculated at each location; need to update R for each variable
    for (size_t jvar = 0; jvar < nvars; ++jvar) {
      obsvector[jvar + jloc * nvars] = gc;
    }
  }
}

// -----------------------------------------------------------------------------

template<typename MODEL>
void ObsLocGC99<MODEL>::print(std::ostream & os) const {
  os << "Gaspari-Cohn horizontal localization with " << rscale_ << " lengthscale" << std::endl;
}

}  // namespace ioda

#endif  // CORE_OBSLOCGC99_H_
