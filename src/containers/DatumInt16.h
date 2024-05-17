/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef DATUMINT16_H
#define DATUMINT16_H

#include <cstdint>

#include "DatumBase.h"

class DatumInt16 : public DatumBase {
 public:
  /// \brief constructor with datum value
  explicit DatumInt16(const std::int16_t&);

  DatumInt16()                             = delete;  //!< Deleted default constr
  DatumInt16(DatumInt16&&)                 = delete;  //!< Deleted move construct
  DatumInt16(const DatumInt16&)            = delete;  //!< Deleted copy construct
  DatumInt16& operator=(DatumInt16&&)      = delete;  //!< Deleted move assign
  DatumInt16& operator=(const DatumInt16&) = delete;  //!< Deleted copy assign

  /// \brief return string representation for the datum value (used for printing)
  const std::string getDatumStr() const;

  /// \brief return datum value
  const std::int16_t getDatum() const;
  std::int16_t getDatum();

  /// \brief set datum value
  /// \param value
  void setDatum(std::int16_t);

 private:
  /// \brief datum value
  std::int16_t datum_;
};

#endif  // DATUMINT16_H
