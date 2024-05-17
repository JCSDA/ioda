/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef DATAUMBASE_H
#define DATAUMBASE_H

#include <cstdint>
#include <string>

class DatumBase {
 public:
  /// \brief constructor with data type
  /// \param data type
  explicit DatumBase(const std::int8_t type);
  virtual ~DatumBase() = default;

  DatumBase()                             = delete;  //!< Deleted default constructor
  DatumBase(DatumBase&&)                  = delete;  //!< Deleted move constructor
  DatumBase(const DatumBase&)             = delete;  //!< Deleted copy constructor
  DatumBase& operator=(DatumBase&&)       = delete;  //!< Deleted move assignment
  DatumBase& operator=(const DatumBase&)  = delete;  //!< Deleted copy assignment

  /// \brief return string representation for the datum value (used for printing)
  virtual const std::string getDatumStr() const = 0;

  /// \brief return data type
  const std::int8_t getType() const;

 protected:
  /// \brief data type
  std::int8_t type_;
};

#endif  // DATUMBASE_H
