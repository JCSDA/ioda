/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef DATAFRAME_H
#define DATAFRAME_H

#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>
#include <numeric>
#include <vector>

#include "ColumnMetadatum.h"
#include "Constants.h"
#include "DataRow.h"
#include "ObsDataFrame.h"

#include "oops/util/Logger.h"

class ObsDataFrameRows : public ObsDataFrame {
 public:
  /// \brief constructor for an empy container
  ObsDataFrameRows();

  /// \brief generic container constructor
  /// \details This constructor can be used for making a copy, but also
  /// can handle a container from scratch, or a slice from another
  /// container.
  /// \param column meta data row
  /// \param list of data rows to insert into the container
  explicit ObsDataFrameRows(ColumnMetadata, std::vector<DataRow>);

  ObsDataFrameRows(ObsDataFrameRows&&)                  = delete;   //!< Deleted move constructor
  ObsDataFrameRows(const ObsDataFrameRows&)             = delete;   //!< Deleted copy constructor
  ObsDataFrameRows& operator=(ObsDataFrameRows&&)       = delete;   //!< Deleted move assignment
  ObsDataFrameRows& operator=(const ObsDataFrameRows&)  = delete;   //!< Deleted copy assignment

  /// From base class
  /// \brief set the column meta data from a list
  /// \param list (vector) of column meta datum values
  void configColumns(std::vector<ColumnMetadatum>) override;
  /// \brief set the column meta data from an initializer list
  /// \param initializer list (vector) of column meta datum values
  void configColumns(std::initializer_list<ColumnMetadatum>) override;

  /// \brief add a new column to the container
  /// \details The column values parameter of these functions needs to be matched
  /// in length with the number of rows in the container. The data type for the
  /// column is determined from the data type of the column values parameter.
  /// \param name for the new column
  /// \param vector of column values
  void appendNewColumn(const std::string&, const std::vector<std::int8_t>&) override;
  void appendNewColumn(const std::string&, const std::vector<std::int16_t>&) override;
  void appendNewColumn(const std::string&, const std::vector<std::int32_t>&) override;
  void appendNewColumn(const std::string&, const std::vector<std::int64_t>&) override;
  void appendNewColumn(const std::string&, const std::vector<float>&) override;
  void appendNewColumn(const std::string&, const std::vector<double>&) override;
  void appendNewColumn(const std::string&, const std::vector<std::string>&) override;

  /// \brief add a new row to the container
  /// \param list of values for the new row
  void appendNewRow(DataRow&) override;

  /// \brief read from an existing column
  /// \note The data type of the output vector must match the data type of the column
  /// \param column name
  /// \param output vector that will contain the column values
  void getColumn(const std::string&, std::vector<std::int8_t>&) const override;
  void getColumn(const std::string&, std::vector<std::int16_t>&) const override;
  void getColumn(const std::string&, std::vector<std::int32_t>&) const override;
  void getColumn(const std::string&, std::vector<std::int64_t>&) const override;
  void getColumn(const std::string&, std::vector<float>&) const override;
  void getColumn(const std::string&, std::vector<double>&) const override;
  void getColumn(const std::string&, std::vector<std::string>&) const override;

  /// \brief write into an existing column
  /// \note The data type of the input vector must match the data type of the column
  /// \param column name
  /// \param input vector containing the new column values
  void setColumn(const std::string&, std::vector<std::int8_t>&) const override;
  void setColumn(const std::string&, std::vector<std::int16_t>&) const override;
  void setColumn(const std::string&, std::vector<std::int32_t>&) const override;
  void setColumn(const std::string&, std::vector<std::int64_t>&) const override;
  void setColumn(const std::string&, std::vector<float>&) const override;
  void setColumn(const std::string&, std::vector<double>&) const override;
  void setColumn(const std::string&, std::vector<std::string>&) const override;

  /// \brief remove the column given the column name
  /// \param column name
  void removeColumn(const std::string&) override;
  /// \brief remove the row given by the row index
  /// \details The index is numbers from zero to n-1, where n is the current
  /// number of rows in the container. Eg, if the given row index is zero then
  /// the first row of the container is removed. If row index == 3 then the
  /// fourth row is removed.
  /// \param row index
  void removeRow(const std::int64_t&) override;

  /// \brief sort the rows in the container
  /// \details This function will sort given a column name and a sort order.
  /// The sort order (ascending or descending) is used to compare values
  /// in the given column to determine the sorting.
  /// \param column name
  /// \param sort order
  void sort(const std::string&, const std::int8_t) override;

  /// \brief sort the rows in the container
  /// \details This function will sort given af function that defines
  /// how to compare two rows.
  /// \param compare function
  void sort(std::function<std::int8_t(DataRow&, DataRow&)>) override;

  /// \brief slice the container given a column and selection criteria
  /// \details This function will apply the selection criteria to each value in the given
  /// column and select the rows that meet that criteria. The selection criteria is simply
  /// a threshold value with a comparison operator (<, <=, ==, >=, >). The column value for
  /// each row is compared to the threshold using to given comparison operator.
  /// \param column name
  /// \param comparison operator
  /// \param threshold value
  /// \return A new (deep copy) container is returned that holds the selected rows
  std::shared_ptr<ObsDataFrame> slice(const std::string&, const std::int8_t, std::int8_t) override;
  std::shared_ptr<ObsDataFrame> slice(const std::string&, const std::int8_t, std::int16_t) override;
  std::shared_ptr<ObsDataFrame> slice(const std::string&, const std::int8_t, std::int32_t) override;
  std::shared_ptr<ObsDataFrame> slice(const std::string&, const std::int8_t, std::int64_t) override;
  std::shared_ptr<ObsDataFrame> slice(const std::string&, const std::int8_t, float) override;
  std::shared_ptr<ObsDataFrame> slice(const std::string&, const std::int8_t, double) override;
  std::shared_ptr<ObsDataFrame> slice(const std::string&, const std::int8_t, std::string) override;

  /// \brief slice the container given a selection function
  /// \details This function uses a general function to determine how to select
  /// a row from the container.
  /// \param selection function
  /// \return A new (deep copy) container is returned that holds the selected rows
  std::shared_ptr<ObsDataFrame> slice(std::function<std::int8_t(DataRow&)>) override;

  /// \brief return the number of rows in the container
  const std::int64_t getNumRows() const override;

  /// \brief print the contents of the container in a tabular format
  void print() override;

  /// \brief return a reference (view) to the data rows in the container
  std::vector<DataRow>& getDataRows();

  /// Templated functions - These cannot be specified in base class

  /// \brief add a new row to the end of the container
  /// \details This function takes a variable number of parameters, but the nubmer
  /// of parameters must match the current number of columns in the container. The
  /// data types of the parameters also need to match up with the corresponding
  /// data types of the columns in the container.
  /// \param data values for each of the columns in the container
  template<typename... T>
  void appendNewRow(T... args) {
    const std::int32_t numParams = sizeof...(T);
    if (columnMetadata_.getNumCols() > 0) {
      if (numParams == columnMetadata_.getNumCols()) {
        DataRow newRow(dataRows_.size());
        std::int8_t typeMatch = true;
        // Iterative function call to unpack variadic template
        ((void) addColumnToRow(newRow, typeMatch, std::forward<T>(args)), ...);

        if (typeMatch == true) {
          appendNewRow(newRow);
        }
      } else {
        oops::Log::error() << "ERROR: Number of columns in new row are incompatible with this "
                              "data frame." << std::endl;
      }
    } else {
      oops::Log::error() << "ERROR: Cannot insert a new row without first setting column headings."
                         << std::endl;
    }
  }

  /// \brief sort the rows of the container according to the given comparison function
  /// \details The comparison function should return true when the first datum parameter
  /// is already in its sorted position before the second datum parameter.
  /// \param index of column being used to determine the sorting
  /// \param comparison function
  template <typename F>
  void sortRows(std::int64_t columnIndex, F&& func) {
    std::int64_t numberOfRows = dataRows_.size();
    std::vector<std::int64_t> indices(numberOfRows, 0);
    std::iota(std::begin(indices), std::end(indices), 0);  // Build initial list of indices.
    std::sort(std::begin(indices), std::end(indices),
              [&](const std::int64_t& a, const std::int64_t& b) {
      std::shared_ptr<DatumBase> datumA = dataRows_[a].getColumn(columnIndex);
      std::shared_ptr<DatumBase> datumB = dataRows_[b].getColumn(columnIndex);
      return func(datumA, datumB);
    });
    for (std::int64_t i = 0; i < numberOfRows; ++i) {
      while (indices[i] != indices[indices[i]]) {
        std::swap(dataRows_[indices[i]], dataRows_[indices[indices[i]]]);
        std::swap(indices[i], indices[indices[i]]);
      }
    }
  }

 private:
  /// \brief default comparison function for the sort function
  /// \details Using this function will result in an ascending sort order.
  /// \param first datum value
  /// \param second datum value
  std::int8_t compareDatums(std::shared_ptr<DatumBase>, std::shared_ptr<DatumBase>);

  /// Functions that serve the base class overrides

  /// \brief add a new column to the container
  /// \details The column values parameter of these functions needs to be matched
  /// in length with the number of rows in the container.
  /// \param name for the new column
  /// \param vector of column values
  /// \param column data type
  template<typename T> void appendNewColumn(const std::string&,
                                            const std::vector<T>&, const std::int8_t);

  /// \brief add datum for a new column in a row
  /// \details This is a helper function for appending a new column. For the i-th row,
  /// the i-th element of the new column vector is added to that row using this function.
  /// \param data row
  /// \param output flag that gets set to true if the add succeeded
  /// \param column datum value to added to the row
  template<typename T> void addColumnToRow(DataRow&, std::int8_t&, const T);

  /// \brief read from an existing column
  /// \details This is a helper function for the public getColumn funciton.
  /// \param column name
  /// \param output vector that will contain the column values
  /// \param column data type
  template<typename T> void getColumn(const std::string&,
                                      std::vector<T>&, const std::int8_t) const;

  /// \brief write into an existing column
  /// \details This is a helper function for the public setColumn function.
  /// \param column name
  /// \param input vector containing the new column values
  /// \param column data type
  template<typename T> void setColumn(const std::string&,
                                      const std::vector<T>&, const std::int8_t) const;

  /// Helper functions

  /// \brief get a single datum value
  /// \param datum object holding the value we want to get
  /// \param output datum value
  template<typename T> void getDatumValue(std::shared_ptr<DatumBase>, T&) const;

  /// \brief set a single datum value
  /// \param datum object holding the value we want to set
  /// \param input datum value
  template<typename T> void setDatumValue(std::shared_ptr<DatumBase>, const T&) const;

  /// \brief comparison function for the public slice function
  /// \param comparison operator
  /// \param threshold value
  /// \param datum value
  template<typename T> std::int8_t compareDatumToThreshold(std::int8_t, T, T);

  /// \brief helper function for the public appendNewColumn function
  /// \param column index
  /// \param data value
  template<typename T> std::shared_ptr<DatumBase> createDatum(const std::int32_t&, T);

  /// \brief construct a new DataRow and add it to the container
  /// \details This function only constructs the DataRow object but does not
  /// populate it.
  void createNewRow();
  /// \brief add the given number of rows to the container
  /// \details This function will add new DataRow objects to the container,
  /// but does not populate them.
  /// \param nubmer of rows to add
  void initialise(const std::int64_t&);

  /// \brief container rows
  std::vector<DataRow> dataRows_;
};

#endif  // DATAFRAME_H
