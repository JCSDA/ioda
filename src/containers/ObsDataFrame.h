/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef OBSDATAFRAME_H
#define OBSDATAFRAME_H

#include <cstdint>
#include <functional>

#include "ColumnMetadata.h"
#include "DataRow.h"

class ObsDataFrame {
 public:
  /// \brief constructor for empty container
  /// \param container type
  explicit ObsDataFrame(const std::int8_t);
  /// \brief constructor for container with a column metadata row but no data rows
  /// \brief column meta data row
  /// \param container type
  explicit ObsDataFrame(ColumnMetadata, const std::int8_t);

  ObsDataFrame()                                = delete;   //!< Deleted default constructor
  ObsDataFrame(ObsDataFrame&&)                  = delete;   //!< Deleted move constructor
  ObsDataFrame(const ObsDataFrame&)             = delete;   //!< Deleted copy constructor
  ObsDataFrame& operator=(ObsDataFrame&&)       = delete;   //!< Deleted move assignment
  ObsDataFrame& operator=(const ObsDataFrame&)  = delete;   //!< Deleted copy assignment

  /// From base class
  /// \brief set the column meta data from a list
  /// \param list (vector) of column meta datum values
  virtual void configColumns(std::vector<ColumnMetadatum>) = 0;
  /// \brief set the column meta data from an initializer list
  /// \param initializer list (vector) of column meta datum values
  virtual void configColumns(std::initializer_list<ColumnMetadatum>) = 0;

  /// \brief add a new column to the container
  /// \details The column values parameter of these functions needs to be matched
  /// in length with the number of rows in the container. The data type for the
  /// column is determined from the data type of the column values parameter.
  /// \param name for the new column
  /// \param vector of column values
  virtual void appendNewColumn(const std::string&, const std::vector<std::int8_t>&) = 0;
  virtual void appendNewColumn(const std::string&, const std::vector<std::int16_t>&) = 0;
  virtual void appendNewColumn(const std::string&, const std::vector<std::int32_t>&) = 0;
  virtual void appendNewColumn(const std::string&, const std::vector<std::int64_t>&) = 0;
  virtual void appendNewColumn(const std::string&, const std::vector<float>&) = 0;
  virtual void appendNewColumn(const std::string&, const std::vector<double>&) = 0;
  virtual void appendNewColumn(const std::string&, const std::vector<std::string>&) = 0;

  /// \brief add a new row to the container
  /// \param list of values for the new row
  virtual void appendNewRow(DataRow&) = 0;

  /// \brief read from an existing column
  /// \note The data type of the output vector must match the data type of the column
  /// \param column name
  /// \param output vector that will contain the column values
  virtual void getColumn(const std::string&, std::vector<std::int8_t>&) const = 0;
  virtual void getColumn(const std::string&, std::vector<std::int16_t>&) const = 0;
  virtual void getColumn(const std::string&, std::vector<std::int32_t>&) const = 0;
  virtual void getColumn(const std::string&, std::vector<std::int64_t>&) const = 0;
  virtual void getColumn(const std::string&, std::vector<float>&) const = 0;
  virtual void getColumn(const std::string&, std::vector<double>&) const = 0;
  virtual void getColumn(const std::string&, std::vector<std::string>&) const = 0;

  /// \brief write into an existing column
  /// \note The data type of the input vector must match the data type of the column
  /// \param column name
  /// \param input vector containing the new column values
  virtual void setColumn(const std::string&, std::vector<std::int8_t>&) const = 0;
  virtual void setColumn(const std::string&, std::vector<std::int16_t>&) const = 0;
  virtual void setColumn(const std::string&, std::vector<std::int32_t>&) const = 0;
  virtual void setColumn(const std::string&, std::vector<std::int64_t>&) const = 0;
  virtual void setColumn(const std::string&, std::vector<float>&) const = 0;
  virtual void setColumn(const std::string&, std::vector<double>&) const = 0;
  virtual void setColumn(const std::string&, std::vector<std::string>&) const = 0;

  /// \brief remove the column given the column name
  /// \param column name
  virtual void removeColumn(const std::string&) = 0;
  /// \brief remove the row given by the row index
  /// \details The index is numbers from zero to n-1, where n is the current
  /// number of rows in the container. Eg, if the given row index is zero then
  /// the first row of the container is removed. If row index == 3 then the
  /// fourth row is removed.
  /// \param row index
  virtual void removeRow(const std::int64_t&) = 0;

  /// \brief sort the rows in the container
  /// \details This function will sort given a column name and a sort order.
  /// The sort order (ascending or descending) is used to compare values
  /// in the given column to determine the sorting.
  /// \param column name
  /// \param sort order
  virtual void sort(const std::string&, const std::int8_t) = 0;
  /// \brief sort the rows in the container
  /// \details This function will sort given af function that defines
  /// how to compare two rows.
  /// \param compare function
  virtual void sort(std::function<std::int8_t(DataRow&, DataRow&)>) = 0;

  /// \brief slice the container given a column and selection criteria
  /// \details This function will apply the selection criteria to each value in the given
  /// column and select the rows that meet that criteria. The selection criteria is simply
  /// a threshold value with a comparison operator (<, <=, ==, >=, >). The column value for
  /// each row is compared to the threshold using to given comparison operator.
  /// \param column name
  /// \param comparison operator
  /// \param threshold value
  /// \return A new (deep copy) container is returned that holds the selected rows
  virtual std::shared_ptr<ObsDataFrame>
      slice(const std::string&, const std::int8_t, std::int8_t) = 0;
  virtual std::shared_ptr<ObsDataFrame>
      slice(const std::string&, const std::int8_t, std::int16_t) = 0;
  virtual std::shared_ptr<ObsDataFrame>
      slice(const std::string&, const std::int8_t, std::int32_t) = 0;
  virtual std::shared_ptr<ObsDataFrame>
      slice(const std::string&, const std::int8_t, std::int64_t) = 0;
  virtual std::shared_ptr<ObsDataFrame>
      slice(const std::string&, const std::int8_t, float) = 0;
  virtual std::shared_ptr<ObsDataFrame>
      slice(const std::string&, const std::int8_t, double) = 0;
  virtual std::shared_ptr<ObsDataFrame>
      slice(const std::string&, const std::int8_t, std::string) = 0;

  /// \brief slice the container given a selection function
  /// \details This function uses a general function to determine how to select
  /// a row from the container.
  /// \param selection function
  /// \return A new (deep copy) container is returned that holds the selected rows
  virtual std::shared_ptr<ObsDataFrame> slice(std::function<std::int8_t(DataRow&)>) = 0;

  /// \brief print out the container contents in a tabular form
  virtual void print() = 0;

  /// \brief return the number of rows in the container
  virtual const std::int64_t getNumRows() const = 0;

  /// \brief return the container type
  const std::int8_t getType();

 protected:
  /// \brief column meta data
  /// \details The column meta data consists of the column name, data type and width.
  /// The column width is only used for print formatting.
  ColumnMetadata columnMetadata_;

 private:
  /// \brief type of data frame container
  /// \details current types are row priority, column priority, row view and column view
  const std::int8_t type_;
};

#endif  // OBSDATAFRAME_H
