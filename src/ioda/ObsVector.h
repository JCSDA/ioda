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
#include <vector>

#include "ioda/ObsDataVector.h"
#include "ioda/ObsSpace.h"
#include "oops/base/Variables.h"
#include "oops/util/ObjectCounter.h"
#include "oops/util/Printable.h"

namespace ioda {

//-----------------------------------------------------------------------------
/*! \brief ObsVector class to handle vectors in observation space for IODA
 *
 *  \details This class holds observation vector data. Examples of an obs vector
 *           are the y vector and the H(x) vector. The methods of this class
 *           that implement vector operations (e.g., bitwise add, bitwise subtract,
 *           dot product) are capable of handling missing values in the obs data.
 */

class ObsVector : public util::Printable,
                  private util::ObjectCounter<ObsVector> {
 public:
  static const std::string classname() {return "ioda::ObsVector";}

  ObsVector(ObsSpace &,
            const std::string & name = "", const bool fail = true);
  ObsVector(const ObsVector &);
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

  std::size_t size() const {return values_.size();}  // Size of vector in local memory
  const double & operator[](const std::size_t ii) const {return values_.at(ii);}
  double & operator[](const std::size_t ii) {return values_.at(ii);}
  unsigned int nobs() const;  // Number of active observations (missing values not included)

  const double & toFortran() const;
  double & toFortran();

  const std::string & obstype() const {return obsdb_.obsname();}
  const oops::Variables & varnames() const {return obsvars_;}
  std::size_t nvars() const {return nvars_;}
  std::size_t nlocs() const {return nlocs_;}
  void mask(const ObsDataVector<int> &);

// I/O
  void save(const std::string &) const;

 private:
  void print(std::ostream &) const;
  void read(const std::string &, const bool fail = true);

  /*! \brief Associate ObsSpace object */
  ObsSpace & obsdb_;

  /*! \brief Variables */
  oops::Variables obsvars_;

  /*! \brief Number of variables */
  std::size_t nvars_;
  std::size_t nlocs_;

  /*! \brief Vector data */
  std::vector<double> values_;

  /*! \brief Missing data mark */
  const double missing_;
};
// -----------------------------------------------------------------------------

}  // namespace ioda

#endif  // IODA_OBSVECTOR_H_
