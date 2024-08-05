/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef DATAROW_H
#define DATAROW_H

#include <cstdint>
#include <memory>
#include <vector>

#include "ioda/containers/ColumnMetadata.h"
#include "ioda/containers/DatumBase.h"

namespace osdf {
class DataRow {
 public:
  /// \brief row constructor
  /// \details This constructor creates an empty row with the given id value.
  /// \param row id (these need to be unique)
  explicit DataRow(std::int64_t);

  DataRow() = delete;  //!< Deleted default constructor

  /// \brief return the row id
  const std::int64_t getId() const;
  /// \brief return the number of columns
  const std::int32_t getSize() const;

  /// \brief get the column datum given the column index
  /// \param column index
  /// \return column datum object
  const std::shared_ptr<DatumBase> getColumn(const std::int32_t&) const;
  std::shared_ptr<DatumBase> getColumn(const std::int32_t&);

  /// \brief append a column datum at the end of the row
  /// \param column datum object
  void insert(const std::shared_ptr<DatumBase>);
  /// \brief remove a column datum from the row
  /// \param column index
  void remove(const std::int32_t&);

  void clear();

  /// \brief print row values
  /// \details This funcion uses the column metadata row to access the column widths.
  /// \param column metadata row
  /// \param width for printing the row id
  void print(const ColumnMetadata&, const std::int32_t) const;

 private:
  /// \brief single row (set of single values for each column, disparate data types)
  std::vector<std::shared_ptr<DatumBase>> dataColumns_;

  /// \brief row id number
  std::int64_t id_;
};
}  // namespace osdf

#endif  // DATAROW_H
