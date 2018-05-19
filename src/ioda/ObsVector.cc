/*
 * (C) Copyright 2017 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#include <math.h>

#include "oops/util/Logger.h"

#include "ObsVector.h"
#include "ObsSpace.h"
#include "Fortran.h"

namespace ioda {
// -----------------------------------------------------------------------------
ObsVector::ObsVector(const ObsSpace & obsdb)
  : obsdb_(obsdb), keyOvec_(0)
{
  int nobs = obsdb_.nobs();
  ioda_obsvec_setup_f90(keyOvec_, nobs);
}
// -----------------------------------------------------------------------------
ObsVector::ObsVector(const ObsVector & other, const bool copy)
  : obsdb_(other.obsdb_), keyOvec_(0) {
  ioda_obsvec_clone_f90(other.keyOvec_, keyOvec_);
  if (copy) {
    ioda_obsvec_assign_f90(keyOvec_, other.keyOvec_);
  } else {
    ioda_obsvec_zero_f90(keyOvec_);
  }
}
// -----------------------------------------------------------------------------
ObsVector::~ObsVector() {
  ioda_obsvec_delete_f90(keyOvec_);
}
// -----------------------------------------------------------------------------
ObsVector & ObsVector::operator= (const ObsVector & rhs) {
  const int keyOvecRhs = rhs.keyOvec_;
  ioda_obsvec_assign_f90(keyOvec_, keyOvecRhs);
  return *this;
}
// -----------------------------------------------------------------------------
ObsVector & ObsVector::operator*= (const double & zz) {
  ioda_obsvec_mul_scal_f90(keyOvec_, zz);
  return *this;
}
// -----------------------------------------------------------------------------
ObsVector & ObsVector::operator+= (const ObsVector & rhs) {
  const int keyOvecRhs = rhs.keyOvec_;
  ioda_obsvec_add_f90(keyOvec_, keyOvecRhs);
  return *this;
}
// -----------------------------------------------------------------------------
ObsVector & ObsVector::operator-= (const ObsVector & rhs) {
  const int keyOvecRhs = rhs.keyOvec_;
  ioda_obsvec_sub_f90(keyOvec_, keyOvecRhs);
  return *this;
}
// -----------------------------------------------------------------------------
ObsVector & ObsVector::operator*= (const ObsVector & rhs) {
  const int keyOvecRhs = rhs.keyOvec_;
  ioda_obsvec_mul_f90(keyOvec_, keyOvecRhs);
  return *this;
}
// -----------------------------------------------------------------------------
ObsVector & ObsVector::operator/= (const ObsVector & rhs) {
  const int keyOvecRhs = rhs.keyOvec_;
  ioda_obsvec_div_f90(keyOvec_, keyOvecRhs);
  return *this;
}
// -----------------------------------------------------------------------------
void ObsVector::zero() {
  ioda_obsvec_zero_f90(keyOvec_);
}
// -----------------------------------------------------------------------------
void ObsVector::axpy(const double & zz, const ObsVector & rhs) {
  const int keyOvecRhs = rhs.keyOvec_;
  ioda_obsvec_axpy_f90(keyOvec_, zz, keyOvecRhs);
}
// -----------------------------------------------------------------------------
void ObsVector::invert() {
  ioda_obsvec_invert_f90(keyOvec_);
}
// -----------------------------------------------------------------------------
void ObsVector::random() {
  ioda_obsvec_random_f90(keyOvec_);
}
// -----------------------------------------------------------------------------
double ObsVector::dot_product_with(const ObsVector & other) const {
  const int keyOvecOther = other.keyOvec_;
  double zz;
  ioda_obsvec_dotprod_f90(keyOvec_, keyOvecOther, zz);
  return zz;
}
// -----------------------------------------------------------------------------
double ObsVector::rms() const {
  double zz;
  ioda_obsvec_dotprod_f90(keyOvec_, keyOvec_, zz);
  int iobs;
  ioda_obsvec_nobs_f90(keyOvec_, iobs);
  zz = sqrt(zz/iobs);
  return zz;
}
// -----------------------------------------------------------------------------
void ObsVector::read(const std::string & name) {
  obsdb_.getdb(name, keyOvec_);
}
// -----------------------------------------------------------------------------
void ObsVector::save(const std::string & name) const {
  obsdb_.putdb(name, keyOvec_);
}
// -----------------------------------------------------------------------------
void ObsVector::print(std::ostream & os) const {
  double zmin, zmax, zavg;
  ioda_obsvec_minmaxavg_f90(keyOvec_, zmin, zmax, zavg);
  os << obsdb_.obsname() << " nobs= " << size()
     << " Min=" << zmin << ", Max=" << zmax << ", Average=" << zavg;
}
// -----------------------------------------------------------------------------
unsigned int ObsVector::size() const {
  int iobs;
  ioda_obsvec_nobs_f90(keyOvec_, iobs);
  unsigned int nobs(iobs);
  return nobs;
}
// -----------------------------------------------------------------------------
}  // namespace ioda 
