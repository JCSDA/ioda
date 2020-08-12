/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/core/ObsLocGC99.h"

#include "eckit/config/Configuration.h"
#include "eckit/exception/Exceptions.h"

#include "ioda/ObsSpace.h"
#include "ioda/ObsVector.h"

#include "oops/generic/gc99.h"
#include "oops/interface/ObsLocalization.h"

// -----------------------------------------------------------------------------
namespace ioda {

// -----------------------------------------------------------------------------

ObsLocGC99::ObsLocGC99(const eckit::Configuration & config, const ObsSpace & obsdb)
  : obsdb_(obsdb),
    rscale_(config.getDouble("lengthscale"))
{
}

// -----------------------------------------------------------------------------

ObsLocGC99::~ObsLocGC99() {}

// -----------------------------------------------------------------------------

void ObsLocGC99::multiply(ObsVector & dy) const {
  const std::vector<double> & obsdist = obsdb_.obsdist();
  const size_t nlocs = dy.nlocs();
  const size_t nvars = dy.nvars();
  for (size_t jloc = 0; jloc < nlocs; ++jloc) {
    double gc = oops::gc99(obsdist[jloc] / rscale_);
    // obsdist is calculated at each location; need to update R for each variable
    for (size_t jvar = 0; jvar < nvars; ++jvar) {
      dy[jvar + jloc * nvars] *= gc;
    }
  }
}

// -----------------------------------------------------------------------------

void ObsLocGC99::print(std::ostream & os) const {
  os << "Gaspari-Cohn localization with " << rscale_ << " lengthscale" << std::endl;
}

// -----------------------------------------------------------------------------

}  // namespace ioda
