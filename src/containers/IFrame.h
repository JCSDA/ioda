/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef CONTAINERS_IFRAME_H_
#define CONTAINERS_IFRAME_H_

#include <cstdint>
#include <string>
#include <vector>

namespace osdf {
class ColumnMetadatum;
class DataBase;
class DataRow;
class DatumBase;

/// \class IFrame
/// \brief The pure-abstract base class for the full read-write data container. This class
/// represents an interface and sets the contract to which which derived data containers must
/// adhere. There are are currently no member variables, and no function definitions. However, this
/// class does have an explicit constructor as a place to initialise future potential member
/// variables. This could be useful if future extenstions to derived classes leads to common
/// functionality being moved here. At present this class allows derived types to be handled using
/// a pointer to this class, and for the interface to this class to be available.

class IFrame {
 public:
  /// \brief The default, currently empty constructor.
  IFrame() {}
  virtual ~IFrame() {}

  IFrame(IFrame&&)                 = delete;
  IFrame(const IFrame&)            = delete;
  IFrame& operator=(IFrame&&)      = delete;
  IFrame& operator=(const IFrame&) = delete;

  /// \brief The following two functions are used to initialise data columns in the absence of
  /// corresponding column data. This can be useful if a container needs to be set up to receive
  /// unknown data from an open source.
  /// \param The column metadata, either as a vector or as an initializer list.
  virtual void configColumns(const std::vector<ColumnMetadatum>) = 0;
  virtual void configColumns(const std::initializer_list<ColumnMetadatum>) = 0;

  /// \brief The following functions are used to add a new column of data to the container.
  /// \param The target column name.
  /// \param A reference to a vector containing the data to populate the new column.
  virtual void appendNewColumn(const std::string&, const std::vector<int>&) = 0;
  virtual void appendNewColumn(const std::string&, const std::vector<std::int64_t>&) = 0;
  virtual void appendNewColumn(const std::string&, const std::vector<float>&) = 0;
  virtual void appendNewColumn(const std::string&, const std::vector<char>&) = 0;
  virtual void appendNewColumn(const std::string&, const std::vector<std::string>&) = 0;

  /// \brief The following functions are used to copy data from a column. The derived classes use
  /// templated functions to achieve this functionality and so this function is required to take the
  /// output location as an input in order for the signature of the templated function to change.
  /// Otherwise it would return a vector of copied data.
  /// \param The target column name.
  /// \param A reference to a vector that will be used to store the copied data.
  virtual void getColumn(const std::string&, std::vector<int>&) const = 0;
  virtual void getColumn(const std::string&, std::vector<std::int64_t>&) const = 0;
  virtual void getColumn(const std::string&, std::vector<float>&) const = 0;
  virtual void getColumn(const std::string&, std::vector<char>&) const = 0;
  virtual void getColumn(const std::string&, std::vector<std::string>&) const = 0;

  /// \brief The following functions are used to replace the data on an existing column.
  /// \param The target column name.
  /// \param A reference to a vector containing the data to overwrite the new column.
  virtual void setColumn(const std::string&, const std::vector<int>&) const = 0;
  virtual void setColumn(const std::string&, const std::vector<std::int64_t>&) const = 0;
  virtual void setColumn(const std::string&, const std::vector<float>&) const = 0;
  virtual void setColumn(const std::string&, const std::vector<char>&) const = 0;
  virtual void setColumn(const std::string&, const std::vector<std::string>&) const = 0;

  /// \brief Returns flag indicating the presence of a column with a specified name.
  /// \param The column name.
  virtual bool hasColumn(const std::string&) const = 0;

  /// \brief Returns type for specified column. (See osdf::consts::eDataTypes for type enum.)
  /// \param The column name.
  virtual std::int8_t getColumnType(const std::string&) const = 0;

  /// \brief As the name suggests.
  /// \param The target column name.
  virtual void removeColumn(const std::string&) = 0;

  /// \brief As the name suggests.
  /// \param The column index.
  virtual void removeColumn(const std::int32_t) = 0;

  /// \brief As the name suggests. Note the comment below for a possible area for improvement.
  /// \param The row index - not the ID which is output alongside the row on a call to print.
  virtual void removeRow(const std::int64_t) = 0;

  /// \brief Remove a set of rows according to an input vector<bool>
  /// \details This function takes a vector<bool> which has a length equal to the
  /// number of rows in the frame. The vector denotes the rows to keep (true entries)
  /// and the the rows to remove (false entries).
  /// \param keepRows vector<bool> where true elements indicate rows to keep, and
  ///                 false elements indicate rows to remove
  virtual void removeRows(const std::vector<bool> &) = 0;

  /// \brief Sorts the order of the rows based on how the values in a target column compare.
  /// \param The target column name.
  /// \param The comparator operator, specified in osdf::Constants::eComparisons.
  virtual void sortRows(const std::string&, const std::int8_t) = 0;

  /// \brief Returns the number of rows in the container.
  virtual std::size_t numRows() const = 0;

  /// \brief Returns the number of columns in the container.
  virtual std::size_t numCols() const = 0;

  /// \brief Returns vector of strings containing the column names.
  virtual std::vector<std::string> columnNames() const = 0;

  /// \brief Outputs the contents to screen. Used primarily for debugging and development.
  virtual void print() const = 0;
};

}  // namespace osdf

#endif  // CONTAINERS_IFRAME_H_
