/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef CONTAINERS_FRAMEROWS_H_
#define CONTAINERS_FRAMEROWS_H_

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

class FrameRows : public IFrame {
 public:
  FrameRows();
  explicit FrameRows(const ColumnMetadata, const std::vector<DataRow>);
  explicit FrameRows(const FrameCols&);

  FrameRows(FrameRows&&)                 = delete;
  FrameRows(const FrameRows&)            = delete;
  FrameRows& operator=(FrameRows&&)      = delete;
  FrameRows& operator=(const FrameRows&) = delete;

  void configColumns(const std::vector<ColumnMetadatum>) override;
  void configColumns(const std::initializer_list<ColumnMetadatum>) override;

  void appendNewColumn(const std::string&, const std::vector<std::int8_t>&) override;
  void appendNewColumn(const std::string&, const std::vector<std::int16_t>&) override;
  void appendNewColumn(const std::string&, const std::vector<std::int32_t>&) override;
  void appendNewColumn(const std::string&, const std::vector<std::int64_t>&) override;
  void appendNewColumn(const std::string&, const std::vector<float>&) override;
  void appendNewColumn(const std::string&, const std::vector<double>&) override;
  void appendNewColumn(const std::string&, const std::vector<std::string>&) override;

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
  void removeColumn(const std::int32_t) override;

  void removeRow(const std::int64_t) override;  // Currently removes based on row index, not ID.

  void sortRows(const std::string&, const std::int8_t) override;

  void print() const override;
  void clear() override;

  FrameRows sliceRows(const std::string&, const std::int8_t, const std::int8_t) const;
  FrameRows sliceRows(const std::string&, const std::int8_t, const std::int16_t) const;
  FrameRows sliceRows(const std::string&, const std::int8_t, const std::int32_t) const;
  FrameRows sliceRows(const std::string&, const std::int8_t, const std::int64_t) const;
  FrameRows sliceRows(const std::string&, const std::int8_t, const float) const;
  FrameRows sliceRows(const std::string&, const std::int8_t, const double) const;
  FrameRows sliceRows(const std::string&, const std::int8_t, const std::string) const;

  void sortRows(const std::string&, const std::function<std::int8_t(
                const std::shared_ptr<DatumBase>, const std::shared_ptr<DatumBase>)>);

  FrameRows sliceRows(const std::function<const std::int8_t(const DataRow&)>);

  ViewRows makeView() const;

  const FrameRowsData& getData() const;

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
  template<typename T> void appendNewColumn(const std::string&, const std::vector<T>&,
                                            const std::int8_t);
  template<typename T> void getColumn(const std::string&, std::vector<T>&, const std::int8_t) const;
  template<typename T> void setColumn(const std::string&, const std::vector<T>&,
                                      const std::int8_t) const;

  template<typename T> FrameRows sliceRows(const std::string&, const std::int8_t, const T) const;

  FunctionsRows funcs_;
  FrameRowsData data_;
};
}  // namespace osdf

#endif  // CONTAINERS_FRAMEROWS_H_
