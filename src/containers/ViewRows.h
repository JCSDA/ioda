/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef CONTAINERS_VIEWROWS_H_
#define CONTAINERS_VIEWROWS_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "ioda/containers/ColumnMetadata.h"
#include "ioda/containers/ColumnMetadatum.h"
#include "ioda/containers/Functions.h"
#include "ioda/containers/FunctionsRows.h"
#include "ioda/containers/IView.h"
#include "ioda/containers/ViewRowsData.h"

namespace osdf {
class ViewRows : public IView {
 public:
  explicit ViewRows(const ColumnMetadata, const std::vector<std::shared_ptr<DataRow>>);

  ViewRows()                           = delete;
  ViewRows(ViewRows&&)                 = delete;
  ViewRows(const ViewRows&)            = delete;
  ViewRows& operator=(ViewRows&&)      = delete;
  ViewRows& operator=(const ViewRows&) = delete;

  void getColumn(const std::string&, std::vector<std::int8_t>&) const override;
  void getColumn(const std::string&, std::vector<std::int16_t>&) const override;
  void getColumn(const std::string&, std::vector<std::int32_t>&) const override;
  void getColumn(const std::string&, std::vector<std::int64_t>&) const override;
  void getColumn(const std::string&, std::vector<float>&) const override;
  void getColumn(const std::string&, std::vector<double>&) const override;
  void getColumn(const std::string&, std::vector<std::string>&) const override;

  void print() const override;

  ViewRows sliceRows(const std::string&, const std::int8_t, const std::int8_t) const;
  ViewRows sliceRows(const std::string&, const std::int8_t, const std::int16_t) const;
  ViewRows sliceRows(const std::string&, const std::int8_t, const std::int32_t) const;
  ViewRows sliceRows(const std::string&, const std::int8_t, const std::int64_t) const;
  ViewRows sliceRows(const std::string&, const std::int8_t, const float) const;
  ViewRows sliceRows(const std::string&, const std::int8_t, const double) const;
  ViewRows sliceRows(const std::string&, const std::int8_t, const std::string) const;

  ViewRows sliceRows(const std::function<const std::int8_t(const std::shared_ptr<DataRow>&)>) const;

  void sortRows(const std::string&, const std::int8_t);

  void sortRows(const std::string&, const std::function<std::int8_t(
                const std::shared_ptr<DatumBase>, const std::shared_ptr<DatumBase>)>);

 private:
  template<typename T> void getColumn(const std::string&, std::vector<T>&, const std::int8_t) const;
  template<typename T> ViewRows sliceRows(const std::string&, const std::int8_t, const T) const;

  FunctionsRows funcs_;
  ViewRowsData data_;
};
}  // namespace osdf

#endif  // CONTAINERS_VIEWROWS_H_
