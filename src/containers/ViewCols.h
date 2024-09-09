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
class ViewCols : public IView {
 public:
  explicit ViewCols(const ColumnMetadata&, const std::vector<std::int64_t>&,
                    const std::vector<std::shared_ptr<DataBase>>&);

  ViewCols()                           = delete;
  ViewCols(ViewCols&&)                 = delete;
  ViewCols(const ViewCols&)            = delete;
  ViewCols& operator=(ViewCols&&)      = delete;
  ViewCols& operator=(const ViewCols&) = delete;

  void getColumn(const std::string&, std::vector<std::int8_t>&) const override;
  void getColumn(const std::string&, std::vector<std::int16_t>&) const override;
  void getColumn(const std::string&, std::vector<std::int32_t>&) const override;
  void getColumn(const std::string&, std::vector<std::int64_t>&) const override;
  void getColumn(const std::string&, std::vector<float>&) const override;
  void getColumn(const std::string&, std::vector<double>&) const override;
  void getColumn(const std::string&, std::vector<std::string>&) const override;

  void print() const override;

  ViewCols sliceRows(const std::string&, const std::int8_t, const std::int8_t) const;
  ViewCols sliceRows(const std::string&, const std::int8_t, const std::int16_t) const;
  ViewCols sliceRows(const std::string&, const std::int8_t, const std::int32_t) const;
  ViewCols sliceRows(const std::string&, const std::int8_t, const std::int64_t) const;
  ViewCols sliceRows(const std::string&, const std::int8_t, const float) const;
  ViewCols sliceRows(const std::string&, const std::int8_t, const double) const;
  ViewCols sliceRows(const std::string&, const std::int8_t, const std::string) const;

 private:
  template<typename T> void getColumn(const std::string&, std::vector<T>&, const std::int8_t) const;
  template<typename T> ViewCols sliceRows(const std::string&, const std::int8_t, const T) const;

  FunctionsCols funcs_;
  ViewColsData data_;
};
}  // namespace osdf

#endif  // CONTAINERS_VIEWCOLS_H_
