/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef COLUMNMETADATA_H
#define COLUMNMETADATA_H

#include <cstdint>
#include <string>
#include <tuple>
#include <vector>

#include "ioda/containers/ColumnMetadatum.h"

namespace osdf {
class ColumnMetadata {
 public:
  ColumnMetadata();

  /// \brief test for existance of a column
  /// \param column name
  /// \return integer representation of true (non-zero) or false (zero)
  std::int8_t exists(const std::string&);

  /// \brief add a single meta data for a single column
  /// \param single column meta datum
  /// \return index in the column meta data row where the new meta dataum
  ///         was added (at the end)
  const std::int64_t add(const ColumnMetadatum);
  /// \brief add a vector of meta data for a set of columns
  /// \param column meta data for the new columns
  /// \return index in the column meta data row where the new meta data
  ///         was added (at the end)
  const std::int64_t add(const std::vector<ColumnMetadatum>);

  const std::vector<ColumnMetadatum>& get() const;

  /// \brief return a reference tothe column meta datum
  /// \param column index
  const ColumnMetadatum& get(const std::int32_t) const;

  /// \brief reset the highest ID - used when copying data frame metadata.
  void resetMaxId();
  /// \brief set the highest ID from the rows in this data frame.
  /// \param the candidate for highest row ID
  void updateMaxId(const std::int64_t&);

  /// \brief set a single column width
  /// \details This function will set the column width to the maximum of
  ///          the current width and the width parameter
  /// \param name of the target column
  /// \param new width value
  void updateColumnWidth(const std::string&, const std::int16_t&);
  /// \brief set a single column width
  /// \details This function will set the column width to the maximum of
  ///          the current width and the width parameter
  /// \param index of the target column
  /// \param new width value
  void updateColumnWidth(const std::int32_t&, const std::int16_t&);

  /// \brief remove a single column
  /// \param index of the target column
  void remove(const std::int32_t&);

  /// \brief return a single column name
  /// \param index of the target column
  const std::string& getName(const std::int32_t&) const;
  /// \brief return a single column data type
  /// \param index of the target column
  const std::int8_t getType(const std::int32_t&) const;
  /// \brief return a single column permission
  /// \param index of the target column
  const std::int8_t getPermission(const std::int32_t&) const;

  /// \brief return a single column index in the row
  /// \param name of the target column
  const std::int32_t getIndex(const std::string&) const;
  /// \brief return the number of columns in the row
  const std::int32_t getNumCols() const;
  /// \brief return the the highest row ID in this data frame
  const std::int64_t getMaxId() const;

  void clear();
  /// \brief print out the row contents for an overall tabular format
  /// \param width for the row number annotation on the left side of the table
  void print(const std::int32_t&);

 private:
  /// \brief the highest id from the rows stored in this data table
  std::int64_t maxId_;
  /// \brief special column header row
  /// \details this row contains all of the column meta data such as the
  /// columnn name, data type, etc.
  std::vector<ColumnMetadatum> columnMetadata_;
};
}  // namespace osdf

#endif  // COLUMNMETADATA_H
