/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef DATUMFLOAT_H
#define DATUMFLOAT_H

#include <string>

#include "DatumBase.h"

class DatumFloat : public DatumBase {
 public:
  /// \brief constructor with datum value
  explicit DatumFloat(const float&);

  DatumFloat()                             = delete;  //!< Deleted default constr
  DatumFloat(DatumFloat&&)                 = delete;  //!< Deleted move construct
  DatumFloat(const DatumFloat&)            = delete;  //!< Deleted copy construct
  DatumFloat& operator=(DatumFloat&&)      = delete;  //!< Deleted move assign
  DatumFloat& operator=(const DatumFloat&) = delete;  //!< Deleted copy assign

  /// \brief return string representation for the datum value (used for printing)
  const std::string getDatumStr() const;

  /// \brief return datum value
  const float getDatum() const;
  float getDatum();

  /// \brief set datum value
  /// \param value
  void setDatum(float);

 private:
  /// \brief datum value
  float datum_;
};

#endif  // DATUMFLOAT_H
