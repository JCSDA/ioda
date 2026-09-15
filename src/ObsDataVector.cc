/*
 * (C) Copyright 2026 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/ObsDataVector.h"

#include <cmath>
#include <limits>
#include <ostream>
#include <string>
#include <type_traits>
#include <vector>

#include <boost/math/special_functions/fpclassify.hpp>

#include "eckit/exception/Exceptions.h"

#include "oops/base/ObsVariables.h"
#include "oops/util/Logger.h"
#include "oops/util/missingValues.h"

#include "ioda/distribution/Accumulator.h"
#include "ioda/distribution/Distribution.h"
#include "ioda/distribution/DistributionUtils.h"
#include "ioda/ObsSpace.h"
#include "ioda/ObsVector.h"

namespace ioda {

namespace {

// Convert as ObsVector element (i.e., double) to a DATATYPE
//
// This helper allows ObsDataVector's interactions with ObsVector (namely, the
// ObsVector constructor and assignToExistingVariables) to compile for any of
// ObsDataVector's template types -- even std::string and util::DateTime that a
// double can't be converted to. We do this because the explicit instantiation
// of ObsDataVector instantiates all of its members, so we need to make sure that
// every member function compiles for every DATATYPE.
template <typename DATATYPE>
DATATYPE fromObsVectorValue(double value) {
  if constexpr (std::is_arithmetic_v<DATATYPE>) {
    return static_cast<DATATYPE>(value);
  } else {
    throw eckit::NotImplemented("ObsDataVector<DATATYPE>: ObsVector values cannot be "
                                "converted to a non-arithmetic DATATYPE", Here());
  }
}

}  // namespace

// -----------------------------------------------------------------------------
template <typename DATATYPE>
ObsDataVector<DATATYPE>::ObsDataVector(ObsSpace & obsdb, const oops::ObsVariables & vars,
                                       const std::string & grp, const bool fail,
                                       const bool skipDerived)
  : obsdb_(obsdb), obsvars_(vars), nvars_(obsvars_.size()),
    nlocs_(obsdb_.nlocs()), indexAppend_(nlocs_), rows_(nvars_),
    missing_(util::missingValue<DATATYPE>())
{
  oops::Log::trace() << "ObsDataVector::ObsDataVector start" << std::endl;
  obsdb_.attach(*this);
  for (size_t jj = 0; jj < nvars_; ++jj) {
    rows_[jj].resize(nlocs_);
  }
  if (!grp.empty()) this->read(grp, fail, skipDerived);
  oops::Log::trace() << "ObsDataVector::ObsDataVector done" << std::endl;
}
// -----------------------------------------------------------------------------
template <typename DATATYPE>
ObsDataVector<DATATYPE>::ObsDataVector(ObsSpace & obsdb, const std::string & var,
                                       const std::string & grp, const bool fail,
                                       const bool skipDerived)
  : obsdb_(obsdb), obsvars_(std::vector<std::string>(1, var)), nvars_(1),
    nlocs_(obsdb_.nlocs()), indexAppend_(nlocs_), rows_(1),
    missing_(util::missingValue<DATATYPE>())
{
  oops::Log::trace() << "ObsDataVector::ObsDataVector start" << std::endl;
  obsdb_.attach(*this);
  rows_[0].resize(nlocs_);
  if (!grp.empty()) this->read(grp, fail, skipDerived);
  oops::Log::trace() << "ObsDataVector::ObsDataVector done" << std::endl;
}
// -----------------------------------------------------------------------------
template <typename DATATYPE>
ObsDataVector<DATATYPE>::ObsDataVector(ObsVector & vect)
  : obsdb_(vect.space()), obsvars_(vect.varnames()), nvars_(vect.nvars()),
    nlocs_(vect.nlocs()), indexAppend_(nlocs_),
    rows_(nvars_), missing_(util::missingValue<DATATYPE>())
{
  oops::Log::trace() << "ObsDataVector::ObsDataVector ObsVector start" << std::endl;
  obsdb_.attach(*this);
  const double dmiss = util::missingValue<double>();
  for (size_t jv = 0; jv < nvars_; ++jv) {
    rows_[jv].resize(nlocs_);
  }
  size_t ii = 0;
  for (size_t jl = 0; jl < nlocs_; ++jl) {
    for (size_t jv = 0; jv < nvars_; ++jv) {
       if (vect[ii] == dmiss) {
         rows_[jv][jl] = missing_;
       } else {
         rows_[jv][jl] = fromObsVectorValue<DATATYPE>(vect[ii]);
       }
       ++ii;
    }
  }
  oops::Log::trace() << "ObsDataVector::ObsDataVector ObsVector done" << std::endl;
}
// -----------------------------------------------------------------------------
template <typename DATATYPE>
ObsDataVector<DATATYPE>::ObsDataVector(const ObsDataVector & other)
  : obsdb_(other.obsdb_), obsvars_(other.obsvars_), nvars_(other.nvars_),
    nlocs_(other.nlocs_), indexAppend_(other.indexAppend_),
    rows_(other.rows_), missing_(util::missingValue<DATATYPE>()) {
  obsdb_.attach(*this);
  oops::Log::trace() << "ObsDataVector copied" << std::endl;
}
// -----------------------------------------------------------------------------
template <typename DATATYPE>
ObsDataVector<DATATYPE>::ObsDataVector(ObsDataVector && other)
  : obsdb_(other.obsdb_), obsvars_(std::move(other.obsvars_)), nvars_(other.nvars_),
    nlocs_(other.nlocs_), indexAppend_(other.indexAppend_),
    rows_(std::move(other.rows_)), missing_(util::missingValue<DATATYPE>()) {
  obsdb_.detach(other);
  other.nvars_ = 0;
  other.nlocs_ = 0;
  obsdb_.attach(*this);
  oops::Log::trace() << "ObsDataVector moved" << std::endl;
}
// -----------------------------------------------------------------------------
template <typename DATATYPE>
ObsDataVector<DATATYPE>::~ObsDataVector() {
  obsdb_.detach(*this);
}
// -----------------------------------------------------------------------------
template <typename DATATYPE>
ObsDataVector<DATATYPE> & ObsDataVector<DATATYPE>::operator= (const ObsDataVector<DATATYPE> & rhs) {
  oops::Log::trace() << "ObsDataVector::operator= start" << std::endl;
  ASSERT(&obsdb_ == &rhs.obsdb_);
  obsvars_ = rhs.obsvars_;
  nvars_ = rhs.nvars_;
  nlocs_ = rhs.nlocs_;
  indexAppend_ = rhs.indexAppend_;
  rows_ = rhs.rows_;
  oops::Log::trace() << "ObsDataVector::operator= done" << std::endl;
  return *this;
}
// -----------------------------------------------------------------------------
template <typename DATATYPE>
ObsDataVector<DATATYPE> & ObsDataVector<DATATYPE>::operator= (ObsDataVector<DATATYPE> && rhs) {
  oops::Log::trace() << "ObsDataVector::operator= start" << std::endl;
  ASSERT(&obsdb_ == &rhs.obsdb_);
  obsvars_ = std::move(rhs.obsvars_);
  nvars_ = rhs.nvars_;
  nlocs_ = rhs.nlocs_;
  indexAppend_ = rhs.indexAppend_;
  rows_ = std::move(rhs.rows_);
  rhs.nvars_ = 0;
  rhs.nlocs_ = 0;
  rhs.indexAppend_ = 0;
  obsdb_.detach(rhs);
  oops::Log::trace() << "ObsDataVector::operator= done" << std::endl;
  return *this;
}
// -----------------------------------------------------------------------------
template <typename DATATYPE>
void ObsDataVector<DATATYPE>::zero() {
  for (size_t jv = 0; jv < nvars_; ++jv) {
    for (size_t jj = 0; jj < nlocs_; ++jj) {
      rows_.at(jv).at(jj) = DATATYPE();
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
void ObsDataVector<DATATYPE>::doRead(const std::string & name, const bool fail,
                                   const bool skipDerived, const std::size_t & stloc ) {
  oops::Log::trace() << "ObsDataVector::doRead, name = " << name << std::endl;

  // Only need to read data when nlocs_ is greater than 0.
  // e.g. if there is no obs. on current MPI task, no read needed.
  if ( nlocs_ > 0 ) {
    std::vector<DATATYPE> tmp(nlocs_);

    for (size_t jv = 0; jv < nvars_; ++jv) {
      if (fail || obsdb_.has(name, obsvars_.variables()[jv])) {
        obsdb_.get_db(name, obsvars_.variables()[jv], tmp, {}, skipDerived);
        for (size_t jj = stloc; jj < nlocs_; ++jj) {
          rows_.at(jv).at(jj) = tmp.at(jj);
        }
      }
    }
  }
}
// -----------------------------------------------------------------------------
template <typename DATATYPE>
void ObsDataVector<DATATYPE>::read(const std::string & name, const bool fail,
                                   const bool skipDerived) {
  oops::Log::trace() << "ObsDataVector::read, name = " << name << std::endl;
  this->doRead(name, fail, skipDerived);
}
// -----------------------------------------------------------------------------
template <typename DATATYPE>
void ObsDataVector<DATATYPE>::readAppended(const std::string & name, const bool fail,
                                   const bool skipDerived) {
  oops::Log::trace() << "ObsDataVector::read, name = " << name << std::endl;
  this->doRead(name, fail, skipDerived, indexAppend_);
}
// -----------------------------------------------------------------------------
template <typename DATATYPE>
void ObsDataVector<DATATYPE>::save(const std::string & name) const {
  oops::Log::trace() << "ObsDataVector::save, name = " << name << std::endl;
  std::vector<DATATYPE> tmp(nlocs_);
  for (size_t jv = 0; jv < nvars_; ++jv) {
    for (std::size_t jj = 0; jj < tmp.size(); ++jj) {
      tmp.at(jj) = rows_.at(jv).at(jj);
    }
    obsdb_.put_db(name, obsvars_.variables()[jv], tmp, obsvars_.dimList());
  }
}
// -----------------------------------------------------------------------------
template <typename DATATYPE>
void ObsDataVector<DATATYPE>::assignToExistingVariables(const ObsVector & vect) {
  oops::Log::trace() << "ObsDataVector::assignToExistingVariables start" << std::endl;
  const double dmiss = util::missingValue<double>();
  std::vector<size_t> inds(nvars_);
  for (size_t jv = 0; jv < nvars_; ++jv) {
    rows_[jv].resize(nlocs_);
    if (vect.varnames().has(obsvars_[jv])) {
      inds[jv] = vect.varnames().find(obsvars_[jv]);
    } else {
      oops::Log::trace() << "ObsDataVector var " << obsvars_[jv]
                         << " not found in ObsVector" << std::endl;
      throw eckit::BadValue("ObsDataVector var "+obsvars_[jv]+
                            " not found in ObsVector", Here());
    }
  }
  for (size_t jv = 0; jv < nvars_; ++jv) {
    for (size_t jl = 0; jl < nlocs_; ++jl) {
      size_t vectindex = jl*vect.nvars()+inds[jv];
      if (vect[vectindex] == dmiss) {
        rows_[jv][jl] = missing_;
      } else {
        rows_[jv][jl] = fromObsVectorValue<DATATYPE>(vect[vectindex]);
      }
    }
  }
  oops::Log::trace() << "ObsDataVector::assignToExistingVariables done" << std::endl;
}
// -----------------------------------------------------------------------------
template <typename DATATYPE>
void ObsDataVector<DATATYPE>::reduce(const std::vector<bool> & keepLocs) {
  ASSERT(keepLocs.size() == nlocs_);
  const size_t nlocs_new = std::count(keepLocs.begin(), keepLocs.end(), true);
  for (auto & row : rows_) {
    std::vector<DATATYPE> masked_row(nlocs_new);
    size_t iloc_new = 0;
    for (size_t iloc = 0; iloc < row.size(); ++iloc) {
      if (keepLocs[iloc]) masked_row[iloc_new++] = row[iloc];
    }
    row = std::move(masked_row);
  }
  nlocs_ = nlocs_new;
  indexAppend_ = nlocs_;
}
// -----------------------------------------------------------------------------
template <typename DATATYPE>
void ObsDataVector<DATATYPE>::append() {
  const size_t newnlocs = obsdb_.nlocs();
  for (auto & row : rows_) {
    row.reserve(newnlocs);
    row.insert(row.end(), newnlocs - nlocs_, missing_);
  }
  indexAppend_ = nlocs_;
  nlocs_ = newnlocs;
}
// -----------------------------------------------------------------------------
template <typename DATATYPE>
void ObsDataVector<DATATYPE>::zeroAppended() {
  for (size_t jv = 0; jv < nvars_; ++jv) {
    for (size_t jj = indexAppend_; jj < nlocs_; ++jj) {
      rows_.at(jv).at(jj) = DATATYPE();
    }
  }
}
// -----------------------------------------------------------------------------
/// Print statistics describing a vector \p obsdatavector of observations taken from \p obsdb
/// to the stream \p os.
///
/// This is an implementation suitable for non-numeric data. Users shouldn't need to call it
/// directly; they should call ObsDataVector::print() instead.
///
/// \see printNumericObsDataVectorStats.
template <typename DATATYPE>
void printNonnumericObsDataVectorStats(const ObsDataVector<DATATYPE> &obsdatavector,
                                       const ObsSpace &obsdb,
                                       std::ostream & os) {
  for (size_t jv = 0; jv < obsdatavector.nvars(); ++jv) {
    int nloc = obsdb.globalNumLocs();
    // collect nobs on all processors
    int nobs = globalNumNonMissingObs(*obsdb.distribution(),
                                      obsdatavector.nvars(), obsdatavector[jv]);

    os << obsdb.obsname() << " " << obsdatavector.varnames()[jv] << " nlocs = " << nloc
       << ", nobs = " << nobs << std::endl;
  }
}
// -----------------------------------------------------------------------------
/// Print statistics describing a vector \p obsdatavector of observations taken from \p obsdb
/// to the stream \p os.
///
/// This is an implementation suitable for numeric data. Users shouldn't need to call it
/// directly; they should call ObsDataVector::print() instead.
///
/// \see printNonnumericObsDataVectorStats.
template <typename DATATYPE>
void printNumericObsDataVectorStats(const ObsDataVector<DATATYPE> &obsdatavector,
                                    const ObsSpace &obsdb,
                                    std::ostream & os) {
  const DATATYPE missing = util::missingValue<DATATYPE>();
  for (size_t jv = 0; jv < obsdatavector.nvars(); ++jv) {
    DATATYPE zmin = std::numeric_limits<DATATYPE>::max();
    DATATYPE zmax = std::numeric_limits<DATATYPE>::lowest();
    std::unique_ptr<Accumulator<DATATYPE>> accumulator =
        obsdb.distribution()->createAccumulator<DATATYPE>();
    int nloc = obsdb.globalNumLocs();

    const std::vector<DATATYPE> &vector = obsdatavector[jv];
    for (size_t jj = 0; jj < obsdatavector.nlocs(); ++jj) {
      DATATYPE zz = vector.at(jj);
      if (zz != missing) {
        if (zz < zmin) zmin = zz;
        if (zz > zmax) zmax = zz;
        accumulator->addTerm(jj, zz);
      }
    }
    // collect zmin, zmax, zavg, globalNumNonMissingObs on all processors
    obsdb.distribution()->min(zmin);
    obsdb.distribution()->max(zmax);
    DATATYPE zsum = accumulator->computeResult();
    int nobs = globalNumNonMissingObs(*obsdb.distribution(), 1, vector);

    os << std::endl << obsdb.obsname() << " " << obsdatavector.varnames()[jv]
       << " nlocs = " << nloc << ", nobs = " << nobs;
    if (nobs > 0) {
      os << ", min = " << zmin << ", max = " << zmax << ", avg = " << zsum/nobs;
    } else {
      os << " : No observations.";
    }
  }
}
// -----------------------------------------------------------------------------
// Default implementation...
template <typename DATATYPE>
void ObsDataVector<DATATYPE>::print(std::ostream & os) const {
  printNonnumericObsDataVectorStats(*this, obsdb_, os);
}
// -----------------------------------------------------------------------------
// and specializations for numeric types that can be held in ioda::ObsSpace variables.
template <>
inline void ObsDataVector<double>::print(std::ostream & os) const {
  printNumericObsDataVectorStats(*this, obsdb_, os);
}
// -----------------------------------------------------------------------------
template <>
inline void ObsDataVector<float>::print(std::ostream & os) const {
  printNumericObsDataVectorStats(*this, obsdb_, os);
}
// -----------------------------------------------------------------------------
template <>
inline void ObsDataVector<int>::print(std::ostream & os) const {
  printNumericObsDataVectorStats(*this, obsdb_, os);
}
// -----------------------------------------------------------------------------
template <typename DATATYPE>
const std::string & ObsDataVector<DATATYPE>::obstype() const {
  return obsdb_.obsname();
}

// -----------------------------------------------------------------------------
// Explicit instantiations for the DATATYPEs used by JEDI. A type missing from
// this list fails at link time with an undefined reference.
// The most common types are float and int; double, std::string, util::DateTime,
// and bool each have a few uses. Some UFO types are used too, but they are
// typedefs that resolve within the same set of types.
template class ObsDataVector<float>;
template class ObsDataVector<int>;
template class ObsDataVector<double>;
template class ObsDataVector<bool>;
template class ObsDataVector<std::string>;
template class ObsDataVector<util::DateTime>;

// -----------------------------------------------------------------------------

}  // namespace ioda
