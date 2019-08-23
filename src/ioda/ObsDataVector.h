/*
 * (C) Copyright 2018-2019 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#ifndef IODA_OBSDATAVECTOR_H_
#define IODA_OBSDATAVECTOR_H_

#include <cmath>
#include <limits>
#include <ostream>
#include <string>
#include <vector>

#include <boost/math/special_functions/fpclassify.hpp>

#include "eckit/exception/Exceptions.h"
#include "eckit/mpi/Comm.h"

#include "oops/base/Variables.h"
#include "oops/util/Logger.h"
#include "oops/util/missingValues.h"
#include "oops/util/ObjectCounter.h"
#include "oops/util/Printable.h"

#include "ioda/ObsSpace.h"

namespace ioda {

//-----------------------------------------------------------------------------

template<typename DATATYPE>
class ObsDataRow {
 public:
  ObsDataRow() : values_() {}
  explicit ObsDataRow(const size_t nn) : values_(nn) {}
  void resize(const size_t nn) {values_.resize(nn);}
  const DATATYPE & operator[](const size_t ii) const {return values_.at(ii);}
  DATATYPE & operator[](const size_t ii) {return values_.at(ii);}
  const DATATYPE & at(const size_t ii) const {return values_.at(ii);}
  DATATYPE & at(const size_t ii) {return values_.at(ii);}
 private:
  std::vector<DATATYPE> values_;
};

//-----------------------------------------------------------------------------
//! ObsDataVector<DATATYPE> handles vectors of data of type DATATYPE in observation space

template<typename DATATYPE>
class ObsDataVector: public util::Printable,
                     private util::ObjectCounter<ObsDataVector<DATATYPE> > {
 public:
  static const std::string classname() {return "ioda::ObsDataVector";}

  ObsDataVector(ObsSpace &, const oops::Variables &,
                const std::string & grp = "", const bool fail = true);
  ObsDataVector(ObsSpace &, const std::string &,
                const std::string & grp = "", const bool fail = true);
  ObsDataVector(const ObsDataVector &);
  ~ObsDataVector();

  ObsDataVector & operator= (const ObsDataVector &);

  void zero();
  void mask(const ObsDataVector<int> &);

  void read(const std::string &, const bool fail = true);  // only used in GNSSRO QC
  void save(const std::string &) const;

// Methods below are used by UFO but not by OOPS
  size_t nvars() const {return nvars_;}  // Size in (local) memory
  size_t nlocs() const {return nlocs_;}  // Size in (local) memory
  bool has(const std::string & vargrp) const {return obsvars_.has(vargrp);}

  const ObsDataRow<DATATYPE> & operator[](const size_t ii) const {return rows_.at(ii);}
  ObsDataRow<DATATYPE> & operator[](const size_t ii) {return rows_.at(ii);}

  const ObsDataRow<DATATYPE> & operator[](const std::string var) const
    {return rows_.at(obsvars_.find(var));}
  ObsDataRow<DATATYPE> & operator[](const std::string var) {return rows_.at(obsvars_.find(var));}

  const std::string & obstype() const {return obsdb_.obsname();}
  const oops::Variables & varnames() const {return obsvars_;}

 private:
  void print(std::ostream &) const;

  ObsSpace & obsdb_;
  oops::Variables obsvars_;
  size_t nvars_;
  size_t nlocs_;
  std::vector<ObsDataRow<DATATYPE> > rows_;
  const DATATYPE missing_;
};

// -----------------------------------------------------------------------------
bool compareFlags(const ObsDataVector<int> &, const ObsDataVector<int> &);

// -----------------------------------------------------------------------------
size_t numZero(const ObsDataVector<int> &);

// -----------------------------------------------------------------------------
template <typename DATATYPE>
ObsDataVector<DATATYPE>::ObsDataVector(ObsSpace & obsdb, const oops::Variables & vars,
                                       const std::string & grp, const bool fail)
  : obsdb_(obsdb), obsvars_(vars), nvars_(obsvars_.size()),
    nlocs_(obsdb_.nlocs()), rows_(nvars_),
    missing_(util::missingValue(missing_))
{
  oops::Log::trace() << "ObsDataVector::ObsDataVector start" << std::endl;
  for (size_t jj = 0; jj < nvars_; ++jj) {
    rows_[jj].resize(nlocs_);
  }
  if (!grp.empty()) this->read(grp, fail);
  oops::Log::trace() << "ObsDataVector::ObsDataVector done" << std::endl;
}
// -----------------------------------------------------------------------------
template <typename DATATYPE>
ObsDataVector<DATATYPE>::ObsDataVector(ObsSpace & obsdb, const std::string & var,
                                       const std::string & grp, const bool fail)
  : obsdb_(obsdb), obsvars_(std::vector<std::string>(1, var)), nvars_(1),
    nlocs_(obsdb_.nlocs()), rows_(1),
    missing_(util::missingValue(missing_))
{
  oops::Log::trace() << "ObsDataVector::ObsDataVector start" << std::endl;
  rows_[0].resize(nlocs_);
  if (!grp.empty()) this->read(grp, fail);
  oops::Log::trace() << "ObsDataVector::ObsDataVector done" << std::endl;
}
// -----------------------------------------------------------------------------
template <typename DATATYPE>
ObsDataVector<DATATYPE>::ObsDataVector(const ObsDataVector & other)
  : obsdb_(other.obsdb_), obsvars_(other.obsvars_), nvars_(other.nvars_),
    nlocs_(other.nlocs_), rows_(other.rows_), missing_(util::missingValue(missing_)) {
  oops::Log::trace() << "ObsDataVector copied" << std::endl;
}
// -----------------------------------------------------------------------------
template <typename DATATYPE>
ObsDataVector<DATATYPE>::~ObsDataVector() {}
// -----------------------------------------------------------------------------
template <typename DATATYPE>
ObsDataVector<DATATYPE> & ObsDataVector<DATATYPE>::operator= (const ObsDataVector<DATATYPE> & rhs) {
  oops::Log::trace() << "ObsDataVector::operator= start" << std::endl;
  ASSERT(&obsdb_ == &rhs.obsdb_);
  obsvars_ = rhs.obsvars_;
  nvars_ = rhs.nvars_;
  nlocs_ = rhs.nlocs_;
  rows_ = rhs.rows_;
  oops::Log::trace() << "ObsDataVector::operator= done" << std::endl;
  return *this;
}
// -----------------------------------------------------------------------------
template <typename DATATYPE>
void ObsDataVector<DATATYPE>::zero() {
  for (size_t jv = 0; jv < nvars_; ++jv) {
    for (size_t jj = 0; jj < nlocs_; ++jj) {
      rows_.at(jv).at(jj) = static_cast<DATATYPE>(0);
    }
  }
}
// -----------------------------------------------------------------------------
template <typename DATATYPE>
void ObsDataVector<DATATYPE>::mask(const ObsDataVector<int> & flags) {
  ASSERT(nvars_ == flags.nvars());
  ASSERT(nlocs_ == flags.nlocs());
  for (size_t jv = 0; jv < nvars_; ++jv) {
    for (size_t jj = 0; jj < nlocs_; ++jj) {
      if (flags[jv][jj] > 0) rows_.at(jv).at(jj) = missing_;
    }
  }
}
// -----------------------------------------------------------------------------
template <typename DATATYPE>
void ObsDataVector<DATATYPE>::read(const std::string & name, const bool fail) {
  oops::Log::trace() << "ObsDataVector::read, name = " << name << std::endl;
  const DATATYPE missing = util::missingValue(missing);
  std::vector<DATATYPE> tmp(nlocs_);

  for (size_t jv = 0; jv < nvars_; ++jv) {
    if (fail || obsdb_.has(name, obsvars_.variables()[jv])) {
      obsdb_.get_db(name, obsvars_.variables()[jv], nlocs_, tmp.data());
      for (size_t jj = 0; jj < nlocs_; ++jj) {
        rows_.at(jv).at(jj) = tmp.at(jj);
      }
    }
  }
}
// -----------------------------------------------------------------------------
template <typename DATATYPE>
void ObsDataVector<DATATYPE>::save(const std::string & name) const {
  oops::Log::trace() << "ObsDataVector::save, name = " << name << std::endl;
  std::vector<DATATYPE> tmp(nlocs_);
  for (size_t jv = 0; jv < nvars_; ++jv) {
    for (size_t jj = 0; jj < nlocs_; ++jj) {
      tmp.at(jj) = rows_.at(jv).at(jj);
    }
    obsdb_.put_db(name, obsvars_.variables()[jv], nlocs_, tmp.data());
  }
}
// -----------------------------------------------------------------------------
template <typename DATATYPE>
void ObsDataVector<DATATYPE>::print(std::ostream & os) const {
  const DATATYPE missing = util::missingValue(missing);
  for (size_t jv = 0; jv < nvars_; ++jv) {
    DATATYPE zmin = std::numeric_limits<DATATYPE>::max();
    DATATYPE zmax = std::numeric_limits<DATATYPE>::lowest();
    int nobs = 0;
    int nloc = nlocs_;
    for (size_t jj = 0; jj < nlocs_; ++jj) {
      DATATYPE zz = rows_.at(jv).at(jj);
      if (zz != missing) {
        if (zz < zmin) zmin = zz;
        if (zz > zmax) zmax = zz;
        ++nobs;
      }
    }
    obsdb_.comm().allReduceInPlace(zmin, eckit::mpi::min());
    obsdb_.comm().allReduceInPlace(zmax, eckit::mpi::max());
    obsdb_.comm().allReduceInPlace(nobs, eckit::mpi::sum());
    obsdb_.comm().allReduceInPlace(nloc, eckit::mpi::sum());
    os << obsdb_.obsname() << " " << obsvars_[jv] << " nlocs = " << nloc
       << ", nobs= " << nobs << " Min=" << zmin << ", Max=" << zmax << std::endl;
  }
}
// -----------------------------------------------------------------------------
}  // namespace ioda

#endif  // IODA_OBSDATAVECTOR_H_
