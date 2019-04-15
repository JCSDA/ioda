/*
 * (C) Copyright 2018-2019 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#ifndef IODA_OBSDATAROWS_H_
#define IODA_OBSDATAROWS_H_

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
//! ObsDataRows<DATATYPE> handles rows of data of type DATATYPE in observation space

template<typename DATATYPE>
class ObsDataRows: public util::Printable,
                   private util::ObjectCounter<ObsDataRows<DATATYPE> > {
 public:
  static const std::string classname() {return "ioda::ObsDataRows";}

  ObsDataRows(ObsSpace &, const std::string &, const std::string & grp = "");
  ObsDataRows(const ObsDataRows &);
  ~ObsDataRows();

  ObsDataRows & operator= (const ObsDataRows &);

  void zero();

  void read(const std::string &);
  void save(const std::string & name = "") const;

  std::size_t size() const {return values_.size();}  // Size of vector in local memory
  unsigned int nobs() const;  // Number of active observations (without missing values)
  const DATATYPE & operator[](const std::size_t ii) const {return values_.at(ii);}
  DATATYPE & operator[](const std::size_t ii) {return values_.at(ii);}
  const std::string & obstype() const {return obsdb_.obsname();}
  const std::vector<DATATYPE> & values() const {return values_;}
  std::vector<DATATYPE> & values() {return values_;}

 private:
  void print(std::ostream &) const;

  ObsSpace & obsdb_;
  std::string var_;
  std::string grp_;
  std::vector<DATATYPE> values_;
};

// -----------------------------------------------------------------------------
template <typename DATATYPE>
ObsDataRows<DATATYPE>::ObsDataRows(ObsSpace & obsdb, const std::string & var,
                                   const std::string & grp)
  : obsdb_(obsdb), var_(var), grp_(grp), values_(obsdb_.nlocs())
{
  oops::Log::debug() << "ObsDataRows " << var_ << ", group = " << grp_ << std::endl;
  if (grp_ != "") {
    obsdb_.get_db(grp_, var_, values_.size(), values_.data());
  } else {
    this->zero();
  }
  oops::Log::trace() << "ObsDataRows constructor done" << std::endl;
}
// -----------------------------------------------------------------------------
template <typename DATATYPE>
ObsDataRows<DATATYPE>::ObsDataRows(const ObsDataRows & other)
  : obsdb_(other.obsdb_), var_(other.var_), grp_(other.grp_), values_(other.values_)
{
  oops::Log::trace() << "ObsDataRows copied " << var_ << ", group = " << grp_ << std::endl;
}
// -----------------------------------------------------------------------------
template <typename DATATYPE>
ObsDataRows<DATATYPE>::~ObsDataRows() {}
// -----------------------------------------------------------------------------
template <typename DATATYPE>
ObsDataRows<DATATYPE> & ObsDataRows<DATATYPE>::operator= (const ObsDataRows & rhs) {
  values_ = rhs.values_;
  return *this;
}
// -----------------------------------------------------------------------------
template <typename DATATYPE>
void ObsDataRows<DATATYPE>::zero() {
  for (size_t jj = 0; jj < values_.size(); ++jj) {
    values_.at(jj) = static_cast<DATATYPE>(0);
  }
}
// -----------------------------------------------------------------------------
template <typename DATATYPE>
void ObsDataRows<DATATYPE>::read(const std::string & group) {
  ASSERT(group != "");
  obsdb_.get_db(group, var_, values_.size(), values_.data());
}
// -----------------------------------------------------------------------------
template <typename DATATYPE>
void ObsDataRows<DATATYPE>::save(const std::string & name) const {
  std::string group = grp_;
  if (name != "") group = name;
  ASSERT(group != "");
  obsdb_.put_db(group, var_, values_.size(), values_.data());
}
// -----------------------------------------------------------------------------
template <typename DATATYPE>
unsigned int ObsDataRows<DATATYPE>::nobs() const {
  const DATATYPE missing = util::missingValue(missing);
  int nobs = 0;
  for (size_t jj = 0; jj < values_.size() ; ++jj) {
    if (values_[jj] != missing) ++nobs;
  }
  obsdb_.comm().allReduceInPlace(nobs, eckit::mpi::sum());
  return nobs;
}
// -----------------------------------------------------------------------------
template <typename DATATYPE>
void ObsDataRows<DATATYPE>::print(std::ostream & os) const {
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

#endif  // IODA_OBSDATAROWS_H_
