/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef DATUMint32_t_H
#define DATUMint32_t_H

#include <cstdint>
#include <string>

#include "DatumBase.h"

class DatumInt32 : public DatumBase {
 public:
  /// \brief constructor with datum value
  explicit DatumInt32(const std::int32_t&);

  DatumInt32()                           = delete;  //!< Deleted default constr
  DatumInt32(DatumInt32&&)                 = delete;  //!< Deleted move construct
  DatumInt32(const DatumInt32&)            = delete;  //!< Deleted copy construct
  DatumInt32& operator=(DatumInt32&&)      = delete;  //!< Deleted move assign
  DatumInt32& operator=(const DatumInt32&) = delete;  //!< Deleted copy assign

  /// \brief return string representation for the datum value (used for printing)
  const std::string getDatumStr() const;

  /// \brief return datum value
  const std::int32_t getDatum() const;
  std::int32_t getDatum();

  /// \brief set datum value
  /// \param value
  void setDatum(std::int32_t);

 private:
  /// \brief datum value
  std::int32_t datum_;
};

#endif  // DATUMint32_t_H
