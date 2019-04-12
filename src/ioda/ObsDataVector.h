/*
 * (C) Copyright 2018-2019 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#ifndef IODA_OBSDATAVECTOR_H_
#define IODA_OBSDATAVECTOR_H_

#include <math.h>
#include <limits>
#include <ostream>
#include <string>
#include <vector>

#include "eckit/mpi/Comm.h"
#include "ioda/ObsSpace.h"
#include "oops/base/Variables.h"
#include "oops/util/Logger.h"
#include "oops/util/missingValues.h"
#include "oops/util/ObjectCounter.h"
#include "oops/util/Printable.h"

namespace ioda {

//-----------------------------------------------------------------------------
//! ObsDataVector<DATATYPE> handles vectors of data of type DATATYPE in observation space

template<typename DATATYPE>
class ObsDataVector: public util::Printable,
                     private util::ObjectCounter<ObsDataVector<DATATYPE> > {
 public:
  static const std::string classname() {return "ioda::ObsDataVector";}

  ObsDataVector(ObsSpace &, const oops::Variables &);
  ObsDataVector(const ObsDataVector &);
  ~ObsDataVector();

  ObsDataVector & operator= (const ObsDataVector &);

  void zero();
  void mask(const ObsDataVector<int> &);

  void read(const std::string &);
  void save(const std::string &) const;

// Used by UFO but not by OOPS
  std::size_t size() const {return values_.size();}  // Size of vector in local memory
//  unsigned int nobs() const;  // Number of active observations (without missing values)
  const DATATYPE & operator[](const std::size_t ii) const {return values_.at(ii);}
  DATATYPE & operator[](const std::size_t ii) {return values_.at(ii);}
//  const std::string & obstype() const {return obsdb_.obsname();}
//  const std::vector<DATATYPE> & values() const {return values_;}
//  std::vector<DATATYPE> & values() {return values_;}

 private:
  void print(std::ostream &) const;

  ObsSpace & obsdb_;
  oops::Variables obsvars_;
  std::size_t nvars_;
  std::size_t nlocs_;
  std::vector<DATATYPE> values_;
  const DATATYPE missing_;
};

// -----------------------------------------------------------------------------
template <typename DATATYPE>
ObsDataVector<DATATYPE>::ObsDataVector(ObsSpace & obsdb, const oops::Variables & vars)
  : obsdb_(obsdb), obsvars_(vars), nvars_(obsvars_.size()),
    nlocs_(obsdb_.nlocs()), values_(nlocs_ * nvars_),
    missing_(util::missingValue(missing_)) {
  oops::Log::trace() << "ObsDataVector constructed" << std::endl;
  oops::Log::debug() << "ObsDataVector constructed with " << nvars_
                     << " variables resulting in " << values_.size()
                     << " elements." << std::endl;
}
// -----------------------------------------------------------------------------
template <typename DATATYPE>
ObsDataVector<DATATYPE>::ObsDataVector(const ObsDataVector & other)
  : obsdb_(other.obsdb_), obsvars_(other.obsvars_), nvars_(other.nvars_),
    nlocs_(other.nlocs_), values_(nlocs_ * nvars_), missing_(other.missing_) {
  oops::Log::trace() << "ObsDataVector copied" << std::endl;
  oops::Log::debug() << "ObsVector copy constructed with " << nvars_
                     << " variables resulting in " << values_.size()
                     << " elements." << std::endl;
  values_ = other.values_;
}
// -----------------------------------------------------------------------------
template <typename DATATYPE>
ObsDataVector<DATATYPE>::~ObsDataVector() {}
// -----------------------------------------------------------------------------
template <typename DATATYPE>
ObsDataVector<DATATYPE> & ObsDataVector<DATATYPE>::operator= (const ObsDataVector & rhs) {
  values_ = rhs.values_;
  return *this;
}
// -----------------------------------------------------------------------------
template <typename DATATYPE>
void ObsDataVector<DATATYPE>::zero() {
  for (size_t jj = 0; jj < values_.size(); ++jj) {
    values_.at(jj) = static_cast<DATATYPE>(0);
  }
}
// -----------------------------------------------------------------------------
template <typename DATATYPE>
void ObsDataVector<DATATYPE>::mask(const ObsDataVector<int> & flags) {
  const DATATYPE missing = util::missingValue(missing);
  ASSERT(values_.size() == flags.size());
  for (size_t jj = 0; jj < values_.size() ; ++jj) {
    if (flags[jj] > 0) values_[jj] = missing_;
  }
}
// -----------------------------------------------------------------------------
template <typename DATATYPE>
void ObsDataVector<DATATYPE>::read(const std::string & name) {
  oops::Log::trace() << "ObsVector::read, name = " <<  name << std::endl;
  std::size_t nlocs = obsdb_.nlocs();
  std::vector<DATATYPE> tmp(nlocs);
  for (std::size_t i = 0; i < nvars_; ++i) {
    obsdb_.get_db(name, obsvars_.variables()[i], nlocs, tmp.data());

    for (std::size_t j = 0; j < nlocs; ++j) {
      std::size_t ivec = i + (j * nvars_);
      values_[ivec] = tmp[j];
    }
  }
}
// -----------------------------------------------------------------------------
template <typename DATATYPE>
void ObsDataVector<DATATYPE>::save(const std::string & name) const {
  oops::Log::trace() << "ObsVector::save, name = " <<  name << std::endl;
  std::size_t nlocs = obsdb_.nlocs();
  std::size_t ivec;
  for (std::size_t i = 0; i < nvars_; ++i) {
    std::vector<DATATYPE> tmp(nlocs);
    for (std::size_t j = 0; j < tmp.size(); ++j) {
      ivec = i + (j * nvars_);
      tmp[j] = values_[ivec];
    }
    obsdb_.put_db(name, obsvars_.variables()[i], nlocs, tmp.data());
  }
}
// -----------------------------------------------------------------------------
//  template <typename DATATYPE>
//  unsigned int ObsDataVector<DATATYPE>::nobs() const {
//    const DATATYPE missing = util::missingValue(missing);
//    int nobs = 0;
//    for (size_t jj = 0; jj < values_.size() ; ++jj) {
//      if (values_[jj] != missing) ++nobs;
//    }
//    obsdb_.comm().allReduceInPlace(nobs, eckit::mpi::sum());
//    return nobs;
//  }
// -----------------------------------------------------------------------------
template <typename DATATYPE>
void ObsDataVector<DATATYPE>::print(std::ostream & os) const {
  const DATATYPE missing = util::missingValue(missing);
  DATATYPE zmin = std::numeric_limits<DATATYPE>::max();
  DATATYPE zmax = std::numeric_limits<DATATYPE>::lowest();
  int nobs = 0;
  for (size_t jj = 0; jj < values_.size() ; ++jj) {
    if (values_[jj] != missing) {
      if (values_[jj] < zmin) zmin = values_[jj];
      if (values_[jj] > zmax) zmax = values_[jj];
      ++nobs;
    }
  }
  obsdb_.comm().allReduceInPlace(zmin, eckit::mpi::min());
  obsdb_.comm().allReduceInPlace(zmax, eckit::mpi::max());
  obsdb_.comm().allReduceInPlace(nobs, eckit::mpi::sum());
  os << obsdb_.obsname() << " nobs= " << nobs
     << " Min=" << zmin << ", Max=" << zmax << std::endl;
}
// -----------------------------------------------------------------------------
}  // namespace ioda

#endif  // IODA_OBSDATAVECTOR_H_
