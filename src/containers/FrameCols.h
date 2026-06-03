/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef CONTAINERS_FRAMECOLS_H_
#define CONTAINERS_FRAMECOLS_H_

#include <cstdint>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "eckit/exception/Exceptions.h"

#include "ioda/containers/ColumnMetadata.h"
#include "ioda/containers/ColumnMetadatum.h"
#include "ioda/containers/Datum.h"
#include "ioda/containers/FrameColsData.h"
#include "ioda/containers/FunctionsCols.h"
#include "ioda/containers/IFrame.h"
#include "ioda/containers/ViewCols.h"
#include "oops/util/Logger.h"

namespace osdf {
class FrameRows;

/// \class FrameCols
/// \brief One of the four primary classes designed for explicit instantiation, and one of two with
/// a full read-write interface to the column-priority data model defined by the IFrame pure
/// abstract class. Use of this interface will allow for polymorphism to be used in code that
/// implements one of this container. If attached to a pointer to the IFrame base-class, the same
/// basic interface will be available. The overridden class functions carry out all
/// required error checking and terminal output, and use the functions classes before carrying out
/// operations on the data model. The members are classes for the read-write data model and
/// functions that are specific to operations on column-priority data. The functions class uses a
/// base class with methods applicable to different container types. Most overridden functions are
/// documented as part of the IFrame class. Other functions are documented below.

class FrameCols : public IFrame {
 public:
  /// \brief For initialising an empty container.
  FrameCols();
  /// \brief For initialising a sliced copy of existing data.
  explicit FrameCols(const ColumnMetadata&, const std::vector<std::int64_t>&,
                     const std::vector<std::shared_ptr<DataBase>>&);
  /// \brief For initialising a column-priority container from a row-priority container.
  explicit FrameCols(const FrameRows&);

  virtual ~FrameCols();

  FrameCols(FrameCols&&)                 = delete;
  FrameCols(const FrameCols&)            = delete;
  FrameCols& operator=(FrameCols&&)      = delete;
  FrameCols& operator=(const FrameCols&) = delete;

  void configColumns(const std::vector<ColumnMetadatum>) override;
  void configColumns(const std::initializer_list<ColumnMetadatum>) override;

  void appendNewColumn(const std::string&, const std::vector<int>&,
                       const std::string& = util::missingValue<std::string>()) override;
  void appendNewColumn(const std::string&, const std::vector<std::int64_t>&,
                       const std::string& = util::missingValue<std::string>()) override;
  void appendNewColumn(const std::string&, const std::vector<float>&,
                       const std::string& = util::missingValue<std::string>()) override;
  void appendNewColumn(const std::string&, const std::vector<char>&,
                       const std::string& = util::missingValue<std::string>()) override;
  void appendNewColumn(const std::string&, const std::vector<std::string>&,
                       const std::string& = util::missingValue<std::string>()) override;

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

  std::int8_t getColumnType(const std::string&) const override;
  std::string getColumnUnits(const std::string&) const override;

  void removeRow(const std::int64_t) override;
  void removeRows(const std::vector<bool> &) override;

  void sortRows(const std::string&, const std::int8_t) override;

  std::size_t numRows() const override { return data_.getSizeRows(); }
  std::size_t numCols() const override { return data_.getSizeCols(); }
  std::vector<std::string> columnNames() const override;

  std::size_t getColumnMetadataBufferSize() const override;
  std::size_t serializeColumnMetadata(eckit::Buffer &) const override;
  void deserializeColumnMetadata(eckit::Buffer & columnMetadataBuffer) override;

  void append(const std::unique_ptr<IFrame>& srcOsdf,
              bool addOffsetToSourceLocationIndices = true) override;

  void print() const override;
  std::string frameType() const override;
  void clear();

  /// \brief The following functions return an instance of the derived type for chaining of function
  /// calls. This is the reason these functions are not part of the IFrame interface.
  /// \param Target column name.
  /// \param One of five enum values.
  /// \param The value to compare against.
  FrameCols sliceRows(const std::string&, const std::int8_t, const int) const;
  FrameCols sliceRows(const std::string&, const std::int8_t, const std::int64_t) const;
  FrameCols sliceRows(const std::string&, const std::int8_t, const float) const;
  FrameCols sliceRows(const std::string&, const std::int8_t, const std::string) const;

  std::unique_ptr<IFrame> sliceFrame(const std::string&, const std::int8_t,
                                     const int) const override;
  std::unique_ptr<IFrame> sliceFrame(const std::string&, const std::int8_t,
                                     const std::int64_t) const override;
  std::unique_ptr<IFrame> sliceFrame(const std::string&, const std::int8_t,
                                     const float) const override;
  std::unique_ptr<IFrame> sliceFrame(const std::string&, const std::int8_t,
                                     const std::string) const override;

  /// \brief Returns an instance of a class with a read-only view of the containing data.
  ViewCols makeView();

  void attach(ViewCols*);
  void detach(ViewCols*);

  /// \brief Returns a reference to the data model.
  const FrameColsData& getData() const override;

  /// \brief Variadic function accepts zero or more parameters of any type. Input parameters are
  /// checked for errors, and added to the data model once a complete and compatible data row has
  /// been constructed.
  /// \param Defined by user input - should match existing frame configuration.
  template<typename... T>
  void appendNewRow(T... args) {
    const std::int32_t numParams = sizeof...(T);
    if (data_.getSizeCols() <= 0) {
      const std::string errMsg = std::string(
        "ERROR: Cannot insert a new row without first setting"
        " column headings.");
      throw eckit::BadValue(errMsg, Here());
    }
    if (numParams != data_.getSizeCols()) {
      const std::string errMsg = std::string(
        "ERROR: Number of columns in new row are incompatible with "
        "this data frame.");
      throw eckit::BadParameter(errMsg, Here());
    }

    for (std::int32_t columnIndex = 0; columnIndex < numParams; ++columnIndex) {
    const std::int8_t permission = data_.getPermission(columnIndex);
      if (permission != consts::eReadWrite) {
        const std::string errMsg = std::string("ERROR: Column named ") + data_.getName(columnIndex)
                                   + std::string(" is set to read-only.");
        throw eckit::BadParameter(errMsg, Here());
      }
    }

    DataRow newRow(data_.getMaxId() + 1);
    std::int8_t typeMatch = true;
    std::int32_t columnIndex = 0;
    // Iterative function call to unpack variadic template
    ((void) funcs_.addColumnToRow(&data_, newRow, typeMatch,
                                        columnIndex, std::forward<T>(args)), ...);
    if (typeMatch != true) {
      const std::string errMsg = "ERROR: Data type for column " + data_.getName(columnIndex)
                                 + " is incompatible with current data frame.";
      throw eckit::BadParameter(errMsg, Here());
    }
    data_.appendNewRow(newRow);
    notify();
  }

 private:
  void notify();

  /// \brief Templated function called from the overridden interface functions.
  template <typename T>
  void appendNewColumn(const std::string&, const std::vector<T>&, const std::int8_t,
                       const std::string& = util::missingValue<std::string>());
  /// \brief Templated function called from the overridden interface functions.
  template<typename T> void getColumn(const std::string&, std::vector<T>&, const std::int8_t) const;
  /// \brief Templated function called from the overridden interface functions.
  template<typename T> void setColumn(const std::string&, const std::vector<T>&,
                                      const std::int8_t) const;
  /// \brief Templated function called from the local functions to slice the data container.
  template<typename T> FrameCols sliceRows(const std::string&, const std::int8_t, const T) const;
  /// \brief Extracts filtered column metadata, ids, and data columns without
  /// constructing a FrameCols.
  template<typename T> std::tuple<ColumnMetadata, std::vector<std::int64_t>,
      std::vector<std::shared_ptr<DataBase>>>
      sliceRowsImpl(const std::string&, const std::int8_t, const T) const;

  FunctionsCols funcs_;  /// \brief Functions for column-priority containers.
  FrameColsData data_;  /// \brief The data model.

  std::vector<ViewCols*> views_;
};
}  // namespace osdf

#endif  // CONTAINERS_FRAMECOLS_H_
