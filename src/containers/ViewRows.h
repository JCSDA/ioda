/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef CONTAINERS_VIEWROWS_H_
#define CONTAINERS_VIEWROWS_H_

#include <algorithm>
#include <cstdint>
#include <memory>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

#include "ioda/containers/ColumnMetadata.h"
#include "ioda/containers/Constants.h"
#include "ioda/containers/Functions.h"
#include "ioda/containers/FunctionsRows.h"
#include "ioda/containers/IView.h"
#include "ioda/containers/ViewRowsData.h"

namespace osdf {
class FrameRows;

/// \class ViewRows
/// \brief One of the four primary classes designed for explicit instantiation, and one of two with
/// a read-only interface to the row-priority data model defined by the IView pure
/// abstract class. The set of available functions are defined by those operations that cannot
/// create new data or modify the underlying data on which its based. The overridden class functions
/// lean on the functions classes to carry out all required error checking and terminal output
/// before carrying out operations on the data model. The members are classes for the read-only data
/// model and functions that are specific to operations on column-priority data. The functions class
/// uses a base class with methods applicable to different container types. Most overridden
/// functions are documented as part of the IFrame class. Other functions are documented below.

class ViewRows : public IView {
 public:
  /// \brief Can only be constructed be a view of existing data.
  explicit ViewRows(const ColumnMetadata&, const std::vector<DataRow*>&, FrameRows*);

  ~ViewRows();

  ViewRows()                           = delete;
  ViewRows(ViewRows&&)                 = delete;
  ViewRows(const ViewRows&)            = delete;
  ViewRows& operator=(ViewRows&&)      = delete;
  ViewRows& operator=(const ViewRows&) = delete;

  void getColumn(const std::string&, std::vector<int>&) const override;
  void getColumn(const std::string&, std::vector<std::int64_t>&) const override;
  void getColumn(const std::string&, std::vector<float>&) const override;
  void getColumn(const std::string&, std::vector<std::string>&) const override;

  void print() override;

  /// \brief Returns an instance of the derived type for chaining of function calls.
  /// \param name Target column name.
  /// \param comparison One of five enum values.
  /// \param threshold The value to compare against.
  ViewRows sliceRows(const std::string&, const consts::eComparisons, const int) const;
  ViewRows sliceRows(const std::string&, const consts::eComparisons, const std::int64_t) const;
  ViewRows sliceRows(const std::string&, const consts::eComparisons, const float) const;
  ViewRows sliceRows(const std::string&, const consts::eComparisons, const std::string) const;

  /// \brief Additional to above, this function accepts a custom lambda comparator function.
  ViewRows sliceRows(const std::function<const std::int8_t(const DataRow*)>) const;

  /// \brief Orders rows based on target column without modifying original data.
  void sortRows(const std::string&, const consts::eSortOrders);

  /// \brief Orders rows based on target column and custom lambda comparator without modifying
  /// original data.
  void sortRows(const std::string&, const std::function<std::int8_t(
                const std::shared_ptr<DatumBase>, const std::shared_ptr<DatumBase>)>);

  void setUpdatedObjects(const ColumnMetadata&, const std::vector<DataRow*>&);

 private:
  template <typename F>
  void reorderDataRows(const std::int32_t columnIndex, const F&& func) {
    // Build list of ordered indices.
    const std::int64_t sizeRows = data_.getSizeRows();
    const std::size_t sizeRowsSz = static_cast<std::size_t>(sizeRows);
    std::vector<std::int64_t> indices(sizeRowsSz, 0);
    std::iota(std::begin(indices), std::end(indices), 0);    // Initial sequential list of indices.
    std::sort(std::begin(indices), std::end(indices), [&](const std::int64_t& i,
                                                          const std::int64_t& j) {
      std::shared_ptr<DatumBase>& datumA = data_.getDataRow(i)->getColumn(columnIndex);
      std::shared_ptr<DatumBase>& datumB = data_.getDataRow(j)->getColumn(columnIndex);
      return func(datumA, datumB);
    });
    // Reorder data row pointers
    for (std::size_t i = 0; i < sizeRowsSz; ++i) {
      while (indices.at(i) != indices.at(static_cast<std::size_t>(indices.at(i)))) {
        const std::size_t iIdx = static_cast<std::size_t>(indices.at(i));
        DataRow* a = data_.getDataRow(indices.at(i));
        DataRow* b = data_.getDataRow(indices.at(iIdx));
        data_.setDataRow(indices.at(i), b);
        data_.setDataRow(indices.at(iIdx), a);
        std::swap(indices.at(i), indices.at(iIdx));
      }
    }
  }

  /// \brief Templated function called from the overridden interface functions.
  template <typename T>
  void getColumn(const std::string&, std::vector<T>&, const consts::eDataTypes) const;
  /// \brief Templated function called from the local functions to slice the data container.
  template <typename T>
  ViewRows sliceRows(const std::string&, const consts::eComparisons, const T) const;

  void clear();

  FunctionsRows funcs_;  /// \brief Functions for row-priority containers.
  ViewRowsData data_;  /// \brief The read-only data model.

  FrameRows* parent_;
};
}  // namespace osdf

#endif  // CONTAINERS_VIEWROWS_H_
