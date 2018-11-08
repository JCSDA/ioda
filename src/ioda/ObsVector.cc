/*
 * (C) Copyright 2017 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#include "ioda/ObsVector.h"

#include <math.h>
#include <limits>
#include <random>

#include "eckit/config/LocalConfiguration.h"
#include "eckit/mpi/Comm.h"
#include "oops/base/Variables.h"
#include "oops/util/abor1_cpp.h"
#include "oops/util/Logger.h"

#include "ioda/ObsSpace.h"

namespace ioda {
// -----------------------------------------------------------------------------
ObsVector::ObsVector(const ObsSpace & obsdb, const oops::Variables & vars)
  : obsdb_(obsdb), missing_(ObsSpace::missingValue()), obsvars_(vars) {
  nvars_ = obsvars_.variables().size();
  values_.resize(obsdb_.nlocs() * nvars_);
  oops::Log::debug() << "ObsVector constructed with " << nvars_
                     << " variables resulting in " << values_.size()
                     << " elements." << std::endl;
}
// -----------------------------------------------------------------------------
ObsVector::ObsVector(const ObsVector & other, const bool copy)
  : obsdb_(other.obsdb_), missing_(other.missing_), obsvars_(other.obsvars_) {
  nvars_ = obsvars_.variables().size();
  values_.resize(obsdb_.nlocs() * nvars_);
  oops::Log::debug() << "ObsVector constructed with " << nvars_
                     << " variables resulting in " << values_.size()
                     << " elements." << std::endl;

  if (copy) values_ = other.values_;
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
  static std::mt19937 generator(1);
  static std::normal_distribution<double> distribution(0.0, 1.0);
  for (size_t jj = 0; jj < values_.size() ; ++jj) {
    values_[jj] = distribution(generator);
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
  obsdb_.comm().allReduceInPlace(zz, eckit::mpi::sum());
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
  obsdb_.comm().allReduceInPlace(zrms, eckit::mpi::sum());
  obsdb_.comm().allReduceInPlace(nobs, eckit::mpi::sum());
  if (nobs > 0) zrms = sqrt(zrms / static_cast<double>(nobs));
  return zrms;
}
// -----------------------------------------------------------------------------
void ObsVector::read(const std::string & name) {
  oops::Log::trace() << "ObsVector::read, name = " <<  name << std::endl;

  // Read in the variables stored in obsvars_ from the group given by "name".
  //
  // We want to construct the vector in the order of all variable values for the
  // first location, then all variable values for the second location, etc. This
  // means that a single variable gets its values spread out across the vector
  // in intervals the size of nvars_, and that the starting point for each variable
  // in the vector is given by the index of the variable name in varnames_.
  std::size_t Nlocs = obsdb_.nlocs();
  std::size_t ivec;
  for (std::size_t i = 0; i < nvars_; ++i) {
    std::vector<double> TmpVar(Nlocs);
    obsdb_.get_db(name, obsvars_.variables()[i], Nlocs, TmpVar.data());

    for (std::size_t j = 0; j < TmpVar.size(); ++j) {
      ivec = i + (j * nvars_);
      values_[ivec] = TmpVar[j];
    }
  }
}
// -----------------------------------------------------------------------------
void ObsVector::save(const std::string & name) const {
  oops::Log::trace() << "ObsVector::save, name = " <<  name << std::endl;

  // As noted in the read method, the order is all variables at the first location,
  // then all variables at the next location, etc.
  std::size_t Nlocs = obsdb_.nlocs();
  std::size_t ivec;
  for (std::size_t i = 0; i < nvars_; ++i) {
    std::vector<double> TmpVar(Nlocs);
    for (std::size_t j = 0; j < TmpVar.size(); ++j) {
      ivec = i + (j * nvars_);
      TmpVar[j] = values_[ivec];
    }

    obsdb_.put_db(name, obsvars_.variables()[i], Nlocs, TmpVar.data());
  }
}
// -----------------------------------------------------------------------------
unsigned int ObsVector::nobs() const {
  int nobs = 0;
  for (size_t jj = 0; jj < values_.size() ; ++jj) {
    if (values_[jj] != missing_) {
      ++nobs;
    }
  }
  obsdb_.comm().allReduceInPlace(nobs, eckit::mpi::sum());
  return nobs;
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
  obsdb_.comm().allReduceInPlace(zmin, eckit::mpi::min());
  obsdb_.comm().allReduceInPlace(zmax, eckit::mpi::max());
  obsdb_.comm().allReduceInPlace(zrms, eckit::mpi::sum());
  obsdb_.comm().allReduceInPlace(nobs, eckit::mpi::sum());
  if (nobs > 0) zrms = sqrt(zrms / static_cast<double>(nobs));
  os << obsdb_.obsname() << " nobs= " << nobs << " Min="
     << zmin << ", Max=" << zmax << ", Average=" << zrms;
}
// -----------------------------------------------------------------------------

}  // namespace ioda
