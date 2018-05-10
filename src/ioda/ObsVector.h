/*
 * (C) Copyright 2017 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#ifndef IODA_OBSVECTOR_H_
#define IODA_OBSVECTOR_H_

#include <ostream>
#include <string>

#include "Fortran.h"
#include "util/ObjectCounter.h"
#include "util/Printable.h"

namespace ioda {
  class ObsSpace;

// -----------------------------------------------------------------------------
/// ObsVector class to handle vectors in observation space for IODA.

class ObsVector : public util::Printable,
                  private util::ObjectCounter<ObsVector> {
 public:
  static const std::string classname() {return "ioda::ObsVector";}

  explicit ObsVector(const ObsSpace &);
  ObsVector(const ObsVector &, const bool copy = true);
  ~ObsVector();

  ObsVector & operator = (const ObsVector &);
  ObsVector & operator*= (const double &);
  ObsVector & operator+= (const ObsVector &);
  ObsVector & operator-= (const ObsVector &);
  ObsVector & operator*= (const ObsVector &);
  ObsVector & operator/= (const ObsVector &);

  void zero();
  void axpy(const double &, const ObsVector &);
  void invert();
  void random();
  double dot_product_with(const ObsVector &) const;
  double rms() const;

  unsigned int size() const;

  int & toFortran() {return keyOvec_;}
  const int & toFortran() const {return keyOvec_;}

// I/O
  void read(const std::string &);
  void save(const std::string &) const;

 private:
  void print(std::ostream &) const;

  const ObsSpace & obsdb_;
  F90ovec keyOvec_;
};
// -----------------------------------------------------------------------------

}  // namespace ioda

#endif  // IODA_OBSVECTOR_H_
