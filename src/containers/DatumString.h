/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef DATUMSTRING_H
#define DATUMSTRING_H

#include <string>

#include "DatumBase.h"

class DatumString : public DatumBase {
 public:
  /// \brief constructor with datum value
  explicit DatumString(const std::string&);

  DatumString()                              = delete;  //!< Deleted default constr
  DatumString(DatumString&&)                 = delete;  //!< Deleted move construct
  DatumString(const DatumString&)            = delete;  //!< Deleted copy construct
  DatumString& operator=(DatumString&&)      = delete;  //!< Deleted move assign
  DatumString& operator=(const DatumString&) = delete;  //!< Deleted copy assign

  /// \brief return string representation for the datum value (used for printing)
  const std::string getDatumStr() const;

  /// \brief return datum value
  const std::string getDatum() const;
  std::string getDatum();

  /// \brief set datum value
  /// \param value
  void setDatum(std::string);

 private:
  /// \brief datum value
  std::string datum_;
};

#endif  // DATUMSTRING_H
