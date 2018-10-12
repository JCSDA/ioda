/*
 * (C) Copyright 2018 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "eckit/config/Configuration.h"


#include "ioda/ObsSpace.h"
#include "ioda/ObsSpaceBase.h"

namespace ioda {

// -----------------------------------------------------------------------------
ObsSpace::ObsSpace(const eckit::Configuration & conf,
                   const util::DateTime & bgn, const util::DateTime & end) :
           ospace_(ObsSpaceFactory::create(conf, bgn, end)) {
}

ObsSpace::~ObsSpace() {}


// -----------------------------------------------------------------------------
const util::DateTime & ObsSpace::windowStart() const {
  return ospace_->windowStart();
}

// -----------------------------------------------------------------------------
const util::DateTime & ObsSpace::windowEnd() const {
  return ospace_->windowEnd();
}

// -----------------------------------------------------------------------------
const eckit::Configuration & ObsSpace::config() const {
  return ospace_->config();
}

// -----------------------------------------------------------------------------
Locations* ObsSpace::locations(const util::DateTime & t1, const util::DateTime & t2) const {
  return ospace_->locations(t1, t2);
}

// -----------------------------------------------------------------------------
void ObsSpace::generateDistribution(const eckit::Configuration & conf) {
  ospace_->generateDistribution(conf);
}

// -----------------------------------------------------------------------------
const std::string & ObsSpace::obsname() const {
  return ospace_->obsname();
}

// -----------------------------------------------------------------------------
int ObsSpace::nobs() const {
  return ospace_->nobs();
}

// -----------------------------------------------------------------------------
void ObsSpace::getdb(const std::string & col, int & keyData) const {
  return ospace_->getdb(col, keyData);
}

// -----------------------------------------------------------------------------
void ObsSpace::putdb(const std::string & col, const int & keyData) const {
  return ospace_->putdb(col, keyData);
}

// -----------------------------------------------------------------------------
void ObsSpace::get_mdata(const std::string & Vname, double Vdata[],
                         const int Vsize) const {
  return ospace_->get_mdata(Vname, Vdata, Vsize);
}

// -----------------------------------------------------------------------------
void ObsSpace::printJo(const ObsVector & dy, const ObsVector & grad) {
  ospace_->printJo(dy, grad);
}


// -----------------------------------------------------------------------------
void ObsSpace::print(std::ostream & os) const {
  os << *ospace_;
}

// -----------------------------------------------------------------------------

}  // namespace ioda
