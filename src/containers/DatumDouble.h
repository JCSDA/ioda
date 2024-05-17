/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef DATUMDOUBLE_H
#define DATUMDOUBLE_H

#include <string>

#include "DatumBase.h"

class DatumDouble : public DatumBase {
 public:
  /// \brief constructor with datum value
  explicit DatumDouble(const double&);

  DatumDouble()                              = delete;  //!< Deleted default constr
  DatumDouble(DatumDouble&&)                 = delete;  //!< Deleted move construct
  DatumDouble(const DatumDouble&)            = delete;  //!< Deleted copy construct
  DatumDouble& operator=(DatumDouble&&)      = delete;  //!< Deleted move assign
  DatumDouble& operator=(const DatumDouble&) = delete;  //!< Deleted copy assign

  /// \brief return string representation for the datum value (used for printing)
  const std::string getDatumStr() const;

  /// \brief return datum value
  const double getDatum() const;
  double getDatum();

  /// \brief set datum value
  /// \param value
  void setDatum(double);

 private:
  /// \brief datum value
  double datum_;
};

#endif  // DATUMDOUBLE_H
