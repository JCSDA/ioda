/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef CONTAINERS_FRAMEROWS_H_
#define CONTAINERS_FRAMEROWS_H_

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "oops/util/Logger.h"

#include "ioda/containers/ColumnMetadata.h"
#include "ioda/containers/ColumnMetadatum.h"
#include "ioda/containers/Datum.h"
#include "ioda/containers/FrameRowsData.h"
#include "ioda/containers/FunctionsRows.h"
#include "ioda/containers/IFrame.h"
#include "ioda/containers/ViewRows.h"

namespace osdf {
class FrameCols;

/// \class FrameRows
/// \brief One of the four primary classes designed for explicit instantiation, and one of two with
/// a full read-write interface to the row-priority data model defined by the IFrame pure
/// abstract class. Use of this interface will allow for polymorphism to be used in code that
/// implements one of this container. If attached to a pointer to the IFrame base-class, the same
/// basic interface will be available. The overridden class functions carry out all
/// required error checking and terminal output, and use the functions classes before carrying out
/// operations on the data model. The members are classes for the read-write data model and
/// functions that are specific to operations on row-priority data. The functions class uses a base
/// class with methods applicable to different container types. Most overridden functions are
/// documented as part of the IFrame class. Other functions are documented below.

class FrameRows : public IFrame {
 public:
  /// \brief For initialising an empty container.
  FrameRows();
  /// \brief For initialising a sliced copy of existing data.
  explicit FrameRows(const ColumnMetadata, const std::vector<DataRow>);
  /// \brief For initialising a row-priority container from a column-priority container.
  explicit FrameRows(const FrameCols&);

  virtual ~FrameRows();

  FrameRows(FrameRows&&)                 = delete;
  FrameRows(const FrameRows&)            = delete;
  FrameRows& operator=(FrameRows&&)      = delete;
  FrameRows& operator=(const FrameRows&) = delete;

  void configColumns(const std::vector<ColumnMetadatum>) override;
  void configColumns(const std::initializer_list<ColumnMetadatum>) override;

  void appendNewColumn(const std::string&, const std::vector<int>&) override;
  void appendNewColumn(const std::string&, const std::vector<std::int64_t>&) override;
  void appendNewColumn(const std::string&, const std::vector<float>&) override;
  void appendNewColumn(const std::string&, const std::vector<char>&) override;
  void appendNewColumn(const std::string&, const std::vector<std::string>&) override;

  void getColumn(const std::string&, std::vector<int>&) const override;
  void getColumn(const std::string&, std::vector<std::int64_t>&) const override;
  void getColumn(const std::string&, std::vector<float>&) const override;
  void getColumn(const std::string&, std::vector<char>&) const override;
  void getColumn(const std::string&, std::vector<std::string>&) const override;

  void setColumn(const std::string&, const std::vector<int>&) const override;
  void setColumn(const std::string&, const std::vector<std::int64_t>&) const override;
  void setColumn(const std::string&, const std::vector<float>&) const override;
  void setColumn(const std::string&, const std::vector<char>&) const override;
  void setColumn(const std::string&, const std::vector<std::string>&) const override;

  bool hasColumn(const std::string& name) const override { return data_.columnExists(name); }

  void removeColumn(const std::string&) override;
  void removeColumn(const std::int32_t) override;

  void removeRow(const std::int64_t) override;
  void removeRows(const std::vector<bool> &) override;

  std::int8_t getColumnType(const std::string&) const override;

  void sortRows(const std::string&, const std::int8_t) override;

  std::size_t numRows() const override { return data_.getSizeRows(); }
  std::size_t numCols() const override { return data_.getSizeCols(); }
  std::vector<std::string> columnNames() const override;

  void print() const override;
  void clear();

  /// \brief The following functions return an instance of the derived type for chaining of function
  /// calls. This is the reason these functions are not part of the IFrame interface.
  /// \param Target column name.
  /// \param One of five enum values.
  /// \param The value to compare against.
  FrameRows sliceRows(const std::string&, const std::int8_t, const int) const;
  FrameRows sliceRows(const std::string&, const std::int8_t, const std::int64_t) const;
  FrameRows sliceRows(const std::string&, const std::int8_t, const float) const;
  FrameRows sliceRows(const std::string&, const std::int8_t, const std::string) const;

  /// \brief Additional to the interface, this function accepts a custom lambda comparator function.
  void sortRows(const std::string&, const std::function<std::int8_t(
                const std::shared_ptr<DatumBase>, const std::shared_ptr<DatumBase>)>);

  /// \brief Additional to above, this function accepts a custom lambda comparator function.
  /// \param Lambda function taking a data row and returning an 8-bit signed int used as a boolean.
  FrameRows sliceRows(const std::function<const std::int8_t(const DataRow&)>);

  /// \brief Returns an instance of a class with a read-only view of the containing data.
  ViewRows makeView();

  void attach(ViewRows*);
  void detach(ViewRows*);

  /// \brief Returns a reference to the data model.
  const FrameRowsData& getData() const;

  /// \brief Variadic function accepts zero or more parameters of any type. Input parameters are
  /// checked for errors, and added to the data model once a complete and compatible data row has
  /// been constructed.
  /// \param Defined by user input - should match existing frame configuration.
  template<typename... T>
  void appendNewRow(T... args) {
    const std::int32_t numParams = sizeof...(T);
    if (data_.getSizeCols() > 0) {
      if (numParams == data_.getSizeCols()) {
        std::int8_t readWrite = true;
        for (std::int32_t columnIndex = 0; columnIndex < numParams; ++columnIndex) {
          const std::int8_t permission = data_.getPermission(columnIndex);
          if (permission != consts::eReadWrite) {
            const std::string name = data_.getName(columnIndex);
            oops::Log::error() << "ERROR: Column named \"" << name
                               << "\" is set to read-only." << std::endl;
            readWrite = false;
            break;
          }
        }
        if (readWrite == true) {
          DataRow newRow(data_.getMaxId() + 1);
          std::int8_t typeMatch = true;
          std::int32_t columnIndex = 0;
          // Iterative function call to unpack variadic template
          ((void) funcs_.addColumnToRow(&data_, newRow, typeMatch,
                                        columnIndex, std::forward<T>(args)), ...);
          if (typeMatch == true) {
            data_.appendNewRow(newRow);
            notify();
          } else {
            const std::string name = data_.getName(columnIndex);
            oops::Log::error() << "ERROR: Data type for column \"" << name
                               << "\" is incompatible with current data frame" << std::endl;
          }
        }
      } else {
        oops::Log::error() << "ERROR: Number of columns in new row are incompatible "
                              "with this data frame." << std::endl;
      }
    } else {
      oops::Log::error() << "ERROR: Cannot insert a new row without first setting "
                            "column headings." << std::endl;
    }
  }

 private:
  void notify();

  std::vector<DataRow*> getViewDataRows();

  template <typename F>
  void reorderDataRows(const std::int32_t columnIndex, const F&& func) {
    // Build list of ordered indices.
    const std::int64_t sizeRows = data_.getSizeRows();
    const std::size_t sizeRowsSz = static_cast<std::size_t>(sizeRows);
    std::vector<std::int64_t> indices(sizeRowsSz, 0);
    std::iota(std::begin(indices), std::end(indices), 0);    // Initial sequential list of indices.
    std::sort(std::begin(indices), std::end(indices), [&](const std::int64_t& i,
                                                          const std::int64_t& j) {
      std::shared_ptr<DatumBase>& datumA = data_.getDataRow(i).getColumn(columnIndex);
      std::shared_ptr<DatumBase>& datumB = data_.getDataRow(j).getColumn(columnIndex);
      return func(datumA, datumB);
    });
    // Reorder data row objects
    for (std::size_t i = 0; i < sizeRowsSz; ++i) {
      while (indices.at(i) != indices.at(static_cast<std::size_t>(indices.at(i)))) {
        const std::size_t iIdx = static_cast<std::size_t>(indices.at(i));
        std::swap(data_.getDataRow(indices.at(i)), data_.getDataRow(indices.at(iIdx)));
        std::swap(indices.at(i), indices.at(iIdx));
      }
    }
  }

  /// \brief Templated function called from the overridden interface functions.
  template<typename T> void appendNewColumn(const std::string&, const std::vector<T>&,
                                            const std::int8_t);
  /// \brief Templated function called from the overridden interface functions.
  template<typename T> void getColumn(const std::string&, std::vector<T>&, const std::int8_t) const;
  /// \brief Templated function called from the overridden interface functions.
  template<typename T> void setColumn(const std::string&, const std::vector<T>&,
                                      const std::int8_t) const;
  /// \brief Templated function called from the local functions to slice the data container.
  template<typename T> FrameRows sliceRows(const std::string&, const std::int8_t, const T) const;

  FunctionsRows funcs_;  /// \brief Functions for row-priority containers.
  FrameRowsData data_;  /// \brief The data model.

  std::vector<ViewRows*> views_;
};
}  // namespace osdf

#endif  // CONTAINERS_FRAMEROWS_H_
