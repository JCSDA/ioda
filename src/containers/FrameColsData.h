/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef CONTAINERS_FRAMECOLSDATA_H_
#define CONTAINERS_FRAMECOLSDATA_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "ioda/containers/ColumnMetadata.h"
#include "ioda/containers/Constants.h"
#include "ioda/containers/DataBase.h"
#include "ioda/containers/DataRow.h"
#include "ioda/containers/Functions.h"
#include "ioda/containers/FunctionsCols.h"
#include "ioda/containers/IColsData.h"
#include "ioda/containers/IFrameData.h"

namespace osdf {

/// \class FrameColsData
/// This class stores the full read-write data model for column-priority data containers. This class
/// stores the data and performs operations on it, but it does not perform any error-checking or
/// user output itself. It is currently assumed that all relevant checks are made before a call to
/// this class is made. This is true of the current design where though capable, this class is not
/// meant for instantiation by user, but is instead instantiated only by the FrameRows class. This
/// class uses a multiple-inheritance strategy to allow derived classes to be passed into relevant
/// functions to reduce code repetition and enhance maintainability. Some method comments are
/// avoided where the documentation in the parent classes is considered complete or where their
/// purpose is considered self-explanatory.
/// \see FrameCols
/// \see Functions::addColumnToRow
/// \see FunctionsCols::sliceRows

class FrameColsData : public IFrameData, public IColsData {
 public:
  explicit FrameColsData(const FunctionsCols&, const ColumnMetadata&,
           const std::vector<std::int64_t>&, const std::vector<std::shared_ptr<DataBase>>&);
  explicit FrameColsData(const FunctionsCols&);

  FrameColsData()                                = delete;
  FrameColsData(FrameColsData&&)                 = delete;
  FrameColsData(const FrameColsData&)            = delete;
  FrameColsData& operator=(FrameColsData&&)      = delete;
  FrameColsData& operator=(const FrameColsData&) = delete;

  /// \brief The following two functions are implemented as part of the IFrame interface and are
  /// used to configure the column metadata for the data container.
  /// \param The column metadata, either as a vector or as an initializer list.
  /// \see IFrame::configColumns
  void configColumns(const std::vector<ColumnMetadatum>);
  void configColumns(const std::initializer_list<ColumnMetadatum>);

  /// \brief Adds a complete and compatible row of data to the container.
  /// \param The row of data.
  void appendNewRow(const DataRow&);

  /// \brief Adds a new column with accompanying data to the data frame. The column is assumed to
  /// read-write uncless specified with the fourth, optional parameter.
  /// \param A pointer to the column data.
  /// \param The column name.
  /// \param An enum value representing the column data type, \see Constants::eDataTypes.
  /// \param An optional parameter specifying the column read-write capability.
  void appendNewColumn(const std::shared_ptr<DataBase>&, const std::string&, const std::int8_t,
                       const std::int8_t = consts::eReadWrite);

  /// \brief Removes a column from the data frame.
  /// \param The index of the column.
  void removeColumn(const std::int32_t);

  /// \brief Removes a row from the data frame.
  /// \param The index (not ID) of the row.
  void removeRow(const std::int64_t);

  /// \brief Updating of the highest numerical ID given to each row. The column-priority container
  /// has to maintain this outside of each data row.
  /// \param The new, highest ID.
  void updateMaxId(const std::int64_t);

  /// \brief Used to adapt the outputting of whitespace for column alignment when printing.
  /// \param Column index.
  /// \param Updated width value.
  void updateColumnWidth(const std::int32_t, const std::int16_t);

  const std::int32_t getSizeCols() const override;
  const std::int64_t getSizeRows() const override;
  const std::int64_t getMaxId() const override;
  void validateColumnMetadata(const osdf::ColumnMetadata& srcColumnMetadata) const;
  void validateColumnMetadataPermissions(
    const osdf::ColumnMetadata& srcColumnMetadata) const;
  void validateCanWriteAllData() const;

  /// \brief Searches for and returns the index of a column using its name.
  /// Throws an exception if a column of that name is not found.
  /// \param Column name to search.
  /// \return The column index.
  const std::int32_t getIndex(const std::string&) const override;

  const std::string& getName(const std::int32_t) const override;
  const std::string& getUnits(const std::int32_t) const override;
  const std::int8_t getType(const std::int32_t) const override;
  const std::int8_t getPermission(const std::int32_t) const override;

  /// \brief Checks to see if a column with a specific name exists in the data frame.
  /// \param Column name to search.
  /// \return An 8-bit integer uses as a boolean.
  const std::int8_t columnExists(const std::string&) const;

  std::vector<std::int64_t>& getIds();
  const std::vector<std::int64_t>& getIds() const override;

  std::shared_ptr<DataBase>& getDataColumn(const std::int32_t);
  const std::shared_ptr<DataBase>& getDataColumn(const std::int32_t) const override;

  ColumnMetadata& getColumnMetadata();
  const ColumnMetadata& getColumnMetadata() const override;

  std::vector<std::shared_ptr<DataBase>>& getDataCols();
  const std::vector<std::shared_ptr<DataBase>>& getDataCols() const override;

  DataRow getDataRow(const std::int64_t index) const;

  void getDataRows(std::vector<DataRow>& dataRowsContainer) const override;

  /// \brief Initialises the row IDs with no columns of data
  /// \param The number of IDs to create.
  void initialise(const std::int64_t);

  void print() const;
  void clear();

 private:
  const FunctionsCols& funcs_;  /// \brief A reference to the column-specific functions.

  ColumnMetadata columnMetadata_;  /// \brief The column metadata.
  std::vector<std::int64_t> ids_;  /// \brief The independent row IDs object.
  std::vector<std::shared_ptr<DataBase>> dataColumns_;  /// \brief The data columns.
};
}  // namespace osdf

#endif  // CONTAINERS_FRAMECOLSDATA_H_
