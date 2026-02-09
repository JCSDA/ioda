/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef CONTAINERS_FRAMEROWSDATA_H_
#define CONTAINERS_FRAMEROWSDATA_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "ioda/containers/ColumnMetadata.h"
#include "ioda/containers/Constants.h"
#include "ioda/containers/DataBase.h"
#include "ioda/containers/DataRow.h"
#include "ioda/containers/DatumBase.h"
#include "ioda/containers/Functions.h"
#include "ioda/containers/FunctionsRows.h"
#include "ioda/containers/IFrameData.h"

namespace osdf {

/// \class FrameRowsData
/// This class stores the full read-write data model for row-priority data containers. This class
/// stores the data and performs operations on it, but it does not perform any error-checking or
/// user output itself. It is currently assumed that all relevant checks are made before a call to
/// this class is made. This is true of the current design where though capable, this class is not
/// meant for instantiation by user, but is instead instantiated only by the FrameRows class. This
/// class uses a multiple-inheritance strategy to allow derived classes to be passed into relevant
/// functions to reduce code repetition and enhance maintainability. Some method comments are
/// avoided where the documentation in the parent classes is considered complete or where their
/// purpose is considered self-explanatory.
/// \see FrameRows
/// \see Functions::addColumnToRow
/// \see FunctionsRows::sortRows

class FrameRowsData : public IFrameData  {
 public:
  /// \brief For initialising an empty data model.
  explicit FrameRowsData(const FunctionsRows&);

  /// \brief For initialising a populated data model for a sliced container.
  explicit FrameRowsData(const FunctionsRows&, const ColumnMetadata&, const std::vector<DataRow>&);

  FrameRowsData()                                = delete;
  FrameRowsData(FrameRowsData&&)                 = delete;
  FrameRowsData(const FrameRowsData&)            = delete;
  FrameRowsData& operator=(FrameRowsData&&)      = delete;
  FrameRowsData& operator=(const FrameRowsData&) = delete;

  /// \brief The following two functions are implemented as part of the IFrame interface and are
  /// used to configure the column metadata for the data container.
  /// \param The column metadata, either as a vector or as an initializer list.
  /// \see IFrame::configColumns
  void configColumns(const std::initializer_list<ColumnMetadatum>);
  void configColumns(const std::vector<ColumnMetadatum>);

  /// \brief Adds a complete and compatible row of data to the container.
  /// \param The row of data.
  void appendNewRow(const DataRow&);

  /// \brief Because this is the data model for the row
  /// -priority class, the equivalent call from the \see FrameRows::appendNewColumn breaks down each
  /// element of the accompanying vector and creates instances of \see Datum in order to modify the
  /// existing instances of \see DataRow. In effect this function _configures_ a new column to the
  /// data frame, but the name is kept for parity with \see FrameColsData::appendNewColumn. The new
  /// column is given read-write uncless specified with the fourth, optional parameter.
  /// \param The column name.
  /// \param An enum value representing the column data type, \see Constants::eDataTypes.
  /// \param An optional parameter specifying the column read-write capability.
  void appendNewColumn(const std::string&, const std::int8_t,
                       const std::int8_t = consts::eReadWrite);

  /// \brief Removes a column from the data frame.
  /// \param The index of the column.
  void removeColumn(const std::int32_t);

  /// \brief Removes a row from the data frame.
  /// \param The index (not ID) of the row.
  void removeRow(const std::int64_t);

  /// \brief Used to adapt the outputting of whitespace for column alignment when printing.
  /// \param Column index.
  /// \param Updated width value.
  void updateColumnWidth(const std::int32_t, const std::int16_t);

  const std::int32_t getSizeCols() const;
  const std::int64_t getSizeRows() const override;
  const std::int64_t getMaxId() const override;
  const bool compareColumnMetadata(const osdf::ColumnMetadata& srcColumnMetadata) const;
  const bool compareColumnMetadataPermissions(
    const osdf::ColumnMetadata& srcColumnMetadata) const;
  const bool canWriteAllData() const;

  /// \brief Searches for and returns the index of a column using its name.
  /// Throws an exception if the column is not found.
  /// \param Column name to search.
  /// \return The column index.
  const std::int32_t getIndex(const std::string&) const;

  const std::string& getName(const std::int32_t) const override;
  const std::int8_t getType(const std::int32_t) const override;
  const std::int8_t getPermission(const std::int32_t) const override;

  /// \brief Checks to see if a column with a specific name exists in the data frame.
  /// \param Column name to search.
  /// \return An 8-bit integer uses as a boolean.
  const std::int8_t columnExists(const std::string&) const;

  DataRow& getDataRow(const std::int64_t);
  const DataRow& getDataRow(const std::int64_t) const;

  ColumnMetadata& getColumnMetadata();
  const ColumnMetadata& getColumnMetadata() const override;
  const std::vector<DataRow>& getDataRows() const;
  std::vector<DataRow>& getDataRows();
  void getDataRows(std::vector<DataRow>& dataRowsContainer) const override;

  /// \brief Initialises a set of DataRow objects with IDs but no columns of data.
  /// \param The number of data rows to create.
  void initialise(const std::int64_t);

  void print() const;
  void clear();

 private:
  const FunctionsRows& funcs_;  /// \brief A reference to the row-specific functions.

  ColumnMetadata columnMetadata_;  /// \brief The column metadata.
  std::vector<DataRow> dataRows_;  /// \brief The data rows.
};
}  // namespace osdf

#endif  // CONTAINERS_FRAMEROWSDATA_H_
