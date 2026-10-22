/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef CONTAINERS_FUNCTIONS_H_
#define CONTAINERS_FUNCTIONS_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include "ioda/containers/Constants.h"
#include "ioda/containers/DataRow.h"
#include "ioda/containers/IFrameData.h"

namespace osdf {

class IFrameData;
class DataRow;
class DataBase;
class DatumBase;

class Functions {
 public:
  Functions();

  Functions(Functions&&)                 = delete;
  Functions(const Functions&)            = delete;
  Functions& operator=(Functions&&)      = delete;
  Functions& operator=(const Functions&) = delete;

  template<typename T> void addColumnToRow(IFrameData* data, DataRow&, bool&,
                                           std::int32_t&, const T) const;

  /// \brief Common checks before removing rows from a data frame.
  /// Throws if the mask size does not match the number of rows, or if rows are to be
  /// removed while any column is read-only.
  /// \param The data of the frame rows are being removed from.
  /// \param Mask with one entry per row; row i is kept iff the entry is true.
  /// \return True if the mask removes at least one row.
  bool validateRowRemoval(const IFrameData&, const std::vector<bool>&) const;

  /// \brief Common checks before appending the rows of one data frame to another.
  /// Throws if the destination has no columns, if any destination column is read-only, or if
  /// the source columns do not match the destination columns (names, units and types).
  /// \param The data of the frame being appended to.
  /// \param The data of the frame being appended.
  void validateAppend(const IFrameData&, const IFrameData&) const;

  /// \brief Appends a copy of each of the given rows to a data frame, numbering the new rows
  /// after the frame's current maximum row id.
  /// \param The data of the frame being appended to (FrameRowsData or FrameColsData).
  /// \param The rows to append. They are assumed to have passed validateAppend.
  template<typename FrameDataT>
  void appendDataRows(FrameDataT& dest, const std::vector<DataRow>& rows) const {
    for (const DataRow& row : rows) {
      DataRow newRow(dest.getMaxId() + 1);
      for (std::int32_t columnIndex = 0; columnIndex < row.getSize(); ++columnIndex) {
        newRow.insert(row.getColumn(columnIndex));
      }
      dest.appendNewRow(newRow);
    }
  }

  template<typename T> const std::shared_ptr<DataBase> createData(const std::vector<T>&) const;

  template<typename T> const std::shared_ptr<DatumBase> createDatum(const T) const;

  const bool compareDatums(const std::shared_ptr<DatumBase>&,
                                  const std::shared_ptr<DatumBase>&) const;

  template<typename T> const bool compareToThreshold(const consts::eComparisons,
                                                            const T, const T) const;

  template<typename T> const std::vector<T>& getDataValues(const std::shared_ptr<DataBase>&) const;
  template<typename T> std::vector<T>& getDataValues(std::shared_ptr<DataBase>&);

  const std::string padString(std::string, const std::int32_t) const;
};
}  // namespace osdf

#endif  // CONTAINERS_FUNCTIONS_H_
