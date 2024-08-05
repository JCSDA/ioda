/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef OBSDATAVIEWROWS_H
#define OBSDATAVIEWROWS_H

#include <algorithm>
#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <numeric>
#include <string>
#include <vector>

#include "ioda/containers/DataRow.h"
#include "ioda/containers/DatumBase.h"
#include "ioda/containers/ObsDataFrame.h"

namespace osdf {
class ObsDataViewRows : public ObsDataFrame {
 public:
  ObsDataViewRows();
  explicit ObsDataViewRows(ColumnMetadata, std::vector<std::shared_ptr<DataRow>>);

  ObsDataViewRows(ObsDataViewRows&&)                  = delete;   //!< Deleted move constructor
  ObsDataViewRows(const ObsDataViewRows&)             = delete;   //!< Deleted copy constructor
  ObsDataViewRows& operator=(ObsDataViewRows&&)       = delete;   //!< Deleted move assignment
  ObsDataViewRows& operator=(const ObsDataViewRows&)  = delete;   //!< Deleted copy assignment

  void appendNewColumn(const std::string&, const std::vector<std::int8_t>&) override;
  void appendNewColumn(const std::string&, const std::vector<std::int16_t>&) override;
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
  void sort(const std::string&, const std::function<std::int8_t(
              const std::shared_ptr<DatumBase>, const std::shared_ptr<DatumBase>)>);

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

  std::shared_ptr<ObsDataFrame> slice(const std::function<const std::int8_t(
                                        const std::shared_ptr<DataRow>)>);

  void clear() override;
  void print() override;

  const std::int64_t getNumRows() const override;

  /// Public member functions
  std::vector<std::shared_ptr<DataRow>>& getDataRows();

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
        std::cout << "ERROR: Number of columns in new row are "
                     "incompatible with this data frame." << std::endl;
      }
    } else {
      std::cout << "ERROR: Cannot insert a new row without "
                   "first setting column headings." << std::endl;
    }
  }

  template <typename F>
  void sortRows(const std::int64_t columnIndex, const F&& func) {
    std::int64_t numberOfRows = dataRows_.size();
    std::vector<std::int64_t> indices(numberOfRows, 0);
    std::iota(std::begin(indices), std::end(indices), 0);  // Build initial list of indices.
    std::sort(std::begin(indices), std::end(indices), [&](const std::int64_t& i,
                                                          const std::int64_t& j) {
      std::shared_ptr<DatumBase> datumA = dataRows_.at(i)->getColumn(columnIndex);
      std::shared_ptr<DatumBase> datumB = dataRows_.at(j)->getColumn(columnIndex);
      return func(datumA, datumB);
    });
    for (std::int64_t i = 0; i < numberOfRows; ++i) {
      while (indices.at(i) != indices.at(indices.at(i))) {
        std::swap(dataRows_.at(indices.at(i)), dataRows_.at(indices.at(indices.at(i))));
        std::swap(indices.at(i), indices.at(indices.at(i)));
      }
    }
  }

 private:
  const std::int8_t compareDatums(const std::shared_ptr<DatumBase>,
                                  const std::shared_ptr<DatumBase>) const;

  /// Functions that serve the base class overrides
  template<typename T> void appendNewColumn(const std::string&, const std::vector<T>&,
                                            const std::int8_t);
  template<typename T> void addColumnToRow(DataRow&, std::int8_t&, const T);
  template<typename T> void getColumn(const std::string&,
                                      std::vector<T>&, const std::int8_t) const;
  template<typename T> void setColumn(const std::string&,
                                      const std::vector<T>&, const std::int8_t) const;

  template<typename T> std::shared_ptr<ObsDataFrame> slice(const std::string&, const std::int8_t&,
                                                           const T&, const std::int8_t&);

  /// Helper functions
  template<typename T> void getDatumValue(const std::shared_ptr<DatumBase>, T&) const;
  template<typename T> void setDatumValue(const std::shared_ptr<DatumBase>, const T&) const;

  template<typename T> const std::int8_t compareDatumToThreshold(
      const std::int8_t, const T, const T) const;

  void initialise(const std::int64_t&);

  std::vector<std::shared_ptr<DataRow>> dataRows_;
};
}  // namespace osdf

#endif  // OBSDATAVIEWROWS_H
