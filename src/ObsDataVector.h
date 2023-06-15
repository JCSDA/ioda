/*
 * (C) Copyright 2018-2019 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef OBSDATAVECTOR_H_
#define OBSDATAVECTOR_H_

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

#include "ioda/distribution/Accumulator.h"
#include "ioda/distribution/DistributionUtils.h"
#include "ioda/ObsSpace.h"
#include "ioda/ObsVector.h"

namespace ioda {

//-----------------------------------------------------------------------------

template <typename DATATYPE> using ObsDataRow = std::vector<DATATYPE>;

//-----------------------------------------------------------------------------
//! ObsDataVector<DATATYPE> handles vectors of data of type DATATYPE in observation space

template<typename DATATYPE>
class ObsDataVector: public util::Printable,
                     private util::ObjectCounter<ObsDataVector<DATATYPE> > {
 public:
  static const std::string classname() {return "ioda::ObsDataVector";}

  ObsDataVector(ObsSpace &, const oops::Variables &,
                const std::string & grp = "", const bool fail = true,
                const bool skipDerived = false);
  ObsDataVector(ObsSpace &, const std::string &,
                const std::string & grp = "", const bool fail = true,
                const bool skipDerived = false);
  explicit ObsDataVector(ObsVector &);
  ObsDataVector(const ObsDataVector &);
  ~ObsDataVector();

  ObsDataVector & operator= (const ObsDataVector &);

  void zero();
  void mask(const ObsDataVector<int> &);

  // Read ObsDataVector.
  void read(const std::string &, const bool fail = true,
            const bool skipDerived = false);
  void save(const std::string &) const;

/// \brief   Assign to all variables of this ObsDataVector, the values in the ObsVector vect
/// \details Loop through all variables in the ObsDataVector, matching them up with variables
///          in ObsVector vect - if present in vect, copy across values into the matching
///          variables of ObsDataVector, taking care to convert missing values. Error if an
///          ObsDataVector variable is not found in vect.
/// \param[in]  vect  ObsVector whose values are to be assigned to this ObsDataVector.
  void assignToExistingVariables(const ObsVector & vect);

// Methods below are used by UFO but not by OOPS
  const ObsSpace & space() const {return obsdb_;}
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
template <typename DATATYPE>
ObsDataVector<DATATYPE>::ObsDataVector(ObsSpace & obsdb, const oops::Variables & vars,
                                       const std::string & grp, const bool fail,
                                       const bool skipDerived)
  : obsdb_(obsdb), obsvars_(vars), nvars_(obsvars_.size()),
    nlocs_(obsdb_.nlocs()), rows_(nvars_),
    missing_(util::missingValue<DATATYPE>())
{
  oops::Log::trace() << "ObsDataVector::ObsDataVector start" << std::endl;
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
    nlocs_(obsdb_.nlocs()), rows_(1),
    missing_(util::missingValue<DATATYPE>())
{
  oops::Log::trace() << "ObsDataVector::ObsDataVector start" << std::endl;
  rows_[0].resize(nlocs_);
  if (!grp.empty()) this->read(grp, fail, skipDerived);
  oops::Log::trace() << "ObsDataVector::ObsDataVector done" << std::endl;
}
// -----------------------------------------------------------------------------
template <typename DATATYPE>
ObsDataVector<DATATYPE>::ObsDataVector(ObsVector & vect)
  : obsdb_(vect.space()), obsvars_(vect.varnames()), nvars_(vect.nvars()), nlocs_(vect.nlocs()),
    rows_(nvars_), missing_(util::missingValue<DATATYPE>())
{
  oops::Log::trace() << "ObsDataVector::ObsDataVector ObsVector start" << std::endl;
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
         rows_[jv][jl] = static_cast<DATATYPE>(vect[ii]);
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
    nlocs_(other.nlocs_), rows_(other.rows_), missing_(util::missingValue<DATATYPE>()) {
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
void ObsDataVector<DATATYPE>::read(const std::string & name, const bool fail,
                                   const bool skipDerived) {
  oops::Log::trace() << "ObsDataVector::read, name = " << name << std::endl;

  // Only need to read data when nlocs_ is greater than 0.
  // e.g. if there is no obs. on current MPI task, no read needed.
  if ( nlocs_ > 0 ) {
    std::vector<DATATYPE> tmp(nlocs_);

    for (size_t jv = 0; jv < nvars_; ++jv) {
      if (fail || obsdb_.has(name, obsvars_.variables()[jv])) {
        obsdb_.get_db(name, obsvars_.variables()[jv], tmp, {}, skipDerived);
        for (size_t jj = 0; jj < nlocs_; ++jj) {
          rows_.at(jv).at(jj) = tmp.at(jj);
        }
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
    for (std::size_t jj = 0; jj < tmp.size(); ++jj) {
      tmp.at(jj) = rows_.at(jv).at(jj);
    }
    obsdb_.put_db(name, obsvars_.variables()[jv], tmp);
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
        rows_[jv][jl] = static_cast<DATATYPE>(vect[vectindex]);
      }
    }
  }
  oops::Log::trace() << "ObsDataVector::assignToExistingVariables done" << std::endl;
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
}  // namespace ioda

#endif  // OBSDATAVECTOR_H_
