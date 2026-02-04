/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef CONTAINERS_VIEWCOLSDATA_H_
#define CONTAINERS_VIEWCOLSDATA_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "ioda/containers/ColumnMetadata.h"
#include "ioda/containers/DataBase.h"
#include "ioda/containers/DataRow.h"
#include "ioda/containers/FunctionsCols.h"
#include "ioda/containers/IColsData.h"

namespace osdf {

/// \class ViewColsData
/// This class stores the read-only data model for column-priority containers. This class maintains
/// a container of pointers to the rows of data in an instance of FrameRows that ViewRows::parent_
/// points to. The container in this class can be manipulated and changed, but not what the pointers
/// point to. This class inherits from IColsData to allow derived classes to be passed into relevant
/// functions to reduce code repetition and enhance maintainability. Some method comments are
/// avoided where the documentation in the parent classes is considered complete or where their
/// purpose is considered self-explanatory.
/// \see ViewCols
/// \see FunctionsCols::sliceRows

class ViewColsData : public IColsData {
 public:
  /// \brief For initialising initial view of data in a FrameCols object, or a sliced view of an
  /// existing instance of ViewCols.
  explicit ViewColsData(const FunctionsCols&, const ColumnMetadata&,
           const std::vector<std::int64_t>&, const std::vector<std::shared_ptr<DataBase>>&);

  ViewColsData()                               = delete;
  ViewColsData(ViewColsData&&)                 = delete;
  ViewColsData(const ViewColsData&)            = delete;
  ViewColsData& operator=(ViewColsData&&)      = delete;
  ViewColsData& operator=(const ViewColsData&) = delete;

  const std::int32_t getSizeCols() const override;
  const std::int64_t getSizeRows() const override;
  const std::int64_t getMaxId() const;

  /// \brief Searches for and returns the index of a column using its name. Throws \see
  /// exception for an unfound column.
  /// \param Column name to search.
  /// \return The column index.
  const std::int32_t getIndex(const std::string&) const override;
  const std::string& getName(const std::int32_t) const;
  const std::int8_t getType(const std::int32_t) const;

  /// \brief Checks to see if a column with a specific name exists in the data frame.
  /// \param Column name to search.
  /// \return An 8-bit integer uses as a boolean.
  const std::int8_t columnExists(const std::string&) const;

  const std::shared_ptr<DataBase>& getDataColumn(const std::int32_t) const override;

  const ColumnMetadata& getColumnMetadata() const override;

  const std::vector<std::int64_t>& getIds() const override;

  const std::vector<std::shared_ptr<DataBase>>& getDataCols() const override;

  void print();
  void clear();

  void setColumnMetadata(const ColumnMetadata&);
  void setIds(const std::vector<std::int64_t>&);
  void setDataCols(const std::vector<std::shared_ptr<DataBase>>&);

 private:
  const FunctionsCols& funcs_;  /// \brief A reference to the row-specific functions.

  ColumnMetadata columnMetadata_;  /// \brief The column metadata.
  std::vector<std::int64_t> ids_;  /// \brief The independent row IDs object.
  std::vector<std::shared_ptr<DataBase>> dataColumns_;  /// \brief Copies of pointers to data.
};
}  // namespace osdf

#endif  // CONTAINERS_VIEWCOLSDATA_H_
