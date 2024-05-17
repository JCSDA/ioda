/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef DATUMINT64_H
#define DATUMINT64_H

#include <cstdint>

#include "DatumBase.h"

class DatumInt64 : public DatumBase {
 public:
  /// \brief constructor with datum value
  explicit DatumInt64(const std::int64_t&);

  DatumInt64()                             = delete;  //!< Deleted default constr
  DatumInt64(DatumInt64&&)                 = delete;  //!< Deleted move construct
  DatumInt64(const DatumInt64&)            = delete;  //!< Deleted copy construct
  DatumInt64& operator=(DatumInt64&&)      = delete;  //!< Deleted move assign
  DatumInt64& operator=(const DatumInt64&) = delete;  //!< Deleted copy assign

  /// \brief return string representation for the datum value (used for printing)
  const std::string getDatumStr() const;

  /// \brief return datum value
  const std::int64_t getDatum() const;
  std::int64_t getDatum();

  /// \brief set datum value
  /// \param value
  void setDatum(std::int64_t);

 private:
  /// \brief datum value
  std::int64_t datum_;
};

#endif  // DATUMINT64_H
