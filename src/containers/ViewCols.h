/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef CONTAINERS_VIEWCOLS_H_
#define CONTAINERS_VIEWCOLS_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "ioda/containers/ColumnMetadata.h"
#include "ioda/containers/ColumnMetadatum.h"
#include "ioda/containers/Functions.h"
#include "ioda/containers/FunctionsCols.h"
#include "ioda/containers/IView.h"
#include "ioda/containers/ViewColsData.h"

namespace osdf {
class FrameCols;

/// \class ViewCols
/// \brief One of the four primary classes designed for explicit instantiation, and one of two with
/// a read-only interface to the column-priority data model defined by the IView pure
/// abstract class. The set of available functions are defined by those operations that cannot
/// create new data or modify the underlying data on which its based. The overridden class functions
/// lean on the functions classes to carry out all required error checking and terminal output
/// before carrying out operations on the data model. The members are classes for the read-only data
/// model and functions that are specific to operations on column-priority data. The functions class
/// uses a base class with methods applicable to different container types. Most overridden
/// functions are documented as part of the IFrame class. Other functions are documented below.

class ViewCols : public IView {
 public:
  /// \brief Can only be constructed be a view of existing data.
  explicit ViewCols(const ColumnMetadata&, const std::vector<std::int64_t>&,
                    const std::vector<std::shared_ptr<DataBase>>&, FrameCols*);

  ~ViewCols();

  ViewCols()                           = delete;
  ViewCols(ViewCols&&)                 = delete;
  ViewCols(const ViewCols&)            = delete;
  ViewCols& operator=(ViewCols&&)      = delete;
  ViewCols& operator=(const ViewCols&) = delete;

  void getColumn(const std::string&, std::vector<int>&) const override;
  void getColumn(const std::string&, std::vector<std::int64_t>&) const override;
  void getColumn(const std::string&, std::vector<float>&) const override;
  void getColumn(const std::string&, std::vector<std::string>&) const override;

  void print() override;

  /// \brief Returns an instance of the derived type for chaining of function calls.
  /// \param name Target column name.
  /// \param comparison One of five enum values.
  /// \param threshold The value to compare against.
  ViewCols sliceRows(const std::string&, const std::int8_t, const int) const;
  ViewCols sliceRows(const std::string&, const std::int8_t, const std::int64_t) const;
  ViewCols sliceRows(const std::string&, const std::int8_t, const float) const;
  ViewCols sliceRows(const std::string&, const std::int8_t, const std::string) const;

  void setUpdatedObjects(const ColumnMetadata&, const std::vector<std::int64_t>&,
                         const std::vector<std::shared_ptr<DataBase>>&);

 private:
  /// \brief Templated function called from the overridden interface functions.
  template<typename T> void getColumn(const std::string&, std::vector<T>&, const std::int8_t) const;
  /// \brief Templated function called from the local functions to slice the data container.
  template<typename T> ViewCols sliceRows(const std::string&, const std::int8_t, const T) const;

  void clear();

  FunctionsCols funcs_;  /// \brief Functions for column-priority containers.
  ViewColsData data_;  /// \brief The read-only data model.

  FrameCols* parent_;
};
}  // namespace osdf

#endif  // CONTAINERS_VIEWCOLS_H_
