/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef OBSDATAFRAMECOLS_H
#define OBSDATAFRAMECOLS_H

#include <cstdint>
#include <numeric>
#include <string>

#include "ioda/containers/DataBase.h"
#include "ioda/containers/ObsDataFrame.h"

#include "oops/util/Logger.h"

namespace osdf {
class ObsDataFrameCols : public ObsDataFrame {
 public:
  /// \brief constructor for an empy container
  ObsDataFrameCols();
  /// \brief generic container constructor
  /// \details This constructor can be used for making a copy, but also
  /// can handle a container from scratch, or a slice from another
  /// container.
  /// \param column meta data row
  /// \param list of data columns to insert into the container
  explicit ObsDataFrameCols(ColumnMetadata, std::vector<std::shared_ptr<DataBase>>);

  ObsDataFrameCols(ObsDataFrameCols&&)                  = delete;   //!< Deleted move constructor
  ObsDataFrameCols(const ObsDataFrameCols&)             = delete;   //!< Deleted copy constructor
  ObsDataFrameCols& operator=(ObsDataFrameCols&&)       = delete;   //!< Deleted move assignment
  ObsDataFrameCols& operator=(const ObsDataFrameCols&)  = delete;   //!< Deleted copy assignment

  /// \brief add a new column to the container
    /// \details The column values parameter of these functions needs to be matched
    /// in length with the number of rows in the container. The data type for the
    /// column is determined from the data type of the column values parameter.
    /// \param name for the new column
    /// \param vector of column values
  void appendNewColumn(const std::string&, const std::vector<std::int8_t>&)  override;
  void appendNewColumn(const std::string&, const std::vector<std::int16_t>&)  override;
  void appendNewColumn(const std::string&, const std::vector<std::int32_t>&) override;
  void appendNewColumn(const std::string&, const std::vector<std::int64_t>&) override;
  void appendNewColumn(const std::string&, const std::vector<float>&) override;
  void appendNewColumn(const std::string&, const std::vector<double>&) override;
  void appendNewColumn(const std::string&, const std::vector<std::string>&) override;

  void appendNewRow(const DataRow&) override;

  void getColumn(const std::string&, std::vector<std::int8_t>&) const override;
  void getColumn(const std::string&, std::vector<std::int16_t>&) const override;
  void getColumn(const std::string&, std::vector<std::int32_t>&) const override;
  void getColumn(const std::string&, std::vector<std::int64_t>&) const override;
  void getColumn(const std::string&, std::vector<float>&) const override;
  void getColumn(const std::string&, std::vector<double>&) const override;
  void getColumn(const std::string&, std::vector<std::string>&) const override;

  void setColumn(const std::string&, const std::vector<std::int8_t>&) const override;
  void setColumn(const std::string&, const std::vector<std::int16_t>&) const override;
  void setColumn(const std::string&, const std::vector<std::int32_t>&) const override;
  void setColumn(const std::string&, const std::vector<std::int64_t>&) const override;
  void setColumn(const std::string&, const std::vector<float>&) const override;
  void setColumn(const std::string&, const std::vector<double>&) const override;
  void setColumn(const std::string&, const std::vector<std::string>&) const override;

  void removeColumn(const std::string&) override;
  void removeRow(const std::int64_t&) override;

  void sort(const std::string&, const std::int8_t) override;

  std::shared_ptr<ObsDataFrame> slice(const std::string&, const std::int8_t&,
                                      const std::int8_t&) override;
  std::shared_ptr<ObsDataFrame> slice(const std::string&, const std::int8_t&,
                                      const std::int16_t&) override;
  std::shared_ptr<ObsDataFrame> slice(const std::string&, const std::int8_t&,
                                      const std::int32_t&) override;
  std::shared_ptr<ObsDataFrame> slice(const std::string&, const std::int8_t&,
                                      const std::int64_t&) override;
  std::shared_ptr<ObsDataFrame> slice(const std::string&, const std::int8_t&,
                                      const float&) override;
  std::shared_ptr<ObsDataFrame> slice(const std::string&, const std::int8_t&,
                                      const double&) override;
  std::shared_ptr<ObsDataFrame> slice(const std::string&, const std::int8_t&,
                                      const std::string&) override;

  void print() override;

  const std::int64_t getNumRows() const;

  /// Templated functions - Cannot be specified in base class
  template<typename... T>
  void appendNewRow(T... args) {
    const std::int32_t numParams = sizeof...(T);
    if (columnMetadata_.getNumCols() > 0) {
      if (numParams == columnMetadata_.getNumCols()) {
        DataRow newRow(columnMetadata_.getMaxId() + 1);
        std::int8_t typeMatch = true;
        // Iterative function call to unpack variadic template
        ((void) addColumnToRow(newRow, typeMatch, std::forward<T>(args)), ...);
        if (typeMatch == true) {
          appendNewRow(newRow);
        }
      } else {
        oops::Log::error() << "ERROR: Number of columns in new row are incompatible with this data "
                              "frame." << std::endl;
      }
    } else {
      oops::Log::error() << "ERROR: Cannot insert a new row without first setting column headings."
                << std::endl;
    }
  }

 private:
  /// Functions that serve the base class overrides
  template<typename T> void appendNewColumn(const std::string&, const std::vector<T>&,
                                            const std::int8_t);
  template<typename T> void addColumnToRow(DataRow&, std::int8_t&, const T);
  template<typename T> void getColumn(const std::string&, std::vector<T>&, const std::int8_t) const;
  template<typename T> void setColumn(const std::string&, const std::vector<T>&,
                                      const std::int8_t) const;

  template<typename T> std::shared_ptr<ObsDataFrameCols> slice(const std::string&,
                                                               const std::int8_t&, const T&,
                                                               const std::int8_t&);

  /// Helper functions
  template<typename T> void getDataValue(std::shared_ptr<DataBase>, std::vector<T>&) const;
  template<typename T> void setDataValue(std::shared_ptr<DataBase>, const std::vector<T>&) const;

  template<typename T> void populateIndices(std::vector<std::int64_t>&, const std::vector<T>&,
                                            const std::int8_t);
  template<typename T> void swapData(std::vector<std::int64_t>&, std::vector<T>&);

  template<typename T> const std::int8_t compareDatumToThreshold(const std::int8_t,
                                                                 const T, const T) const;

  void initialise(const std::int64_t&);

  std::vector<std::int64_t> ids_;
  std::vector<std::shared_ptr<DataBase>> dataColumns_;
};
}  // namespace osdf

#endif  // OBSDATAFRAMECOLS_H
