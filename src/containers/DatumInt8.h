/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef DATUMINT8_H
#define DATUMINT8_H

#include <cstdint>

#include "DatumBase.h"

class DatumInt8 : public DatumBase {
 public:
  /// \brief constructor with datum value
  explicit DatumInt8(const std::int8_t&);

  DatumInt8()                            = delete;  //!< Deleted default constr
  DatumInt8(DatumInt8&&)                 = delete;  //!< Deleted move construct
  DatumInt8(const DatumInt8&)            = delete;  //!< Deleted copy construct
  DatumInt8& operator=(DatumInt8&&)      = delete;  //!< Deleted move assign
  DatumInt8& operator=(const DatumInt8&) = delete;  //!< Deleted copy assign

  /// \brief return string representation for the datum value (used for printing)
  const std::string getDatumStr() const;

  /// \brief return datum value
  const std::int8_t getDatum() const;
  std::int8_t getDatum();

  /// \brief set datum value
  /// \param value
  void setDatum(std::int8_t);

 private:
  /// \brief datum value
  std::int8_t datum_;
};

#endif  // DATUMINT8_H
