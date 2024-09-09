/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef CONTAINERS_FUNCTIONSROWS_H_
#define CONTAINERS_FUNCTIONSROWS_H_

#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

#include "ioda/containers/DataRow.h"
#include "ioda/containers/DatumBase.h"
#include "ioda/containers/Functions.h"
#include "ioda/containers/IRowsData.h"

namespace osdf {
class FunctionsRows : public Functions {
 public:
  FunctionsRows();

  FunctionsRows(FunctionsRows&&)                 = delete;
  FunctionsRows(const FunctionsRows&)            = delete;
  FunctionsRows& operator=(FunctionsRows&&)      = delete;
  FunctionsRows& operator=(const FunctionsRows&) = delete;

  void sortRows(osdf::IRowsData*, const std::string&, const std::function<std::int8_t(
                const std::shared_ptr<DatumBase>, const std::shared_ptr<DatumBase>)>);

  template<typename T> const T getDatumValue(const std::shared_ptr<DatumBase>&) const;
  template<typename T> void setDatumValue(const std::shared_ptr<DatumBase>&, const T&) const;

  template<typename T> void getColumn(const osdf::IRowsData*,
                                      const std::int32_t, std::vector<T>&) const;

  template <typename F>
  void sortRows(osdf::IRowsData* data, const std::int32_t columnIndex, const F&& func) {
    // Build list of ordered indices.
    const std::int64_t sizeRows = data->getSizeRows();
    const std::size_t sizeRowsSz = static_cast<std::size_t>(sizeRows);
    std::vector<std::int64_t> indices(sizeRowsSz, 0);
    std::iota(std::begin(indices), std::end(indices), 0);    // Initial sequential list of indices.
    std::sort(std::begin(indices), std::end(indices), [&](const std::int64_t& i,
                                                          const std::int64_t& j) {
      std::shared_ptr<DatumBase>& datumA = data->getDataRow(i).getColumn(columnIndex);
      std::shared_ptr<DatumBase>& datumB = data->getDataRow(j).getColumn(columnIndex);
      return func(datumA, datumB);
    });
    // Swap data values for whole rows - explicit casting makes it look more complicated than it is.
    for (std::size_t i = 0; i < sizeRowsSz; ++i) {
      while (indices.at(i) != indices.at(static_cast<std::size_t>(indices.at(i)))) {
        const std::size_t iIdx = static_cast<std::size_t>(indices.at(i));
        std::swap(data->getDataRow(indices.at(i)), data->getDataRow(indices.at(iIdx)));
        std::swap(indices.at(i), indices.at(iIdx));
      }
    }
  }
};
}  // namespace osdf

#endif  // CONTAINERS_FUNCTIONSROWS_H_
