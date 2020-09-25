/*
 * (C) Copyright 2017-2019 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/ObsVector.h"

#include <math.h>
#include <limits>

#include "eckit/config/LocalConfiguration.h"
#include "ioda/ObsSpace.h"
#include "oops/base/Variables.h"
#include "oops/util/abor1_cpp.h"
#include "oops/util/Logger.h"
#include "oops/util/missingValues.h"
#include "oops/util/Random.h"

namespace ioda {
// -----------------------------------------------------------------------------
ObsVector::ObsVector(ObsSpace & obsdb,
                     const std::string & name, const bool fail)
  : obsdb_(obsdb), obsvars_(obsdb.obsvariables()),
    nvars_(obsvars_.variables().size()), nlocs_(obsdb_.nlocs()),
    values_(nlocs_ * nvars_),
    missing_(util::missingValue(missing_)) {
  oops::Log::trace() << "ObsVector::ObsVector " << name << std::endl;
  if (!name.empty()) this->read(name, fail);
}
// -----------------------------------------------------------------------------
ObsVector::ObsVector(const ObsVector & other)
  : obsdb_(other.obsdb_), obsvars_(other.obsvars_), nvars_(other.nvars_),
    nlocs_(other.nlocs_), values_(nlocs_ * nvars_), missing_(other.missing_) {
  values_ = other.values_;
  oops::Log::trace() << "ObsVector copied " << std::endl;
}
// -----------------------------------------------------------------------------
ObsVector::ObsVector(ObsSpace & obsdb, const ObsVector & other)
  : obsdb_(obsdb), obsvars_(other.obsvars_), nvars_(other.nvars_),
    nlocs_(obsdb.localobs().size()), values_(nlocs_ * nvars_), missing_(other.missing_) {
  for (size_t ii = 0; ii < nlocs_; ++ii) {
    for (size_t vv = 0; vv < nvars_; ++vv) {
      values_[ii*nvars_ + vv] = other.values_[obsdb.localobs()[ii]*nvars_ + vv];
    }
  }
  oops::Log::trace() << "Local ObsVector copied " << std::endl;
}
// -----------------------------------------------------------------------------
ObsVector::~ObsVector() {
}
// -----------------------------------------------------------------------------
ObsVector & ObsVector::operator= (const ObsVector & rhs) {
  values_ = rhs.values_;
  return *this;
}
// -----------------------------------------------------------------------------
ObsVector & ObsVector::operator*= (const double & zz) {
  for (size_t jj = 0; jj < values_.size() ; ++jj) {
    if (values_[jj] != missing_) {
      values_[jj] = zz * values_[jj];
    }
  }
  return *this;
}
// -----------------------------------------------------------------------------
ObsVector & ObsVector::operator+= (const ObsVector & rhs) {
  const size_t nn = values_.size();
  ASSERT(rhs.values_.size() == nn);
  for (size_t jj = 0; jj < nn ; ++jj) {
    if (values_[jj] == missing_ || rhs.values_[jj] == missing_) {
      values_[jj] = missing_;
    } else {
      values_[jj] += rhs.values_[jj];
    }
  }
  return *this;
}
// -----------------------------------------------------------------------------
ObsVector & ObsVector::operator-= (const ObsVector & rhs) {
  const size_t nn = values_.size();
  ASSERT(rhs.values_.size() == nn);
  for (size_t jj = 0; jj < nn ; ++jj) {
    if (values_[jj] == missing_ || rhs.values_[jj] == missing_) {
      values_[jj] = missing_;
    } else {
      values_[jj] -= rhs.values_[jj];
    }
  }
  return *this;
}
// -----------------------------------------------------------------------------
ObsVector & ObsVector::operator*= (const ObsVector & rhs) {
  const size_t nn = values_.size();
  ASSERT(rhs.values_.size() == nn);
  for (size_t jj = 0; jj < nn ; ++jj) {
    if (values_[jj] == missing_ || rhs.values_[jj] == missing_) {
      values_[jj] = missing_;
    } else {
      values_[jj] *= rhs.values_[jj];
    }
  }
  return *this;
}
// -----------------------------------------------------------------------------
ObsVector & ObsVector::operator/= (const ObsVector & rhs) {
  const size_t nn = values_.size();
  ASSERT(rhs.values_.size() == nn);
  for (size_t jj = 0; jj < nn ; ++jj) {
    if (values_[jj] == missing_ || rhs.values_[jj] == missing_) {
      values_[jj] = missing_;
    } else {
      values_[jj] /= rhs.values_[jj];
    }
  }
  return *this;
}
// -----------------------------------------------------------------------------
void ObsVector::zero() {
  for (size_t jj = 0; jj < values_.size() ; ++jj) {
    values_[jj] = 0.0;
  }
}
// -----------------------------------------------------------------------------
void ObsVector::axpy(const double & zz, const ObsVector & rhs) {
  const size_t nn = values_.size();
  ASSERT(rhs.values_.size() == nn);
  for (size_t jj = 0; jj < nn ; ++jj) {
    if (values_[jj] == missing_ || rhs.values_[jj] == missing_) {
      values_[jj] = missing_;
    } else {
      values_[jj] += zz * rhs.values_[jj];
    }
  }
}
// -----------------------------------------------------------------------------
void ObsVector::invert() {
  for (size_t jj = 0; jj < values_.size() ; ++jj) {
    if (values_[jj] != missing_) {
      values_[jj] = 1.0 / values_[jj];
    }
  }
}
// -----------------------------------------------------------------------------
void ObsVector::random() {
  util::NormalDistribution<double> x(values_.size(), 0.0, 1.0, this->getSeed());
  for (size_t jj = 0; jj < values_.size() ; ++jj) {
    values_[jj] = x[jj];
  }
}
// -----------------------------------------------------------------------------
double ObsVector::dot_product_with(const ObsVector & other) const {
  const size_t nn = values_.size();
  ASSERT(other.values_.size() == nn);
  double zz = 0.0;
  for (size_t jj = 0; jj < nn ; ++jj) {
    if (values_[jj] != missing_ && other.values_[jj] != missing_) {
      zz += values_[jj] * other.values_[jj];
    }
  }

  obsdb_.sum(zz);

  return zz;
}
// -----------------------------------------------------------------------------
double ObsVector::rms() const {
  double zrms = 0.0;
  int nobs = 0;
  for (size_t jj = 0; jj < values_.size() ; ++jj) {
    if (values_[jj] != missing_) {
      zrms += values_[jj] * values_[jj];
      ++nobs;
    }
  }

  obsdb_.sum(zrms);
  obsdb_.sum(nobs);

  if (nobs > 0) zrms = sqrt(zrms / static_cast<double>(nobs));
  return zrms;
}
// -----------------------------------------------------------------------------
void ObsVector::read(const std::string & name, const bool fail) {
  oops::Log::trace() << "ObsVector::read, name = " << name << std::endl;

  // Read in the variables stored in obsvars_ from the group given by "name".
  //
  // We want to construct the vector in the order of all variable values for the
  // first location, then all variable values for the second location, etc. This
  // means that a single variable gets its values spread out across the vector
  // in intervals the size of nvars_, and that the starting point for each variable
  // in the vector is given by the index of the variable name in varnames_.

  std::size_t nlocs = obsdb_.nlocs();
  std::vector<double> tmp(nlocs);
  for (std::size_t jv = 0; jv < nvars_; ++jv) {
    if (fail || obsdb_.has(name, obsvars_.variables()[jv])) {
      obsdb_.get_db(name, obsvars_.variables()[jv], tmp);

      for (std::size_t jj = 0; jj < nlocs; ++jj) {
        std::size_t ivec = jv + (jj * nvars_);
        values_[ivec] = tmp[jj];
      }
    }
  }
}
// -----------------------------------------------------------------------------
void ObsVector::save(const std::string & name) const {
  oops::Log::trace() << "ObsVector::save, name = " << name << std::endl;

  // As noted in the read method, the order is all variables at the first location,
  // then all variables at the next location, etc.
  std::size_t nlocs = obsdb_.nlocs();
  std::size_t ivec;
  for (std::size_t jv = 0; jv < nvars_; ++jv) {
    std::vector<double> tmp(nlocs);
    for (std::size_t jj = 0; jj < tmp.size(); ++jj) {
      ivec = jv + (jj * nvars_);
      tmp[jj] = values_[ivec];
    }

    obsdb_.put_db(name, obsvars_.variables()[jv], tmp);
  }
}
// -----------------------------------------------------------------------------
void ObsVector::mask(const ObsDataVector<int> & flags) {
  oops::Log::trace() << "ObsVector::mask" << std::endl;
  ASSERT(values_.size() == flags.nvars() * flags.nlocs());
  size_t ii = 0;
  for (size_t jv = 0; jv < flags.nvars(); ++jv) {
    for (size_t jj = 0; jj < flags.nlocs(); ++jj) {
      if (flags[jv][jj] > 0) values_[ii] = missing_;
      ++ii;
    }
  }
}
// -----------------------------------------------------------------------------
unsigned int ObsVector::nobs() const {
  int nobs = 0;
  for (size_t jj = 0; jj < values_.size() ; ++jj) {
    if (values_[jj] != missing_) ++nobs;
  }
  obsdb_.sum(nobs);
  return nobs;
}
// -----------------------------------------------------------------------------
const double & ObsVector::toFortran() const {
  return values_[0];
}
// -----------------------------------------------------------------------------
double & ObsVector::toFortran() {
  return values_[0];
}
// -----------------------------------------------------------------------------
void ObsVector::print(std::ostream & os) const {
  double zmin = std::numeric_limits<double>::max();
  double zmax = std::numeric_limits<double>::lowest();
  double zrms = 0.0;
  int nobs = 0;
  for (size_t jj = 0; jj < values_.size() ; ++jj) {
    if (values_[jj] != missing_) {
      if (values_[jj] < zmin) zmin = values_[jj];
      if (values_[jj] > zmax) zmax = values_[jj];
      zrms += values_[jj] * values_[jj];
      ++nobs;
    }
  }

  obsdb_.sum(zmin);
  obsdb_.sum(zmax);
  obsdb_.sum(zrms);
  obsdb_.sum(nobs);

  if (nobs > 0) zrms = sqrt(zrms / static_cast<double>(nobs));
  os << obsdb_.obsname() << " nobs= " << nobs << " Min="
     << zmin << ", Max=" << zmax << ", RMS=" << zrms << std::endl;
}
// -----------------------------------------------------------------------------
}  // namespace ioda
