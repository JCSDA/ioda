/*
 * (C) Copyright 2017-2025 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/ObsVector.h"

#include <math.h>
#include <algorithm>
#include <limits>
#include <string>

#include "eckit/config/LocalConfiguration.h"
#include "ioda/distribution/DistributionUtils.h"
#include "ioda/ObsDataVector.h"
#include "ioda/ObsSpace.h"
#include "oops/base/Variables.h"
#include "oops/util/abor1_cpp.h"
#include "oops/util/Logger.h"
#include "oops/util/missingValues.h"
#include "oops/util/Random.h"

namespace ioda {
// -----------------------------------------------------------------------------
ObsVector::ObsVector(ObsSpace & obsdb,
                     const std::string & name)
                       : obsdb_(obsdb), obsvars_(obsdb.assimvariables()),
                         nvars_(obsvars_.variables().size()), nlocs_(obsdb_.nlocs()),
                         indexAppend_(nlocs_), values_(nlocs_ * nvars_),
                         missing_(util::missingValue<double>()) {
  oops::Log::trace() << "ObsVector::ObsVector " << name << std::endl;
  obsdb_.attach(*this);
  if (!name.empty()) this->read(name);
}

// -----------------------------------------------------------------------------
ObsVector::ObsVector(const ObsVector & other)
  : obsdb_(other.obsdb_), obsvars_(other.obsvars_), nvars_(other.nvars_),
    nlocs_(other.nlocs_), indexAppend_(other.indexAppend_),
    values_(nlocs_ * nvars_), missing_(other.missing_) {
  values_ = other.values_;
  obsdb_.attach(*this);
  oops::Log::trace() << "ObsVector copied " << std::endl;
}
// -----------------------------------------------------------------------------
ObsVector::ObsVector(ObsVector && other)
  : obsdb_(other.obsdb_), obsvars_(std::move(other.obsvars_)), nvars_(other.nvars_),
    nlocs_(other.nlocs_), indexAppend_(other.indexAppend_),
    values_(std::move(other.values_)), missing_(other.missing_) {
  obsdb_.detach(other);
  obsdb_.attach(*this);
  other.nvars_ = 0;
  other.nlocs_ = 0;
  other.indexAppend_ = 0;
  oops::Log::trace() << "ObsVector moved " << std::endl;
}
// -----------------------------------------------------------------------------
ObsVector::~ObsVector() {
  obsdb_.detach(*this);
}
// -----------------------------------------------------------------------------
ObsVector & ObsVector::operator= (const ObsVector & rhs) {
  ASSERT(&obsdb_ == &rhs.obsdb_);
  obsvars_ = rhs.obsvars_;
  nvars_ = rhs.nvars_;
  nlocs_ = rhs.nlocs_;
  indexAppend_ = rhs.indexAppend_;
  values_ = rhs.values_;
  return *this;
}
// -----------------------------------------------------------------------------
ObsVector & ObsVector::operator= (ObsVector && rhs) {
  ASSERT(&obsdb_ == &rhs.obsdb_);
  obsvars_ = std::move(rhs.obsvars_);
  nvars_ = rhs.nvars_;
  nlocs_ = rhs.nlocs_;
  indexAppend_ = rhs.indexAppend_;
  values_ = std::move(rhs.values_);
  rhs.nvars_ = 0;
  rhs.nlocs_ = 0;
  rhs.indexAppend_ = 0;
  obsdb_.detach(rhs);
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
void ObsVector::zeroAppended() {
  std::size_t nlocs = obsdb_.nlocs();
    for (std::size_t jv = 0; jv < nvars_; ++jv) {
      for (std::size_t jj = indexAppend_; jj < nlocs; ++jj) {
      std::size_t ivec = jv + (jj * nvars_);
      values_[ivec] = 0;
    }
  }
}
// -----------------------------------------------------------------------------
void ObsVector::ones() {
  std::fill(values_.begin(), values_.end(), 1.0);
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
void ObsVector::axpy(const std::vector<double> & beta, const ObsVector & y) {
  ASSERT(y.values_.size() == values_.size());
  ASSERT(beta.size() == nvars_);

  size_t ivec = 0;
  for (size_t jloc = 0; jloc < nlocs_; ++jloc) {
    for (size_t jvar = 0; jvar < nvars_; ++jvar, ++ivec) {
      if (values_[ivec] == missing_ || y.values_[ivec] == missing_) {
        values_[ivec] = missing_;
      } else {
        values_[ivec] += beta[jvar] * y.values_[ivec];
      }
    }
  }
}
// -----------------------------------------------------------------------------
void ObsVector::axpy_byrecord(const std::vector<double> & beta, const ObsVector & y) {
  const size_t nrecs = obsdb_.nrecs();
  ASSERT(y.values_.size() == values_.size());
  ASSERT(beta.size() == nrecs * nvars_);

  // all record numbers on this task (the values of the record numbers are global)
  const std::vector<size_t> recnums = obsdb_.recidx_all_recnums();
  size_t ivec = 0;
  for (size_t jloc = 0; jloc < nlocs_; ++jloc) {
    // rec_value is the global record number at this location
    const std::size_t rec_value = obsdb_.recnum()[jloc];
    // map between the global record numbers and the index in the records on this MPI task
    const std::size_t recidxLocal = std::distance(recnums.begin(),
                                    std::find(recnums.begin(), recnums.end(), rec_value));
    for (size_t jvar = 0; jvar < nvars_; ++jvar, ++ivec) {
      if (values_[ivec] == missing_ || y.values_[ivec] == missing_) {
        values_[ivec] = missing_;
      } else {
        values_[ivec] += beta[recidxLocal * nvars_ + jvar] * y.values_[ivec];
      }
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
void ObsVector::random(const std::string& distType, const double relvar) {
  const size_t globalnobs = obsdb_.sourceNumLocs() * nvars_;
  std::vector<double> perts(globalnobs);

  if (distType != "Normal" && distType != "InverseGamma") {
    throw eckit::BadParameter("ObsVector::random: distType must be 'Normal' or 'InverseGamma'");
  }

  if (obsdb_.comm().rank() == 0) {
    if (distType == "InverseGamma") {
      util::InverseGammaDistribution<double> x(globalnobs, 1.0, relvar, getSeed());
      perts = x.data();
    } else {  // distType == "Normal"
      util::NormalDistribution<double> x(globalnobs, 0.0, 1.0, getSeed());
      perts = x.data();
    }
  }

  obsdb_.comm().broadcast(perts, 0);

  for (size_t jloc = 0; jloc < nlocs_; ++jloc) {
    const size_t xindex = obsdb_.index()[jloc];
    for (size_t jvar = 0; jvar < nvars_; ++jvar) {
      values_[jloc * nvars_ + jvar] = perts[xindex * nvars_ + jvar];
    }
  }
}
// -----------------------------------------------------------------------------
double ObsVector::dot_product_with(const ObsVector & other) const {
  double zz = dotProduct(*obsdb_.distribution(), nvars_, values_, other.values_);
  return zz;
}
// -----------------------------------------------------------------------------
std::vector<double> ObsVector::multivar_dot_product_with(const ObsVector & other) const {
  std::vector<double> result(nvars_, 0);
  for (size_t jvar = 0; jvar < nvars_; ++jvar) {
    // fill vectors for current variable (note: if elements in values_
    // were distributed as all locs for var1; all locs for var2; etc, we
    // wouldn't need copies here).
    std::vector<double> x1(nlocs_);
    std::vector<double> x2(nlocs_);
    for (size_t jloc = 0; jloc < nlocs_; ++jloc) {
      x1[jloc] = values_[jvar + (jloc * nvars_)];
      x2[jloc] = other.values_[jvar + (jloc*nvars_)];
    }
    result[jvar] = dotProduct(*obsdb_.distribution(), 1, x1, x2);
  }
  // Communication between time subwindows is handled at oops level for `dot_product_with`,
  // but is not handled for this method which is used in ufo to compute the bias correction
  // coefficients updates. Handle it here.
  // TODO(Someone): the time communicator handling needs to only happen at the oops level, the
  // code here should not handle this at all. The code that calls this method needs refactoring.
  obsdb_.commTime().allReduceInPlace(result.begin(), result.end(), eckit::mpi::sum());
  return result;
}
// -----------------------------------------------------------------------------
std::vector<double> ObsVector::multivarrec_dot_product_with(const ObsVector & other) const {
  const size_t nrecs = obsdb_.nrecs();
  std::vector<double> result(nrecs * nvars_, 0);
  size_t recidxLocal = 0;
  // loop over records on this task; irec is for indexing within recidx_vector,
  // recidxLocal is the local record index
  for (auto irec = obsdb_.recidx_begin(); irec != obsdb_.recidx_end();
       ++irec, ++recidxLocal) {
    std::vector<size_t> rec_idx = obsdb_.recidx_vector(irec);
    const size_t nlocsInRec = rec_idx.size();
    for (size_t jvar = 0; jvar < nvars_; ++jvar) {
      // Note: no communication is needed here since the dot product is done record by
      // record, and locations within a given record cannot be split up across MPI tasks.
      for (size_t jloc = 0; jloc < nlocsInRec; ++jloc) {
        if ((values_[rec_idx[jloc] * nvars_ + jvar] != missing_) &&
            (other.values_[rec_idx[jloc] * nvars_ + jvar] != missing_)) {
          result[recidxLocal * nvars_ + jvar] += values_[rec_idx[jloc] * nvars_ + jvar] *
                                                 other.values_[rec_idx[jloc] * nvars_ + jvar];
        }
      }
    }
  }
  // Communication between time subwindows is handled at oops level for `dot_product_with`,
  // but is not handled for this method which is used in ufo to compute the bias correction
  // coefficients updates. Handle it here.
  // TODO(Someone): the time communicator handling needs to only happen at the oops level, the
  // code here should not handle this at all. The code that calls this method needs refactoring.
  obsdb_.commTime().allReduceInPlace(result.begin(), result.end(), eckit::mpi::sum());
  return result;
}
// -----------------------------------------------------------------------------
double ObsVector::rms() const {
  double zrms = dot_product_with(*this);
  int nobs = this->nobs();
  if (nobs > 0) zrms = sqrt(zrms / static_cast<double>(nobs));

  return zrms;
}
// -----------------------------------------------------------------------------
void ObsVector::doRead(const std::string & name, const std::size_t stloc) {
  oops::Log::trace() << "ObsVector::doRead, name = " << name << std::endl;

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
    obsdb_.get_db(name, obsvars_.variables()[jv], tmp);

    for (std::size_t jj = stloc; jj < nlocs; ++jj) {
      std::size_t ivec = jv + (jj * nvars_);
      values_[ivec] = tmp[jj];
    }
  }
}
// -----------------------------------------------------------------------------
void ObsVector::read(const std::string & name) {
  oops::Log::trace() << "ObsVector::Read, name = " << name << std::endl;
  this->doRead(name);
}
// -----------------------------------------------------------------------------
void ObsVector::readAppended(const std::string & name) {
  oops::Log::trace() << "ObsVector::readAppended, name = " << name << std::endl;
  this->doRead(name, indexAppend_);
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
void ObsVector::serialize(std::vector<double> & values) const {
  values.reserve(values.size() + values_.size());
  values.insert(values.end(), values_.begin(), values_.end());
}
// -----------------------------------------------------------------------------
void ObsVector::deserialize(const std::vector<double> & values, size_t & index) {
  for (size_t ii = 0; ii < values_.size(); ++ii) {
    values_[ii] = values[index];
    ++index;
  }
}
// -----------------------------------------------------------------------------
void ObsVector::maskAndSerialize(const ObsVector & mask,
                              std::vector<double> & valid_values) const {
  const size_t nn = values_.size();
  assert(mask.values_.size() == nn);
  valid_values.reserve(valid_values.size() + nn);
  for (size_t jj = 0; jj < nn; ++jj) {
    if ((mask.values_[jj] != missing_) && (values_[jj] != missing_)) {
      valid_values.push_back(values_[jj]);
    }
  }
}
// -----------------------------------------------------------------------------
std::vector<size_t> ObsVector::maskAndSerialIndices(const ObsVector & mask) const {
  const size_t nn = values_.size();
  assert(mask.values_.size() == nn);
  std::vector<size_t> indices;
  indices.reserve(nn);
  for (size_t jj = 0; jj < nn; ++jj) {
    if ((mask.values_[jj] != missing_) && (values_[jj] != missing_)) {
        indices.push_back(jj);
    }
  }
  return indices;
}
// -----------------------------------------------------------------------------
ObsVector & ObsVector::operator=(const ObsDataVector<float> & rhs) {
  oops::Log::trace() << "ObsVector::operator= start" << std::endl;
  ASSERT(&rhs.space() == &obsdb_);
  ASSERT(rhs.nlocs() == nlocs_);
  const float  fmiss = util::missingValue<float>();
  const double dmiss = util::missingValue<double>();
  size_t ii = 0;
  for (size_t jl = 0; jl < nlocs_; ++jl) {
    for (size_t jv = 0; jv < nvars_; ++jv) {
       if (rhs[this->obsvars_[jv]][jl] == fmiss) {
         values_[ii] = dmiss;
       } else {
         values_[ii] = static_cast<double>(rhs[this->obsvars_[jv]][jl]);
       }
       ++ii;
    }
  }
  oops::Log::trace() << "ObsVector::operator= done" << std::endl;

  return *this;
}
// -----------------------------------------------------------------------------
void ObsVector::mask(const ObsVector & mask) {
  const size_t nn = values_.size();
  assert(mask.values_.size() == nn);
  for (size_t jj = 0; jj < nn; ++jj) {
    if (mask[jj] == missing_) values_[jj] = missing_;
  }
}
// -----------------------------------------------------------------------------
void ObsVector::mask(const ObsDataVector<int> & mask) {
  size_t ii = 0;
  assert(mask.nlocs() == nlocs_);
  assert(mask.nvars() == nvars_);
  for (size_t jloc = 0; jloc < nlocs_; ++jloc) {
    for (size_t jvar = 0; jvar < nvars_; ++jvar) {
       if (mask[this->obsvars_[jvar]][jloc] > 0) {
         values_.at(ii) = missing_;
       }
       ++ii;
    }
  }
}
// -----------------------------------------------------------------------------
unsigned int ObsVector::nobs() const {
  int nobs = globalNumNonMissingObs(*obsdb_.distribution(), nvars_, values_);
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
void ObsVector::reduce(const std::vector<bool> & keepLocs) {
  ASSERT(keepLocs.size() == nlocs_);
  // Reducing after appending not implemented yet (but can be!)
  ASSERT(nlocs_ == indexAppend_);
  auto newEnd = std::remove_if(values_.begin(), values_.end(),
                               [&](const auto& element) {
                                  return !keepLocs[(&element - &values_[0]) / nvars_];
                               });
  values_.erase(newEnd, values_.end());
  ASSERT(values_.size() % nvars_ == 0);
  nlocs_ = values_.size() / nvars_;
  indexAppend_ = nlocs_;
}
// -----------------------------------------------------------------------------
void ObsVector::append() {
  const size_t newnlocs = obsdb_.nlocs();
  values_.reserve(newnlocs);
  values_.insert(values_.end(), (newnlocs - nlocs_) * nvars_, missing_);
  indexAppend_ = nlocs_;
  nlocs_ = newnlocs;
}
// -----------------------------------------------------------------------------
void ObsVector::print(std::ostream & os) const {
  double zmin = std::numeric_limits<double>::max();
  double zmax = std::numeric_limits<double>::lowest();
  double zrms = rms();
  int nobs = this->nobs();
  for (size_t jj = 0; jj < values_.size() ; ++jj) {
    if (values_[jj] != missing_) {
      if (values_[jj] < zmin) zmin = values_[jj];
      if (values_[jj] > zmax) zmax = values_[jj];
    }
  }

  obsdb_.distribution()->min(zmin);
  obsdb_.distribution()->max(zmax);

  if (nobs > 0) {
    os << obsdb_.obsname() << " nobs= " << nobs << " Min="
       << zmin << ", Max=" << zmax << ", RMS=" << zrms << std::endl;
  } else {
    os << obsdb_.obsname() << ": No observations." << std::endl;
  }
}
// -----------------------------------------------------------------------------
std::string ObsVector::info(const std::string & prefix) const {
  std::vector<double> mins(nvars_, std::numeric_limits<double>::max());
  std::vector<double> maxs(nvars_, std::numeric_limits<double>::lowest());
  std::vector<double> rmss(nvars_, 0.0);
  std::vector<size_t> nval(nvars_, 0);
  size_t indx = 0;
  for (size_t jloc = 0; jloc < nlocs_; ++jloc) {
    for (size_t jvar = 0; jvar < nvars_; ++jvar) {
      if (values_[indx] != missing_) {
        if (values_[indx] < mins[jvar]) mins[jvar] = values_[indx];
        if (values_[indx] > maxs[jvar]) maxs[jvar] = values_[indx];
        rmss[jvar] += values_[indx] * values_[indx];
        ++nval[jvar];
      }
      ++indx;
    }
  }

  std::stringstream ss;
  this->infoImpl(prefix, mins, maxs, rmss, nval, ss);

  return ss.str();
}
// -----------------------------------------------------------------------------
std::string ObsVector::info(const std::string & prefix, const ObsDataVector<int> & flags) const {
  ASSERT(&flags.space() == &obsdb_);
  ASSERT(flags.nlocs() == nlocs_);

  // Local stats
  std::vector<double> mins(nvars_, std::numeric_limits<double>::max());
  std::vector<double> maxs(nvars_, std::numeric_limits<double>::lowest());
  std::vector<double> rmss(nvars_, 0.0);
  std::vector<size_t> nval(nvars_, 0);
  size_t indx = 0;
  for (size_t jloc = 0; jloc < nlocs_; ++jloc) {
    for (size_t jvar = 0; jvar < nvars_; ++jvar) {
      if (flags[this->obsvars_[jvar]][jloc] == 0 && values_[indx] != missing_) {
        if (values_[indx] < mins[jvar]) mins[jvar] = values_[indx];
        if (values_[indx] > maxs[jvar]) maxs[jvar] = values_[indx];
        rmss[jvar] += values_[indx] * values_[indx];
        ++nval[jvar];
      }
      ++indx;
    }
  }

  std::stringstream ss;
  this->infoImpl(prefix, mins, maxs, rmss, nval, ss);

  return ss.str();
}
// -----------------------------------------------------------------------------
void ObsVector::infoImpl(const std::string & prefix,
                         const std::vector<double> & mins, const std::vector<double> & maxs,
                         const std::vector<double> & rmss, const std::vector<size_t> & nval,
                         std::stringstream & ss) const {
  std::string grep = prefix;
  if (!grep.empty() && std::isalnum(grep.back())) grep += ": ";

  // Local stats
  std::vector<double> stats(4 * nvars_);
  size_t ii = 0;
  for (size_t jvar = 0; jvar < nvars_; ++jvar) {
    stats[ii] = static_cast<double>(nval[jvar]);
    stats[ii+1] = rmss[jvar];
    stats[ii+2] = mins[jvar];
    stats[ii+3] = maxs[jvar];
    ii += 4;
  }

  // Gather info
  std::vector<double> allstats(4 * nvars_ * obsdb_.comm().size());
  obsdb_.comm().gather(stats, allstats, 0);

  // Global stats
  ss << std::scientific << std::setprecision(6);
  ii = 0;
  for (size_t jvar = 0; jvar < nvars_; ++jvar) {
    size_t nobs = 0;
    double zrms = 0.0;
    double zmin = std::numeric_limits<double>::max();
    double zmax = std::numeric_limits<double>::lowest();
    size_t ioff = ii;
    for (size_t jp = 0; jp < obsdb_.comm().size(); ++jp) {
      nobs += static_cast<size_t>(allstats[ioff]);
      zrms += allstats[ioff + 1];
      zmin = std::min(allstats[ioff + 2], zmin);
      zmax = std::max(allstats[ioff + 3], zmax);
      ioff += 4 * nvars_;
    }
    ii += 4;
    ss << "\n" << std::left << grep << std::setw(40) << obsdb_.obsname() + ":" + obsvars_[jvar];
    if (nobs > 0) {
      ss << std::right
         << std::setw(0) << ": Nobs=" << std::setw(9) << nobs
         << std::setw(0) << ", Min=" << std::setw(13) << zmin
         << std::setw(0) << ", Max=" << std::setw(13) << zmax
         << std::setw(0) << ", RMS=" << std::setw(13) << std::sqrt(zrms/static_cast<double>(nobs));
    } else {
      ss << ": No observations.";
    }
  }
}
// -----------------------------------------------------------------------------
}  // namespace ioda
