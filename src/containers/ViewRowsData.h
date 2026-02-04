/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef CONTAINERS_VIEWROWSDATA_H_
#define CONTAINERS_VIEWROWSDATA_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "ioda/containers/ColumnMetadata.h"
#include "ioda/containers/DataBase.h"
#include "ioda/containers/DataRow.h"
#include "ioda/containers/Functions.h"
#include "ioda/containers/FunctionsRows.h"

namespace osdf {

/// \class ViewRowsData
/// This class stores the read-only data model for row-priority containers. This class maintains a
/// container of pointers to the rows of data in an instance of FrameRows that ViewRows::parent_
/// points to. The container in this class can be manipulated and changed, but not what the pointers
/// point to. Some method comments are avoided where the documentation in the parent classes is
/// considered complete or where their purpose is considered self-explanatory.
/// \see ViewRows

class ViewRowsData {
 public:
  /// \brief For initialising initial view of data in a FrameRows object, or a sliced view of an
  /// existing instance of ViewRows.
  explicit ViewRowsData(const Functions&, const ColumnMetadata&,
                        const std::vector<DataRow*>&);

  ViewRowsData()                               = delete;
  ViewRowsData(ViewRowsData&&)                 = delete;
  ViewRowsData(const ViewRowsData&)            = delete;
  ViewRowsData& operator=(ViewRowsData&&)      = delete;
  ViewRowsData& operator=(const ViewRowsData&) = delete;

  void setDataRow(const std::int64_t, DataRow*);

  const std::int32_t getSizeCols() const;
  const std::int64_t getSizeRows() const;
  const std::int64_t getMaxId() const;

  /// \brief Searches for and returns the index of a column using its name. Throws \see
  /// exception for an unfound column.
  /// \param Column name to search.
  /// \return The column index.
  const std::int32_t getIndex(const std::string&) const;
  const std::string& getName(const std::int32_t) const;
  const std::int8_t getType(const std::int32_t) const;

  /// \brief Checks to see if a column with a specific name exists in the data frame.
  /// \param Column name to search.
  /// \return An 8-bit integer uses as a boolean.
  const std::int8_t columnExists(const std::string&) const;

  DataRow* getDataRow(const std::int64_t);
  const DataRow* getDataRow(const std::int64_t) const;

  const ColumnMetadata& getColumnMetadata() const;
  const std::vector<DataRow*>& getDataRows() const;

  void print();
  void clear();

  void setColumnMetadata(const ColumnMetadata&);
  void setDataRows(const std::vector<DataRow*>&);

 private:
  const Functions& funcs_;  /// \brief A reference to the row-specific functions.

  ColumnMetadata columnMetadata_;  /// \brief The column metadata.
  std::vector<DataRow*> dataRows_;  /// \brief The pointers to data rows in *ViewRows::parent_.
};
}  // namespace osdf

#endif  // CONTAINERS_VIEWROWSDATA_H_
